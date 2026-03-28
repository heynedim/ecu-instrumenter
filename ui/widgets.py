# -*- coding: utf-8 -*-
from __future__ import division
import pygame
from core.logger import logger


class Widget(object):
    def draw(self, surface, x, y, width, is_selected, fonts): pass
    def handle_left(self): pass
    def handle_right(self): pass
    def handle_click(self): pass


class IpInputWidget(Widget):
    def __init__(self, label, value):
        self.label = label
        self.value = value
        self.parts = [int(p) for p in value.split(".")]
        self.selected_part = 3

    def draw(self, surface, x, y, width, is_selected, fonts):
        from config import settings as C
        color = C.CYAN if is_selected else C.WHITE
        lbl = fonts.button.render(self.label, True, C.DIM)
        surface.blit(lbl, (x, y))

        val_x = x + 200
        for i, p in enumerate(self.parts):
            p_color = C.CYAN if (is_selected and self.selected_part == i) else color
            txt = fonts.button.render(str(p), True, p_color)
            surface.blit(txt, (val_x, y))
            if is_selected and self.selected_part == i:
                pygame.draw.rect(surface, p_color, (val_x, y + 22, txt.get_width(), 3))
            val_x += txt.get_width()
            if i < 3:
                dot = fonts.button.render(".", True, C.WHITE)
                surface.blit(dot, (val_x, y))
                val_x += dot.get_width()

    def handle_left(self):
        self.parts[self.selected_part] = max(0, self.parts[self.selected_part] - 1)
        self.value = ".".join([str(p) for p in self.parts])

    def handle_right(self):
        self.parts[self.selected_part] = min(255, self.parts[self.selected_part] + 1)
        self.value = ".".join([str(p) for p in self.parts])

    def handle_click(self):
        # Cycle through segments with A button
        self.selected_part = (self.selected_part + 1) % 4


class PortInputWidget(Widget):
    def __init__(self, label, value):
        self.label = label
        self.value = value
        self.parts = [int(p) for p in "%05d" % value]
        self.selected_part = 4

    def draw(self, surface, x, y, width, is_selected, fonts):
        from config import settings as C
        color = C.CYAN if is_selected else C.WHITE
        lbl = fonts.button.render(self.label, True, C.DIM)
        surface.blit(lbl, (x, y))

        val_x = x + 200
        for i, p in enumerate(self.parts):
            p_color = C.CYAN if (is_selected and self.selected_part == i) else color
            txt = fonts.button.render(str(p), True, p_color)
            surface.blit(txt, (val_x, y))
            if is_selected and self.selected_part == i:
                pygame.draw.rect(surface, p_color, (val_x, y + 22, txt.get_width(), 3))
            val_x += txt.get_width() + 2

    def handle_left(self):
        self.parts[self.selected_part] = (self.parts[self.selected_part] - 1) % 10
        self.value = int("".join([str(p) for p in self.parts]))

    def handle_right(self):
        self.parts[self.selected_part] = (self.parts[self.selected_part] + 1) % 10
        self.value = int("".join([str(p) for p in self.parts]))

    def handle_click(self):
        # Cycle through digit positions with A button
        self.selected_part = (self.selected_part + 1) % 5


class ToggleWidget(Widget):
    def __init__(self, label, value):
        self.label = label
        self.value = value

    def draw(self, surface, x, y, width, is_selected, fonts):
        from config import settings as C
        color = C.CYAN if is_selected else C.WHITE
        lbl = fonts.button.render(self.label, True, C.DIM)
        surface.blit(lbl, (x, y))
        val = fonts.button.render("ON" if self.value else "OFF", True, color)
        surface.blit(val, (x + 200, y))

    def handle_left(self):  self.value = not self.value
    def handle_right(self): self.value = not self.value
    def handle_click(self): self.value = not self.value


