# ProjectFPS: Diversion source export

This automation exports only `Config/` and `Source/` from every Diversion
branch to the GitHub branch with the same name. It never enables Diversion's
Git synchronization.

## 1. Windows runner

Register a GitHub Actions self-hosted runner on the Windows machine that has
Diversion installed and authenticated. Add these labels to the runner:

- `Windows`
- `X64`
- `projectfps-sync`

Run the runner interactively as the same Windows user that is signed in to
Diversion, or configure its service to run as that user.

Create this GitHub repository variable:

- Name: `DIVERSION_WORKSPACE_PATH`
- Value: the path to a clean, dedicated Diversion workspace for `ProjectFPS`

Do not point this variable at a workspace where a developer has pending edits.
The exporter refuses to run when it detects pending Diversion changes.

## 2. First test

Open GitHub Actions, select **Export Diversion source branches**, choose
**Run workflow**, and keep the branch input as `*`.

The workflow mirrors `Config/` and `Source/`, commits only when those folders
changed, and pushes to the matching branch. A Diversion branch that does not
exist on GitHub is created from `develop` before its source snapshot is added.
GitHub branches are not automatically deleted when a Diversion branch is
deleted.

## 3. Notion webhook bridge

Deploy the files in `.github/notion-bridge/` as a Cloudflare Worker. Configure
these Worker secrets:

- `GITHUB_TOKEN`: a fine-grained token restricted to
  `assortrock-KDT6/ProjectFPS`, with repository Contents read/write permission
- `BRIDGE_SECRET`: a long random value

In the Notion database automation or button:

1. Add **Send webhook**.
2. Set the URL to the deployed Worker URL.
3. Add header `x-projectfps-sync-secret` with the same `BRIDGE_SECRET` value.

The bridge deliberately ignores Notion page content. Each accepted request
starts an all-branch source export and returns HTTP 202.

