#include "web_assets.h"

#include <cstddef>

namespace web_assets {

const char kStylesCss[] = R"ESP32WIIMOTE_CSS(:root {
  --bg: radial-gradient(circle at 20% 0%, #1f3558 0%, #0e1624 45%, #080c14 100%);
  --panel: rgba(12, 21, 36, 0.82);
  --ink: #e8f0ff;
  --muted: #96a8ca;
  --accent: #34d6a3;
  --warn: #ff6f61;
  --line: rgba(255, 255, 255, 0.12);
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  min-height: 100vh;
  font-family: "Trebuchet MS", "Avenir Next", "Segoe UI", sans-serif;
  color: var(--ink);
  background: var(--bg);
}

.shell {
  width: min(1800px, 96vw);
  margin: 24px auto;
  display: grid;
  gap: 16px;
  animation: fade-in 300ms ease-out;
}

.content-layout {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(100ch, 100ch);
  gap: 16px;
  align-items: stretch;
}

.controls-stack {
  display: grid;
  gap: 16px;
  min-width: 0;
}

.hero h1 {
  margin: 0;
  font-size: clamp(2rem, 7vw, 3.2rem);
  letter-spacing: 0.02em;
}

.eyebrow {
  margin: 0;
  color: var(--accent);
  font-weight: 700;
  letter-spacing: 0.18em;
}

.sub {
  margin-top: 8px;
  color: var(--muted);
}

.panel {
  border: 1px solid var(--line);
  background: var(--panel);
  border-radius: 14px;
  padding: 16px;
  backdrop-filter: blur(6px);
}

.status-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.label {
  display: block;
  color: var(--muted);
  font-size: 0.85rem;
}

.actions {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 10px;
}

.field-grid {
  margin-top: 10px;
  display: grid;
  grid-template-columns: 1fr 1fr auto;
  gap: 10px;
}

.btn {
  border: 1px solid var(--line);
  color: var(--ink);
  background: #1a2944;
  border-radius: 10px;
  padding: 10px 12px;
  font-weight: 700;
  cursor: pointer;
}

.btn:hover {
  transform: translateY(-1px);
}

.primary {
  background: #1f6f57;
}

.danger {
  background: #5f2630;
}

.input {
  width: 100%;
  border: 1px solid var(--line);
  border-radius: 10px;
  background: rgba(0, 0, 0, 0.18);
  color: var(--ink);
  padding: 10px;
}

.small {
  color: var(--muted);
  margin-top: 0;
}

.log {
  margin: 0;
  min-height: 140px;
  border-radius: 12px;
  border: 1px solid var(--line);
  background: rgba(0, 0, 0, 0.25);
  padding: 12px;
  overflow: auto;
  font-size: 0.85rem;
  white-space: pre-wrap;
  overflow-wrap: anywhere;
  word-break: break-word;
  max-inline-size: none;
}

.logs-sidebar {
  min-height: 0;
  border: 1px solid var(--line);
  border-radius: 14px;
  background: rgba(8, 14, 24, 0.9);
  backdrop-filter: blur(6px);
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 12px;
  min-width: 0;
  position: sticky;
  top: 24px;
  height: calc(100vh - 48px);
}

.logs-sidebar h2 {
  margin: 0;
  color: var(--accent);
  font-size: 1rem;
  letter-spacing: 0.08em;
}

#api-log {
  flex: 1;
  min-height: 0;
  width: 100%;
  overflow-y: auto;
}

@keyframes fade-in {
  from {
    opacity: 0;
    transform: translateY(8px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 640px) {
  .shell {
    margin: 16px auto;
    gap: 12px;
  }

  .content-layout {
    grid-template-columns: 1fr;
  }

  .panel {
    padding: 14px;
  }

  .field-grid {
    grid-template-columns: 1fr;
  }

  .logs-sidebar {
    min-height: 45vh;
    border: 1px solid var(--line);
    position: static;
    height: auto;
  }
}
)ESP32WIIMOTE_CSS";

const size_t kStylesCssLen = sizeof(kStylesCss) - 1U;

const char *stylesCss() {
    return kStylesCss;
}

size_t stylesCssLen() {
    return kStylesCssLen;
}

}  // namespace web_assets