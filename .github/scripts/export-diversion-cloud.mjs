import { execFileSync, spawnSync } from "node:child_process";
import { createHash } from "node:crypto";
import { chmod, cp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import path from "node:path";

const API_BASE_URL = "https://api.diversion.dev/v0";
const INCLUDED_ROOTS = ["Config", "Source"];
const FALLBACK_BRANCH = "develop";

const token = process.env.DIVERSION_API_TOKEN;
const repoId = process.env.DIVERSION_REPO_ID || "ProjectFPS";
const repoName = process.env.DIVERSION_REPO_NAME || "ProjectFPS";
const requestedBranch = process.env.REQUESTED_BRANCH || "*";
const gitWorkspace = path.resolve(process.env.GITHUB_WORKSPACE || process.cwd());
const runnerTemp = path.resolve(process.env.RUNNER_TEMP || path.join(gitWorkspace, ".tmp"));

const diversionCloneRoot = path.join(
  runnerTemp,
  "projectfps-diversion-cli",
);
const diversionWorkspace = path.join(
  diversionCloneRoot,
  repoName,
);

let diversionWorkspaceReady = false;
if (!token) {
  throw new Error("GitHub secret DIVERSION_API_TOKEN is not configured.");
}

function git(args, options = {}) {
  return execFileSync("git", args, {
    cwd: gitWorkspace,
    encoding: "utf8",
    stdio: options.capture ? ["ignore", "pipe", "pipe"] : "inherit",
  });
}

function gitStatus(args) {
  return spawnSync("git", args, {
    cwd: gitWorkspace,
    encoding: "utf8",
    stdio: "pipe",
  });
}

function diversionCli(args, options = {}) {
  return execFileSync("dv", args, {
    cwd: options.cwd || gitWorkspace,
    encoding: "utf8",
    stdio: "inherit",
  });
}

async function checkoutDiversionBranch(branchName) {
  if (!diversionWorkspaceReady) {
    await rm(diversionCloneRoot, {
      recursive: true,
      force: true,
    });
    await mkdir(diversionCloneRoot, {
      recursive: true,
    });

    // 공식 CI 예제와 동일하게 현재 폴더 아래에 저장소를 복제한다.
    diversionCli(
      [
        "clone",
        "-new-workspace",
        repoName,
      ],
      { cwd: diversionCloneRoot },
    );

    diversionWorkspaceReady = true;
  }

  // clone 옵션의 ref 처리 대신 명시적으로 브랜치를 전환한다.
  diversionCli(
    [
      "checkout",
      branchName,
    ],
    { cwd: diversionWorkspace },
  );

  // 모든 파일이 내려올 때까지 대기한다.
  diversionCli(
    ["status"],
    { cwd: diversionWorkspace },
  );
}

async function diversionFetch(apiPath) {
  const url = `${API_BASE_URL}${apiPath}`;
  let lastError;

  for (let attempt = 1; attempt <= 4; attempt += 1) {
    const response = await fetch(url, {
      headers: {
        accept: "*/*",
        authorization: `Bearer ${token}`,
      },
    });

    if (response.ok) {
      return response;
    }

    const details = await response.text();
    lastError = new Error(
      `Diversion API request failed (${response.status}): ${apiPath}\n${details}`,
    );

    if (response.status !== 429 && response.status < 500) {
      throw lastError;
    }

    await new Promise((resolve) => setTimeout(resolve, attempt * 1000));
  }

  throw lastError;
}

function encodeRepoRef(value) {
  return encodeURIComponent(value);
}

function validateRepositoryPath(repositoryPath) {
  const normalized = repositoryPath.replaceAll("\\", "/").replace(/^\/+/, "");
  const segments = normalized.split("/");

  if (
    !normalized ||
    path.posix.isAbsolute(normalized) ||
    segments.some((segment) => !segment || segment === "." || segment === "..")
  ) {
    throw new Error(`Unsafe Diversion repository path: ${repositoryPath}`);
  }

  return normalized;
}

async function listBranches() {
  const response = await diversionFetch(
    `/repos/${encodeRepoRef(repoId)}/branches`,
  );
  const payload = await response.json();
  return payload.items || [];
}

async function listFiles(refId) {
  const query = new URLSearchParams({
    recurse: "true",
    include_deleted: "false",
    use_selective_sync: "false",
    include_download_urls: "true",
  });
  const response = await diversionFetch(
    `/repos/${encodeRepoRef(repoId)}/tree_content/${encodeRepoRef(refId)}?${query}`,
  );
  const body = await response.text();
  const files = body
    .split(/\r?\n/)
    .filter((line) => line.trim())
    .map((line) => JSON.parse(line));

  return files.filter((entry) => {
    if (!entry.blob) {
      return false;
    }
    const repositoryPath = entry.path.replaceAll("\\", "/").replace(/^\/+/, "");
    return INCLUDED_ROOTS.some(
      (root) => repositoryPath === root || repositoryPath.startsWith(`${root}/`),
    );
  });
}

async function stageBranch(branch) {
  const stagingRoot = path.join(
    runnerTemp,
    "projectfps-diversion-export",
  );

  await rm(stagingRoot, {
    recursive: true,
    force: true,
  });

  for (const root of INCLUDED_ROOTS) {
    await mkdir(path.join(stagingRoot, root), {
      recursive: true,
    });
  }

  // 브랜치 이름 대신 정확한 Diversion 커밋을 받는다.
  await checkoutDiversionBranch(branch.branch_name);

  // API는 파일 내용이 아니라 검증용 크기와 SHA만 조회한다.
  const entries = await listFiles(branch.commit_id);

  if (entries.length === 0) {
    throw new Error(
      `No Config/Source files were returned for ${branch.branch_name}.`,
    );
  }

  for (const entry of entries) {
    const repositoryPath = validateRepositoryPath(entry.path);

    const sourcePath = path.resolve(
      diversionWorkspace,
      ...repositoryPath.split("/"),
    );

    const sourcePrefix = `${path.resolve(diversionWorkspace)}${path.sep}`;
    if (!sourcePath.startsWith(sourcePrefix)) {
      throw new Error(
        `Refusing to read outside Diversion workspace: ${sourcePath}`,
      );
    }

    const bytes = await readFile(sourcePath);
    const expectedSize = Number(entry.blob.size);

    if (bytes.length !== expectedSize) {
      throw new Error(
        `Diversion CLI file size mismatch for ${repositoryPath}: ` +
          `expected ${expectedSize}, received ${bytes.length}.`,
      );
    }

    if (typeof entry.blob.sha === "string" && entry.blob.sha) {
      const actualSha = createHash("sha1")
        .update(bytes)
        .digest("hex");

      if (actualSha !== entry.blob.sha.toLowerCase()) {
        throw new Error(
          `Diversion CLI SHA mismatch for ${repositoryPath}.`,
        );
      }
    }

    const destination = path.resolve(
      stagingRoot,
      ...repositoryPath.split("/"),
    );

    const stagingPrefix = `${path.resolve(stagingRoot)}${path.sep}`;
    if (!destination.startsWith(stagingPrefix)) {
      throw new Error(
        `Refusing to write outside staging directory: ${destination}`,
      );
    }

    await mkdir(path.dirname(destination), {
      recursive: true,
    });
    await writeFile(destination, bytes);

    if (entry.mode === 33261) {
      await chmod(destination, 0o755);
    }
  }

  return {
    stagingRoot,
    fileCount: entries.length,
  };
}

async function mirrorIncludedRoots(stagingRoot) {
  for (const root of INCLUDED_ROOTS) {
    const destination = path.resolve(gitWorkspace, root);
    const expectedPrefix = `${gitWorkspace}${path.sep}`;
    if (!destination.startsWith(expectedPrefix)) {
      throw new Error(`Refusing to mirror outside Git workspace: ${destination}`);
    }

    await rm(destination, { recursive: true, force: true });
    await cp(path.join(stagingRoot, root), destination, { recursive: true });
  }
}

function checkoutGitBranch(branchName) {
  const validation = gitStatus(["check-ref-format", "--branch", branchName]);
  if (validation.status !== 0) {
    throw new Error(`Invalid Git branch name from Diversion: ${branchName}`);
  }

  const remoteRef = `refs/remotes/origin/${branchName}`;
  const existing = gitStatus(["show-ref", "--verify", "--quiet", remoteRef]);
  if (existing.status === 0) {
    git(["checkout", "-B", branchName, `origin/${branchName}`]);
    return;
  }

  git(["checkout", "-B", branchName, `origin/${FALLBACK_BRANCH}`]);
}

async function exportBranch(branch) {
  console.log(`::group::Exporting Diversion Cloud branch '${branch.branch_name}'`);
  try {
    checkoutGitBranch(branch.branch_name);
    const { stagingRoot, fileCount } = await stageBranch(branch);
    await mirrorIncludedRoots(stagingRoot);

    git(["add", "-A", "--", ...INCLUDED_ROOTS]);
    const diff = gitStatus(["diff", "--cached", "--quiet"]);

    if (diff.status === 0) {
      console.log(`No Config/Source changes for '${branch.branch_name}'.`);
      return {
        branch: branch.branch_name,
        result: "No changes",
        commit: branch.commit_id,
        files: fileCount,
      };
    }
    if (diff.status !== 1) {
      throw new Error(`git diff --cached failed: ${diff.stderr}`);
    }

    git([
      "commit",
      "-m",
      `sync: Diversion/${branch.branch_name} source (${branch.commit_id})`,
    ]);
    git(["push", "origin", `HEAD:refs/heads/${branch.branch_name}`]);

    return {
      branch: branch.branch_name,
      result: "Pushed",
      commit: branch.commit_id,
      files: fileCount,
    };
  } finally {
    console.log("::endgroup::");
  }
}

let branches = await listBranches();
if (requestedBranch !== "*") {
  branches = branches.filter((branch) => branch.branch_name === requestedBranch);
  if (branches.length === 0) {
    throw new Error(`Diversion branch was not found: ${requestedBranch}`);
  }
}

if (branches.length === 0) {
  throw new Error("No Diversion branches were returned by the API.");
}

git(["config", "user.name", "github-actions[bot]"]);
git([
  "config",
  "user.email",
  "41898282+github-actions[bot]@users.noreply.github.com",
]);
git(["fetch", "origin", "--prune"]);

const fallback = gitStatus([
  "show-ref",
  "--verify",
  "--quiet",
  `refs/remotes/origin/${FALLBACK_BRANCH}`,
]);
if (fallback.status !== 0) {
  throw new Error(`Fallback GitHub branch does not exist: ${FALLBACK_BRANCH}`);
}

const results = [];
for (const branch of branches) {
  results.push(await exportBranch(branch));
}

if (process.env.GITHUB_STEP_SUMMARY) {
  const rows = results.map(
    (item) =>
      `| \`${item.branch}\` | ${item.result} | ${item.files} | \`${item.commit}\` |`,
  );
  await writeFile(
    process.env.GITHUB_STEP_SUMMARY,
    [
      "# Diversion Cloud source export",
      "",
      "| Branch | Result | Files | Diversion commit |",
      "|---|---|---:|---|",
      ...rows,
      "",
    ].join("\n"),
    { flag: "a" },
  );
}
