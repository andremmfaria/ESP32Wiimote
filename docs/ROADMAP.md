# ESP32Wiimote Roadmap

This document is the canonical roadmap for runtime control improvements.

Plan files in `plans/` are execution aids. The source of truth for externally documented behavior is the `docs/` directory.

## Scope

The runtime-control roadmap is centered on two external transports:

- Serial
- Wi-Fi

Bluetooth remains dedicated to Wii Remote communication and is not used as an external control transport.

## Objectives

1. Surface Bluetooth controller controls as first-class runtime operations.
2. Expose candidate runtime methods consistently on the public API.
3. Add a Serial control surface for command writes and status reads.
4. Add a Wi-Fi control surface for authenticated command writes and status reads.

## Workstreams

### Workstream A: Bluetooth controller controls

Target capabilities:

- Enable/disable scan
- Start/stop discovery
- Disconnect active controller
- Enable/disable auto-reconnect
- Clear reconnect cache
- Query controller state

### Workstream B: Public API completion

Public API acts as the single runtime-control surface used by:

- firmware calls
- Serial command handlers
- Wi-Fi handlers

### Workstream C: Serial control API

Line-oriented ASCII protocol over the primary serial console.

Baseline characteristics:

- one command per line
- `wm` command prefix
- bounded parser and token counts
- deterministic error text responses

### Workstream D: Wi-Fi control API

REST-first authenticated API over Wi-Fi.

Baseline characteristics:

- read endpoints for snapshots (`/api/wiimote/input`, `/api/wiimote/status`, `/api/controller/status`)
- write endpoints for runtime commands
- static OpenAPI document at `/openapi.json`
- minimal on-device web UI at `/`

## Phase Plan

### Phase 1: Public runtime API completion

Deliverables:

- missing runtime write methods exposed on `ESP32Wiimote`
- Bluetooth controller operations exposed on `ESP32Wiimote`
- TinyWiimote bridge completion
- disconnect HCI builder
- API documentation updates

Exit criteria:

- all intended runtime methods reachable from public API
- native tests cover bridge and API behavior

### Phase 2: Controller state and command hardening

Deliverables:

- authoritative controller transition model
- deterministic state guards in controller command paths
- scanning lifecycle tracking from command submission
- reconnect policy controls validated under churn

Exit criteria:

- repeated scan/discovery/disconnect sequences remain deterministic
- invalid transitions are rejected cleanly and predictably

### Phase 3: Serial API

Deliverables:

- serial parser
- serial dispatcher
- response formatter
- optional privileged unlock window

Status:

- Completed

Exit criteria:

- primary runtime commands available over Serial
- status/config endpoints usable from host tools

Current implementation snapshot:

- parser/dispatcher/formatter wired through `ESP32Wiimote::task()`
- one complete command line processed per `task()` call
- unlock window for privileged serial commands (`wm unlock <seconds>`)
- native serial suites cover parser, dispatcher, formatter, session, and integration paths

### Phase 4: Wi-Fi API

Deliverables:

- authenticated REST API (Bearer and/or Basic)
- read snapshots for input/status/config/controller status
- write endpoints for runtime and controller commands
- static OpenAPI and minimal web page routing

Exit criteria:

- runtime state readable remotely
- runtime commands accepted/rejected deterministically
- Wi-Fi load does not destabilize core Wiimote behavior

### Phase 5: Reliability and persistence refinement

Deliverables:

- optional queued command execution and result tracking
- optional persistence for reconnect/runtime policy fields
- optional split WebSocket events for input/status

Exit criteria:

- stable mixed local/remote command behavior
- persistence and event features validated when enabled

## Cross-Cutting Requirements

- preserve low-latency input processing
- keep handlers non-blocking
- validate aggressively at API boundaries
- add tests for all modified behavior
- target 80% coverage on modified/new files

## Related Docs

- [Architecture](ARCHITECTURE.md)
- [API Reference](API.md)
- [Testing Guide](TESTING.md)
- [Troubleshooting](TROUBLESHOOTING.md)
