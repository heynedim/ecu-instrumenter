# ECU Instrumenter

ECU Instrumenter is a native SDL2 diagnostics terminal for the Miyoo Mini Plus running OnionOS. The app is now a small C++/SDL2 project with source in `src/`.

## Features

- Native SDL2 UI tuned for 640x480 handheld display.
- Simulated OBD signal acquisition startup screen.
- Live telemetry dashboard with RPM, latency, coolant, and voltage graphs.
- Fault-code view with selectable local DTC clearing.
- Raw packet monitor with pause/export support.
- OnionOS launch/deploy path using `launch.sh` and `config.json`.

## Controls

| Action | Desktop Key | Miyoo Button |
|:---|:---:|:---:|
| Navigate | Arrow keys | D-Pad |
| Confirm / A | `z`, Space, Enter | A |
| Back / B | `b`, Escape | B |
| Packet shortcut / SELECT | Backspace | Select |
| Overlay / START | Tab | Start |
| Clear / X action | `x`, Left Shift | X |
| Y action | `y` | Y |

## Development

Install SDL2, then build or run:

```bash
make check
make run
```

## Deployment

```bash
make deploy
make deploy MIYOO_IP=x.x.x.x
```

Deployment builds `bin/ecu-instrumenter` and syncs the OnionOS app bundle to `/mnt/SDCARD/App/ECUInstrumenter/`.

## Structure

| Path | Purpose |
|:---|:---|
| `src/main.cpp` | SDL2 entry point and loop |
| `src/app_state.cpp` | Globals, drawing primitives, font/data tables |
| `src/render.cpp` | Screens and HUD rendering |
| `src/telemetry.cpp` | Simulated OBD/packet generation and export |
| `src/input.cpp` | Input handling and navigation |
| `Makefile` | C++ build, run, clean, and deploy commands |
| `build/` | Generated object/dependency files |
| `bin/` | Generated `ecu-instrumenter` binary |
| `launch.sh` | OnionOS app launcher |
| `config.json` | OnionOS app metadata |
| `assets/icon.png` | OnionOS app icon |
