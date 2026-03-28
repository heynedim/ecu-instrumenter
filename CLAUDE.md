# ECU Instrumenter — Claude Project Rules

This file defines the project conventions, architecture rules, and coding standards
for Claude (and any AI assistant) working on this codebase.

---

## Project Identity

- **Name:** ECU Instrumenter
- **Target hardware:** Miyoo Mini Plus (OnionOS, Python 2.7 ARM)
- **Dev platform:** macOS / Linux (Python 3)
- **UI framework:** PyGame (640×480, 30 FPS)
- **Language:** Python — must stay compatible with **both Python 2.7 and Python 3**

---

## Python Compatibility Rules

These are non-negotiable for Miyoo deployment:

- Always include `# -*- coding: utf-8 -*-` at the top of every `.py` file
- Use `from __future__ import division` in any file that does division arithmetic
- Use `object` as explicit base class: `class Foo(object):` — not `class Foo:`
- Use `super(ClassName, self).__init__(...)` — not `super().__init__(...)`
- Use `.format()` for string formatting — **never** f-strings (`f"..."`)
- Use `"".join(...)` not `f"{''.join(...)}"` patterns
- No walrus operator (`:=`), no type hints, no `match` statements
- `print` must be used as a function: `print("text")` — always (already valid in both)
- Avoid standard library modules that may be absent on Miyoo's stripped Python 2.7 build

---

## Architecture Rules

### Screen Contract

Every screen must inherit `BaseScreen` from `ui/screens/base.py`:

```python
class MyScreen(BaseScreen):
    def draw(self, surface, fps=0): ...
    def update(self, dt): ...          # optional
    def handle_event(self, event):
        # Return True  → event consumed (stop propagation)
        # Return False → let app.py handle global keys
        return False
```

**Never** switch `global_state.screen` from inside `draw()`.
**Never** do tab switching inside a screen's `handle_event` — that is `app.py`'s responsibility.

### Tab Switching

Tab switching lives exclusively in `app.py → _switch_tab(direction)`.

Tab order: `DASHBOARD(0) → LOGS(1) → ERRORS(2) → SETTINGS(3)` — wraps around.

Screens that are **not** tab screens (connection, loading) must return `True` for any
key they handle (to block leaking into `_switch_tab`), or `False` for keys they
intentionally ignore.

### Key Bindings

All key constants live in **`ui/input_map.py`** — the single source of truth.

- **Never** hardcode `pygame.K_*` tuples inline inside screen files.
- Import from `ui.input_map` instead: `from ui.input_map import CONFIRM, UP, DOWN`
- If a new binding is needed, add it to `input_map.py` first, then import it.

### Widget System

- All widgets inherit `Widget` from `ui/widgets.py`.
- `MenuList.handle_event` consumes Left/Right **only** for non-`ButtonWidget` items.
  This allows Left/Right to fall through to `app.py` for tab switching when a button row
  is selected.
- The `editing` mode flag has been removed — do **not** re-introduce it.
- Widget `draw(surface, x, y, width, is_selected, fonts)` — signature is fixed; do not alter.

---

## Control Scheme (Do Not Change Without Updating README)

| Action | Miyoo Button | Key(s) |
|--------|-------------|--------|
| Tab previous | L shoulder | `e`, `q`, PageUp |
| Tab next | R shoulder | `t`, `w`, PageDown |
| Tab prev (Mac only) | ← | K_LEFT (when widget returns False) |
| Tab next (Mac only) | → | K_RIGHT (when widget returns False) |
| Confirm / A | A | `z`, Space, Return, KP_Enter |
| Cancel / B | B | `b`, Escape, Backspace, LAlt |
| Action X | X | Left Shift |
| Action Y | Y | `y` |
| Navigate up | D-Pad Up | ↑ |
| Navigate down | D-Pad Down | ↓ |
| Adjust value | D-Pad Left/Right | ← / → (consumed by widget when editable) |

---

## File & Structure Rules

```
app.py            → main loop ONLY: event dispatch, tab switching, draw calls
config/settings.py → all display/theme constants (WIDTH, HEIGHT, BG, CYAN…)
core/state.py     → GlobalState, AppSettings, AppScreen — no UI code
core/obd_client.py → OBD TCP thread — no UI code, no direct screen switching
ui/input_map.py   → key constants ONLY
ui/panels.py      → stateless draw helpers (top bar, tab bar, cards, AFR)
ui/fonts.py       → UIFonts loader ONLY
ui/widgets.py     → Widget base + all concrete widgets + MenuList
ui/screens/base.py → BaseScreen ONLY
ui/screens/*.py   → one screen per file, no cross-screen imports
```

### What Goes Where

| Task | File |
|------|------|
| New OBD PID | `core/obd_client.py → _setup_pid_map()` |
| New theme colour | `config/settings.py` (add to ThemeSettings + alias) |
| New key binding | `ui/input_map.py` |
| New screen | New file in `ui/screens/`, inherit `BaseScreen`, register in `app.py` |
| New dashboard card | `ui/panels.py` draw helper + `ui/screens/dashboard.py` |
| New widget type | `ui/widgets.py`, inherit `Widget`, add to `_EDITABLE_TYPES` if applicable |

---

## State Management

- All mutable app state lives on `global_state` (singleton from `core/state.py`).
- Settings are persisted to `settings.json` via `global_state.save()`.
- Screens read from `global_state` directly — do not pass state as constructor arguments.
- `global_state.save()` must be called whenever settings change (e.g., in `_on_exit_to_menu`).
- Never store references to `global_state.telemetry` — always access via `global_state.telemetry.attr`.

---

## Naming Conventions

| Item | Convention | Example |
|------|------------|---------|
| Screen classes | `PascalCase + Screen` | `DashboardScreen` |
| Widget classes | `PascalCase + Widget` | `SliderWidget` |
| Screen files | `snake_case.py` | `dashboard.py` |
| Input map constants | `UPPER_SNAKE` | `TAB_PREV`, `ACTION_X` |
| Config constants | `UPPER_SNAKE` | `C.WIDTH`, `C.CYAN` |
| Private methods | `_snake_case` | `_save_settings()` |
| Draw helpers in panels | `draw_*` | `draw_top_bar()`, `draw_card()` |

---

## Drawing Rules

- `config/settings.py` is imported locally inside `draw()` as `from config import settings as C`.
  **Never** import `C` at module level in screen files — it causes issues on device.
- All screen `draw()` methods must accept `fps=0` as a default keyword argument.
- Draw order: `surface.fill(C.BG)` → top bar → content → tab bar. Always.
- Tab bar index: Dashboard=0, Logs=1, Errors=2, Settings=3.
- The tab bar height is `35px`; reserve it at the bottom (`C.HEIGHT - 35`).

---

## Performance Rules (Miyoo has ~128 MB RAM, slow CPU)

- Do **not** create new `pygame.Surface` objects every frame — cache them.
- Do **not** call `fonts.render()` for static strings inside `draw()` on hot paths; pre-render where possible.
- Keep the logging level at `CRITICAL` (`logging.getLogger().setLevel(logging.CRITICAL)`) — Miyoo's Python 2.7 emits spurious warnings otherwise.
- History logging writes to disk every 5 seconds maximum — never per-frame.
- `OBDClient` polling runs in a daemon thread; never call blocking I/O from the main thread.

---

## Deployment

```bash
make check          # syntax check all .py files
make run            # run locally (Python 3)
make deploy         # rsync to Miyoo at 192.168.1.53
make deploy MIYOO_IP=x.x.x.x  # custom IP
```

Before any PR or deploy: **`make check` must pass with zero errors.**
