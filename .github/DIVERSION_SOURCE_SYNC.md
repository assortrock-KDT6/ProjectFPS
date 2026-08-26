# ProjectFPS: Diversion source export

This automation exports only `Config/` and `Source/` from every Diversion
Cloud branch to the GitHub branch with the same name. It never enables
Diversion's Git synchronization and does not depend on a local workstation.

## 1. Diversion API token

Generate an API token in **Diversion → Settings → Integrations**. API tokens
start with `dvk_` and are shown only once. Confirm that token generation is
available for the Diversion account being used; it is available for this
ProjectFPS Indie account.

Create this GitHub Actions repository secret:

- Name: `DIVERSION_API_TOKEN`
- Value: the generated Diversion API token

Never commit the token. The workflow reads it only from GitHub's encrypted
Actions secret store.

## 2. First test

Open GitHub Actions, select **Export Diversion source branches**, choose
**Run workflow**, and keep the branch input as `*`.

GitHub's hosted runner reads the branch list from the Diversion Cloud API,
then uses Diversion's official Linux CLI in a temporary selective-sync
workspace. Only `Config/` and `Source/` are copied into Git, committed when
those folders changed, and pushed to the matching branch. A Diversion branch
that does not exist on GitHub is created from `develop` before its source
snapshot is added. GitHub branches are not automatically deleted when a
Diversion branch is deleted.

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
