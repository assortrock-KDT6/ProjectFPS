import { execFileSync, spawnSync } from "node:child_process";
import { cp, mkdir, readdir, rm, writeFile } from "node:fs/promises";
import path from "node:path";

const API_BASE_URL = "https://api.diversion.dev/v0";
const INCLUDED_ROOTS = ["Config", "Source"];
const FALLBACK_BRANCH = "develop";

const token = process.env.DIVERSION_API_TOKEN;
const repoId = process.env.DIVERSION_REPO_ID || "ProjectFPS";
const requestedBranch = process.env.REQUESTED_BRANCH || "*";
const gitWorkspace = path.resolve(process.env.GITHUB_WORKSPACE || process.cwd());
const runnerTemp = path.resolve(process.env.RUNNER_TEMP || path.join(gitWorkspace, ".tmp"));
const diversionWorkspace = path.join(runnerTemp, "projectfps-diversion-workspace");

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

function dv(args, cwd = runnerTemp) {
  return execFileSync("dv", args, {
    cwd,
    encoding: "utf8",
    stdio: "inherit",
  });
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

async function listBranches() {
  const response = await diversionFetch(
    `/repos/${encodeRepoRef(repoId)}/branches`,
  );
  const payload = await response.json();
  return payload.items || [];
}

async function stageBranch(branch) {
  const stagingRoot = path.join(runnerTemp, "projectfps-diversion-export");
  await rm(stagingRoot, { recursive: true, force: true });
  await mkdir(stagingRoot, { recursive: true });

  dv(
    ["checkout", branch.branch_id || branch.branch_name, "--discard-changes"],
    diversionWorkspace,
  );
  dv(["status"], diversionWorkspace);

  for (const root of INCLUDED_ROOTS) {
    await cp(path.join(diversionWorkspace, root), path.join(stagingRoot, root), {
      recursive: true,
    });
  }

  const entries = await readdir(stagingRoot, {
    recursive: true,
    withFileTypes: true,
  });
  const fileCount = entries.filter((entry) => entry.isFile()).length;

  return { stagingRoot, fileCount };
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
    const { stagingRoot, fileCount } = await stageBranch(branch);
    checkoutGitBranch(branch.branch_name);
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

await rm(diversionWorkspace, { recursive: true, force: true });
dv([
  "clone",
  repoId,
  diversionWorkspace,
  "--new-workspace",
  "--ref",
  branches[0].branch_id || branches[0].branch_name,
  "--nowait",
]);
dv(["preferences", "-add", "Config"], diversionWorkspace);
dv(["preferences", "-add", "Source"], diversionWorkspace);
dv(["status"], diversionWorkspace);

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
