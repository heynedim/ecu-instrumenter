# -*- coding: utf-8 -*-
"""
Central input mapping for ECU Instrumenter.

Miyoo Mini Plus physical buttons map to the following pygame keys:
  A      → K_z       (confirm / click)
  B      → K_b       (cancel / back — only in loading screen)
  X      → K_LSHIFT  (action X: export log, clear faults)
  Y      → K_y       (action Y: clear log)
  L      → K_e / K_q / K_PAGEUP   (previous tab)
  R      → K_t / K_w / K_PAGEDOWN (next tab)
  Start  → K_RCTRL
  Select → K_LALT
  D-Pad  → K_UP, K_DOWN, K_LEFT, K_RIGHT

MacBook equivalents:
  Confirm        → Space / Return / KP_Enter
  Cancel         → Escape / Backspace
  Tab Prev       → Left Arrow  (when no editable widget is focused)
  Tab Next       → Right Arrow (when no editable widget is focused)
  Value Adjust   → Left / Right Arrow (when an editable widget IS focused)
"""
import pygame

# ── Confirm / A button ────────────────────────────────────────────────────────
CONFIRM = (pygame.K_SPACE, pygame.K_RETURN, pygame.K_KP_ENTER, pygame.K_z)

# ── Cancel / B button (only used in loading screen to abort connection) ───────
CANCEL  = (pygame.K_ESCAPE, pygame.K_b, pygame.K_BACKSPACE, pygame.K_LALT)

# ── Action buttons ─────────────────────────────────────────────────────────────
ACTION_X = (pygame.K_LSHIFT,)          # X: export log / clear faults
ACTION_Y = (pygame.K_y,)               # Y: clear log

# ── Tab navigation ─────────────────────────────────────────────────────────────
# Miyoo L/R shoulder → always switch tabs
# Mac Left/Right → switch tabs ONLY when current screen's handle_event returns False
TAB_PREV = (pygame.K_e, pygame.K_q, pygame.K_PAGEUP)    # Miyoo L shoulder
TAB_NEXT = (pygame.K_t, pygame.K_w, pygame.K_PAGEDOWN)  # Miyoo R shoulder

# ── D-Pad / Directional ────────────────────────────────────────────────────────
UP    = (pygame.K_UP,)
DOWN  = (pygame.K_DOWN,)
LEFT  = (pygame.K_LEFT,)
RIGHT = (pygame.K_RIGHT,)
