#include "web_assets.h"

#include <cstddef>

namespace web_assets {

const char kIndexHtml[] = R"HTML(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Wiimote Control</title>
  <link rel="stylesheet" href="/styles.css">
</head>
<body>
  <main class="shell">
    <header class="hero">
      <p class="eyebrow">ESP32 WIIMOTE</p>
      <h1>Control Bridge</h1>
      <p class="sub">Monitor status and send runtime commands from your browser.</p>
    </header>
    <div class="content-layout">
      <div class="controls-stack">
        <section class="panel" aria-label="Connection status">
          <h2>Status</h2>
          <div class="status-grid">
            <div>
              <span class="label">Connected</span>
              <strong id="status-connected">-</strong>
            </div>
            <div>
              <span class="label">Battery</span>
              <strong id="status-battery">-</strong>
            </div>
          </div>
          <br>
          <button id="refresh-status" class="btn primary">Refresh Status</button>
        </section>
        <section class="panel" aria-label="Authentication">
          <h2>Auth</h2>
          <p class="small">Insert the token</code>.</p>
          <input id="auth-input" class="input" type="text" placeholder="Authorization header value">
        </section>
        <section class="panel" aria-label="Controller actions">
          <h2>Actions</h2>
          <div class="actions">
            <button id="scan-start" class="btn">Start Scan</button>
            <button id="scan-stop" class="btn">Stop Scan</button>
            <button id="request-status" class="btn">Request Status</button>
            <button id="disconnect" class="btn danger">Disconnect</button>
          </div>
        </section>
        <section class="panel" aria-label="Wi-Fi control">
          <h2>Wi-Fi Control</h2>
          <div class="status-grid">
            <div>
              <span class="label">Control</span>
              <strong id="wifi-control-enabled">-</strong>
            </div>
            <div>
              <span class="label">Delivery Mode</span>
              <strong id="wifi-control-mode">-</strong>
            </div>
            <div>
              <span class="label">API Token</span>
              <strong id="wifi-control-token">-</strong>
            </div>
          </div>
          <br>
          <div class="actions">
            <button id="refresh-wifi-control" class="btn primary">Refresh Wi-Fi Status</button>
            <button id="wifi-enable" class="btn">Enable Wi-Fi Control</button>
            <button id="wifi-disable" class="btn danger">Disable Wi-Fi Control</button>
            <button id="wifi-mode-rest" class="btn">Mode: REST</button>
            <button id="wifi-mode-rest-ws" class="btn">Mode: REST + WS</button>
            <button id="wifi-restart" class="btn">Restart Wi-Fi</button>
          </div>
          <br>
          <div class="field-grid">
            <input id="wifi-ssid" class="input" type="text" placeholder="Wi-Fi SSID">
            <input id="wifi-password" class="input" type="text" placeholder="Wi-Fi Password">
            <button id="wifi-set-network" class="btn">Set Network</button>
          </div>
          <div class="field-grid">
            <input id="wifi-token" class="input" type="text" placeholder="New API token">
            <button id="wifi-set-token" class="btn">Set API Token</button>
          </div>
        </section>
      </div>

      <aside class="logs-sidebar" aria-label="API logs">
        <h2>API Logs</h2>
        <pre id="api-log" class="log" aria-live="polite"></pre>
      </aside>
    </div>
  </main>

  <script src="/app.js"></script>
</body>
</html>
)HTML";

const size_t kIndexHtmlLen = sizeof(kIndexHtml) - 1U;

const char *indexHtml() {
    return kIndexHtml;
}

size_t indexHtmlLen() {
    return kIndexHtmlLen;
}

}  // namespace web_assets