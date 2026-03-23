const logEl = document.getElementById('log');
const authInput = document.getElementById('auth-input');

function appendLog(line) {
  const stamp = new Date().toISOString().slice(11, 19);
  logEl.textContent = `[${stamp}] ${line}\n` + logEl.textContent;
}

function authHeader() {
  const value = authInput.value.trim();
  return value.length > 0 ? value : 'Bearer esp32wiimote_bearer_token_v1';
}

async function fetchWithAuth(path, init = {}) {
  const headers = {
    'Authorization': authHeader(),
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

async function postCommand(path, command, extra = {}) {
  const body = JSON.stringify({ command, ...extra });
  await fetchWithAuth(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body,
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

refreshStatus();
