const GITHUB_API_URL =
  "https://api.github.com/repos/assortrock-KDT6/ProjectFPS/dispatches";

function jsonResponse(body, status) {
  return new Response(JSON.stringify(body), {
    status,
    headers: { "content-type": "application/json; charset=utf-8" },
  });
}

export default {
  async fetch(request, env) {
    if (request.method !== "POST") {
      return jsonResponse({ error: "method_not_allowed" }, 405);
    }

    const suppliedSecret = request.headers.get("x-projectfps-sync-secret");
    if (!env.BRIDGE_SECRET || suppliedSecret !== env.BRIDGE_SECRET) {
      return jsonResponse({ error: "unauthorized" }, 401);
    }

    if (!env.GITHUB_TOKEN) {
      return jsonResponse({ error: "github_token_not_configured" }, 500);
    }

    const githubResponse = await fetch(GITHUB_API_URL, {
      method: "POST",
      headers: {
        accept: "application/vnd.github+json",
        authorization: `Bearer ${env.GITHUB_TOKEN}`,
        "content-type": "application/json",
        "user-agent": "projectfps-notion-bridge",
        "x-github-api-version": "2022-11-28",
      },
      body: JSON.stringify({
        event_type: "diversion_source_sync",
        client_payload: { branch: "*", source: "notion" },
      }),
    });

    if (!githubResponse.ok) {
      const details = await githubResponse.text();
      return jsonResponse(
        { error: "github_dispatch_failed", status: githubResponse.status, details },
        502,
      );
    }

    return jsonResponse({ accepted: true, branches: "*" }, 202);
  },
};