class SliderWidget(Widget):
    def __init__(self, label, value, min_val, max_val):
        self.label = label
        self.value = value
        self.min_val = min_val
        self.max_val = max_val

    def draw(self, surface, x, y, width, is_selected, fonts):
        from config import settings as C
        color = C.CYAN if is_selected else C.WHITE
        lbl = fonts.button.render(self.label, True, C.DIM)
        surface.blit(lbl, (x, y))

        bar_x = x + 180
        bar_w = width - 240
        pygame.draw.rect(surface, C.BORDER, (bar_x, y + 15, bar_w, 4))

        ratio = float(self.value - self.min_val) / max(1, self.max_val - self.min_val)
        knob_x = bar_x + int(ratio * bar_w)
        pygame.draw.circle(surface, color, (knob_x, y + 17), 10)

        val_txt = fonts.button.render(str(self.value), True, color)
        surface.blit(val_txt, (bar_x + bar_w + 15, y))

    def handle_left(self):  self.value = max(self.min_val, self.value - 5)
    def handle_right(self): self.value = min(self.max_val, self.value + 5)


class ButtonWidget(Widget):
    def __init__(self, label, on_click):
        self.label = label
        self.on_click = on_click

    def draw(self, surface, x, y, width, is_selected, fonts):
        from config import settings as C
        border_color = C.CYAN if is_selected else C.BORDER
        bg_color     = C.CYAN if is_selected else C.PANEL
        rect = pygame.Rect(x, y, width, 40)
        pygame.draw.rect(surface, bg_color, rect)
        pygame.draw.rect(surface, border_color, rect, 2)
        txt = fonts.button.render(self.label, True, C.WHITE)
        surface.blit(txt, (x + width // 2 - txt.get_width() // 2,
                           y + 20 - txt.get_height() // 2))

    def handle_click(self):
        if self.on_click:
            self.on_click()


# Widget types that support Left/Right value adjustment
_EDITABLE_TYPES = (SliderWidget, IpInputWidget, PortInputWidget, ToggleWidget)


class MenuList(object):
    """Vertical menu with keyboard/gamepad navigation.

    Left/Right keys are consumed when the focused item is an editable widget
    (Slider, IP, Port, Toggle).  For ButtonWidget they are NOT consumed, so
    app.py can use them for tab-switching.

    A (CONFIRM) on IP/Port cycles the active digit segment.
    A (CONFIRM) on Slider/Toggle does an immediate adjustment (same as Right).
    A (CONFIRM) on ButtonWidget fires the button's callback.
    """

    def __init__(self, items, fonts):
        self.items = items
        self.fonts = fonts
        self.selected_idx = 0

    def draw(self, surface, x, y, item_height=50, width=300):
        for i, item in enumerate(self.items):
            item.draw(surface, x, y + i * item_height, width,
                      i == self.selected_idx, self.fonts)

    def handle_event(self, event):
        if not self.items:
            return False

        if event.type != pygame.KEYDOWN:
            return False

        from ui.input_map import CONFIRM, UP, DOWN, LEFT, RIGHT
        key  = event.key
        item = self.items[self.selected_idx]

        # ── Navigation ────────────────────────────────────────────────────────
        if key in UP:
            if self.selected_idx > 0:
                self.selected_idx -= 1
            return True

        if key in DOWN:
            if self.selected_idx < len(self.items) - 1:
                self.selected_idx += 1
            return True

        # ── Value adjustment: Left / Right ────────────────────────────────────
        # Only consume for editable widgets; pass through for ButtonWidget
        # so app.py can handle tab switching via arrow keys on Mac.
        if key in LEFT:
            if isinstance(item, _EDITABLE_TYPES):
                item.handle_left()
                return True
            return False   # ButtonWidget → let app.py switch tab

        if key in RIGHT:
            if isinstance(item, _EDITABLE_TYPES):
                item.handle_right()
                return True
            return False   # ButtonWidget → let app.py switch tab

        # ── Confirm / A button ────────────────────────────────────────────────
        if key in CONFIRM:
            item.handle_click()
            return True

        return False
