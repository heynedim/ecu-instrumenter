# ECU Instrumenter — Project Rules

This project is native C++/SDL2 only. Do not reintroduce the old Python/PyGame app.

## Project Identity

- **Name:** ECU Instrumenter
- **Target hardware:** Miyoo Mini Plus running OnionOS
- **Dev platform:** macOS / Linux
- **UI framework:** SDL2, 640x480 logical resolution
- **Language:** C++11
- **Primary source:** `src/main.cpp` plus focused implementation files under `src/`

## Build Commands

```bash
make check
make run
make clean
make run-server
make run-dev
make probe-obd
make deploy
make deploy MIYOO_IP=x.x.x.x
```

Before deploy, `make check` must pass.

## Native Code Rules

- Keep the Miyoo target in mind: low memory, slow CPU, D-pad-first input.
- Stay compatible with C++11 unless the Makefile is intentionally updated.
- Prefer small, direct functions over framework-style abstractions.
- Avoid heap churn in hot draw paths.
- Keep generated files under `build/`, `bin/`, or `data/`.

## UI Direction

- Visual language: restrained OEM/cyberpunk diagnostic terminal.
- Palette: black, phosphor green, amber, cyan, muted technical lines.
- Startup should feel like waiting for OBD signal acquisition.
- Main modules are `LIVE TELEMETRY`, `FAULT CODES`, `PACKET MONITOR`, and `SYSTEM`.
- Do not restore Trip Recorder, Engine Health, or Status Feed.

## File Layout

| Path | Purpose |
|:---|:---|
| `src/main.cpp` | SDL2 entry point and loop |
| `src/app_state.cpp` | Globals, drawing primitives, font/data tables |
| `src/config.cpp` | Runtime `ecu_config.ini` parser |
| `src/obd_client.cpp` | Non-blocking TCP OBD client |
| `src/render.cpp` | Screens and HUD rendering |
| `src/telemetry.cpp` | Simulated OBD/packet generation and export |
| `src/input.cpp` | Input handling and navigation |
| `tools/obd_sim_server.cpp` | Desktop OBD simulator server |
| `tools/obd_probe.cpp` | Command-line OBD endpoint health check |
| `Makefile` | Native build, run, clean, and deploy commands |
| `launch.sh` | OnionOS launcher |
| `config.json` | OnionOS app metadata |
| `ecu_config.ini` | User-editable OBD connection settings |
| `assets/icon.png` | OnionOS app icon |

## Controls

| Action | Key |
|:---|:---|
| Navigate | Arrow keys / D-pad |
| Confirm / A | `z`, Space, Enter |
| Back / B | `b`, Escape |
| SELECT packet shortcut | Backspace |
| START overlay | Tab |
| X action | `x`, Left Shift |
| Y action | `y` |
