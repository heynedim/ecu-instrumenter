# ECU Instrumenter

ECU Instrumenter is a native SDL2 diagnostics terminal for the Miyoo Mini Plus running OnionOS. The app is now a small C++/SDL2 project with source in `src/`.

## Features

- Native SDL2 UI tuned for 640x480 handheld display.
- Simulated OBD signal acquisition startup screen.
- Live telemetry dashboard with RPM, latency, coolant, and voltage graphs.
- Fault-code view with selectable local DTC clearing.
- Raw packet monitor with pause/export support.
- Configurable OBD TCP endpoint with simulator fallback.
- OnionOS launch/deploy path using `launch.sh` and `config.json`.

## Runtime Configuration

OBD connection settings live in `ecu_config.ini`:

```ini
[obd]
host=127.0.0.1
port=35000
connect_timeout_ms=900
poll_interval_ms=120
fallback_sim=true
```

For a real ELM327 Wi-Fi adapter, change `host` and `port` before deploying, or edit the deployed file on device:

```text
/mnt/SDCARD/App/ECUInstrumenter/ecu_config.ini
```

Common ELM327 Wi-Fi adapters use:

```ini
[obd]
host=192.168.0.10
port=35000
connect_timeout_ms=900
poll_interval_ms=120
fallback_sim=false
```

Some adapters use port `23` instead of `35000`. Check the adapter label, listing, or vendor notes.

Configuration only tells ECU Instrumenter where to connect. It does not guarantee the adapter will work. The adapter must be a standard ELM327-compatible Wi-Fi TCP adapter that accepts raw ASCII OBD commands such as `010C` and returns ELM-style responses such as `41 0C xx xx >`.

Before using the app with a real vehicle, connect the Miyoo/device to the ELM327 Wi-Fi network and verify the endpoint:

```bash
make probe-obd OBD_HOST=192.168.0.10 OBD_PORT=35000
```

If the adapter uses port `23`:

```bash
make probe-obd OBD_HOST=192.168.0.10 OBD_PORT=23
```

Expected success:

```text
PASS: OBD endpoint replied to PID 010C
```

If the probe passes, the app should be able to read live Mode 01 telemetry from that adapter. If the probe fails, the app will show `NO SIGNAL` or `FALLBACK SIM`, depending on `fallback_sim`.

Real-adapter checklist:

- Use an `ELM327 WiFi` adapter, not Bluetooth-only, BLE-only, HTTP, or app-specific hardware.
- Connect the Miyoo/device to the adapter's Wi-Fi network first.
- Use the correct adapter IP and TCP port in `ecu_config.ini`.
- Turn vehicle ignition on so the ECU is awake.
- Set `fallback_sim=false` when testing real hardware so failures are obvious.
- Run `make probe-obd` before relying on the in-app display.

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

Run the desktop OBD simulator server:

```bash
make run-server
make run-server OBD_HOST=127.0.0.1 OBD_PORT=35000
make run-server OBD_MODE=corrupt
make run-server OBD_MODE=drop
make run-server OBD_MODE=timeout
make run-server OBD_MODE=slow
```

Or run the simulator and app together:

```bash
make run-dev
make run-dev OBD_MODE=corrupt
make run-dev OBD_MODE=drop
make run-dev OBD_MODE=timeout
make run-dev OBD_MODE=slow
```

Simulator modes:

| Mode | Link behavior | Expected UI |
|:---|:---|:---|
| `normal` | Valid Mode 01 responses | `SERVER ONLINE`, `LAST FAULT NONE` |
| `corrupt` | Replies with malformed OBD frames | `BAD FRAME`, packet rows flagged `ANM` |
| `drop` | Accepts TCP then closes after request | `FALLBACK SIM` or `NO SIGNAL`, `LAST FAULT NO SIGNAL` |
| `timeout` | Accepts TCP but never replies | `FALLBACK SIM` or `NO SIGNAL`, `LAST FAULT TIMEOUT` |
| `slow` | Replies after the client timeout window | `FALLBACK SIM` or `NO SIGNAL`, usually `LAST FAULT TIMEOUT` |

For failure-mode testing without simulated fallback data, set `fallback_sim=false` in `ecu_config.ini` before starting the app.

Check whether an OBD endpoint is reachable and replying:

```bash
make probe-obd
make probe-obd OBD_HOST=192.168.0.10 OBD_PORT=35000
```

`PASS` means the endpoint replied to PID `010C`. `FAIL` means the app will show `FALLBACK SIM` or `NO SIGNAL`, depending on `fallback_sim`.

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
| `src/config.cpp` | Runtime `ecu_config.ini` parser |
| `src/obd_client.cpp` | Non-blocking TCP OBD client |
| `src/render.cpp` | Screens and HUD rendering |
| `src/telemetry.cpp` | Simulated OBD/packet generation and export |
| `src/input.cpp` | Input handling and navigation |
| `tools/obd_sim_server.cpp` | Desktop OBD simulator server |
| `tools/obd_probe.cpp` | Command-line OBD endpoint health check |
| `Makefile` | C++ build, run, clean, and deploy commands |
| `build/` | Generated object/dependency files |
| `bin/` | Generated `ecu-instrumenter` binary |
| `launch.sh` | OnionOS app launcher |
| `config.json` | OnionOS app metadata |
| `ecu_config.ini` | User-editable OBD connection settings |
| `assets/icon.png` | OnionOS app icon |
