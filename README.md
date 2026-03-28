# ECU Instrumenter

> **v1.0** — High-Performance OBD-II Dashboard for Miyoo Mini Plus

ECU Instrumenter is a lightweight telemetry dashboard designed for the **Miyoo Mini Plus (OnionOS)**. It connects wirelessly to any standard ELM327 Wi-Fi OBD-II adapter over TCP and turns your retro handheld into a real-time vehicle instrument cluster.

![ECU Instrumenter running on Miyoo Mini Plus](assets/in-miyoo.png)

---

## Features

- **Wireless OBD-II** — Polls ELM327 TCP packets for RPM, Speed, Throttle, Coolant, Oil Temp, and AFR
- **Demo Mode** — Full UI simulation with mathematical engine data, no car required
- **Telemetry History** — Enable `SAVE HISTORY` to append JSON-Lines records every 5 seconds to `history.log`
- **DTC Fault Codes** — Live engine error code display with plain-English descriptions
- **System Logs** — Colour-coded in-app log viewer with export to `ecu_log.txt`
- **Cross-Platform** — Runs on Miyoo Mini Plus (OnionOS / Python 2.7) and macOS/Linux (Python 3)

---

## Control Scheme

### Tab Navigation

| | Previous Tab | Next Tab |
|---|---|---|
| **Miyoo Mini Plus** | L shoulder | R shoulder |
| **MacBook / Desktop** | ← Left Arrow | → Right Arrow |

Tab order: `DASHBOARD → LOGS → ERRORS → SETTINGS` (wraps around)

### Universal Controls

| Button | Miyoo | Mac | Action |
|--------|-------|-----|--------|
| A | `z` | Space / Enter | Confirm / cycle digit (IP, Port) |
| B | `b` | Escape / Backspace | Cancel (loading screen only) |
| X | Left Shift | Left Shift | Export log / clear faults |
| Y | `y` | `y` | Clear log |
| D-Pad Up/Down | ↑ / ↓ | ↑ / ↓ | Navigate menu rows |
| D-Pad Left/Right | ← / → | ← / → | Adjust value (when an editable setting is focused) |

> **Note:** Left/Right arrows on the Mac only switch tabs when the currently focused settings row is a button (not an editable value). When a Slider, IP, Port, or Toggle is selected, Left/Right adjusts the value instead.

### Settings Screen

| Action | Result |
|--------|--------|
| Navigate to **EXIT TO MAIN MENU** → A | Save settings, stop connection, return to connection screen |
| Navigate to **EXIT APPLICATION** → A | Save settings and quit |

---

## Running Locally (macOS / Desktop)

```bash
pip3 install pygame
make run
```

Start the mock ELM327 server to simulate live data over localhost:

```bash
python3 mock_server.py &
make run
```

---

## Deploying to Miyoo Mini Plus

```bash
make deploy                        # default IP: 192.168.1.53
make deploy MIYOO_IP=192.168.1.54  # override IP
```

Requires the Miyoo to be on the same Wi-Fi network with SSH enabled (OnionOS default). After deployment, launch **ECU Instrumenter** from the Apps tab.

---

## Project Structure

```
ecu-instrumenter/
├── app.py                  # Main loop, tab switching, global key handling
├── config/settings.py      # Display, theme, and threshold constants
├── core/
│   ├── obd_client.py       # Threaded ELM327 TCP client
│   ├── state.py            # GlobalState, AppSettings, AppScreen
│   └── logger.py           # In-memory logger (shown on Logs screen)
├── models/telemetry.py     # TelemetryFrame dataclass
├── sim/simulator.py        # Demo-mode data simulator
├── ui/
│   ├── input_map.py        # Central key bindings (Miyoo ↔ Mac)
│   ├── fonts.py            # UIFonts loader
│   ├── panels.py           # Shared draw helpers (top bar, tab bar, cards)
│   ├── widgets.py          # MenuList, Slider, IP/Port, Toggle, Button widgets
│   └── screens/
│       ├── base.py         # BaseScreen interface
│       ├── connection.py   # Startup / connection choice screen
│       ├── loading.py      # Connection progress screen
│       ├── dashboard.py    # Live telemetry dashboard
│       ├── logs.py         # System log viewer
│       ├── errors.py       # DTC fault code screen
│       └── settings.py     # App settings screen
├── mock_server.py          # Simulated ELM327 server (localhost dev)
└── Makefile                # run / deploy / check targets
```

---

## Adding New PIDs

Open `core/obd_client.py` and add an entry to `_setup_pid_map()`:

```python
"01XX": {
    "attr": "my_value",          # attribute set on global_state.telemetry
    "parser": lambda d: ...,     # receives hex byte strings [A, B, ...]
    "expected_len": 1,           # minimum bytes expected in response
},
```

The value becomes available as `global_state.telemetry.my_value` across all screens instantly.

---

## Quality Check

```bash
make check   # compiles all .py files for syntax errors
```

Contributions and PRs are welcome — please run `make check` before submitting.
