#include "web_assets.h"

#include <cstddef>

namespace web_assets {

const char kAppJs[] = R"ESP32WIIMOTE_JS(const logEl = document.getElementById('api-log');
const authInput = document.getElementById('auth-input');
const kMaxLogMessages = 1000;
let gApiMessages = [];

function appendLog(line) {
  const stamp = new Date().toISOString().slice(11, 19);
  gApiMessages.push(`[${stamp}] ${line}`);
  if (gApiMessages.length > kMaxLogMessages) {
    gApiMessages = gApiMessages.slice(gApiMessages.length - kMaxLogMessages);
  }
  logEl.textContent = gApiMessages.join('\n');
  logEl.scrollTop = logEl.scrollHeight;
}

function authHeader() {
  const value = authInput.value.trim();
  if (value.length === 0) {
    appendLog('ERROR missing auth token: set the Auth field before sending requests');
    return null;
  }

  return `Bearer ${value}`;
}

async function fetchWithAuth(path, init = {}) {
  const authorization = authHeader();
  if (!authorization) {
    const text = '{"status":"error","message":"missing auth token"}';
    appendLog(`${init.method || 'GET'} ${path} -> 400 ${text}`);
    return { res: { ok: false, status: 400 }, text };
  }

  const headers = {
    'Authorization': authorization,
    ...(init.headers || {}),
  };

  const res = await fetch(path, { ...init, headers });
  const text = await res.text();
  appendLog(`${init.method || 'GET'} ${path} -> ${res.status} ${text}`);
  return { res, text };
}

function setStatusText(connected, battery) {
  document.getElementById('status-connected').textContent = connected ? 'yes' : 'no';
  document.getElementById('status-battery').textContent = `${battery}%`;
}

function setWifiControlText(state) {
  const enabledEl = document.getElementById('wifi-control-enabled');
  const modeEl = document.getElementById('wifi-control-mode');
  const tokenEl = document.getElementById('wifi-control-token');

  if (enabledEl) {
    enabledEl.textContent = state.enabled ? 'enabled' : 'disabled';
  }
  if (modeEl) {
    modeEl.textContent = state.restAndWebSocket ? 'rest-ws' : 'rest';
  }
  if (tokenEl) {
    tokenEl.textContent = state.hasToken ? 'present' : 'missing';
  }
}

async function refreshStatus() {
  const { res, text } = await fetchWithAuth('/api/wiimote/status');
  if (!res.ok) {
    return;
  }

  try {
    const data = JSON.parse(text);
    setStatusText(Boolean(data.connected), Number(data.batteryLevel || 0));
  } catch (err) {
    appendLog(`status parse error: ${err}`);
  }
}

async function refreshWifiControl() {
  const { res, text } = await fetchWithAuth('/api/wifi/control');
  if (!res.ok) {
    return;
  }

  try {
    const data = JSON.parse(text);
    setWifiControlText({
      enabled: Boolean(data.enabled),
      restAndWebSocket: Boolean(data.restAndWebSocket),
      hasToken: Boolean(data.hasToken),
    });
  } catch (err) {
    appendLog(`wifi control parse error: ${err}`);
  }
}

async function postCommand(path, command, extra = {}) {
  const body = JSON.stringify({ command, ...extra });
  await fetchWithAuth(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body,
  });
}

async function postWifi(path, command, extra = {}) {
  return fetchWithAuth(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ command, ...extra }),
  });
}

document.getElementById('refresh-status').addEventListener('click', refreshStatus);
document.getElementById('scan-start').addEventListener('click', () =>
  postCommand('/api/wiimote/commands/scan', 'scan_start')
);
document.getElementById('scan-stop').addEventListener('click', () =>
  postCommand('/api/wiimote/commands/scan', 'scan_stop')
);
document.getElementById('request-status').addEventListener('click', () =>
  postCommand('/api/wiimote/commands/request-status', 'request_status')
);
document.getElementById('disconnect').addEventListener('click', () =>
  postCommand('/api/wiimote/commands/disconnect', 'disconnect', { reason: '22' })
);

document.getElementById('refresh-wifi-control')?.addEventListener('click', refreshWifiControl);
document.getElementById('wifi-enable')?.addEventListener('click', async () => {
  await postWifi('/api/wifi/control', 'set_wifi_control', { enabled: 'true' });
  await refreshWifiControl();
});
document.getElementById('wifi-disable')?.addEventListener('click', async () => {
  await postWifi('/api/wifi/control', 'set_wifi_control', { enabled: 'false' });
  await refreshWifiControl();
});
document.getElementById('wifi-mode-rest')?.addEventListener('click', async () => {
  await postWifi('/api/wifi/delivery-mode', 'set_wifi_mode', { mode: 'rest' });
  await refreshWifiControl();
});
document.getElementById('wifi-mode-rest-ws')?.addEventListener('click', async () => {
  await postWifi('/api/wifi/delivery-mode', 'set_wifi_mode', { mode: 'rest-ws' });
  await refreshWifiControl();
});
document.getElementById('wifi-set-network')?.addEventListener('click', async () => {
  const ssid = document.getElementById('wifi-ssid')?.value || '';
  const password = document.getElementById('wifi-password')?.value || '';
  await postWifi('/api/wifi/network', 'set_wifi_network', { ssid, password });
});
document.getElementById('wifi-restart')?.addEventListener('click', async () => {
  await postWifi('/api/wifi/restart', 'restart_wifi_control');
  await refreshWifiControl();
});
document.getElementById('wifi-set-token')?.addEventListener('click', async () => {
  const token = document.getElementById('wifi-token')?.value || '';
  await postWifi('/api/wifi/token', 'set_wifi_token', { token });
  await refreshWifiControl();
});

refreshStatus();
refreshWifiControl();
)ESP32WIIMOTE_JS";

const size_t kAppJsLen = sizeof(kAppJs) - 1U;

const char *appJs() {
    return kAppJs;
}

size_t appJsLen() {
    return kAppJsLen;
}

}  // namespace web_assets