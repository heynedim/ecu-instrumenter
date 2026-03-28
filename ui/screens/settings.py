# -*- coding: utf-8 -*-
import pygame
from ui.screens.base import BaseScreen
from core.state import global_state, AppScreen
from ui.widgets import MenuList, IpInputWidget, PortInputWidget, ToggleWidget, ButtonWidget, SliderWidget


class SettingsScreen(BaseScreen):
    def __init__(self, fonts):
        super(SettingsScreen, self).__init__(fonts)

        self.ip_input       = IpInputWidget("IP ADDRESS",    getattr(global_state.settings, 'ip', '127.0.0.1'))
        self.port_input     = PortInputWidget("PORT",         getattr(global_state.settings, 'port', 35000))
        self.oil_slider     = SliderWidget("OIL ALARM",      getattr(global_state.settings, 'oil_warn', 130), 80, 150)
        self.coolant_slider = SliderWidget("COOLANT ALARM",  getattr(global_state.settings, 'coolant_warn', 105), 80, 130)
        self.mph_toggle     = ToggleWidget("USE MPH / F",    getattr(global_state.settings, 'is_mph', False))
        self.history_toggle = ToggleWidget("SAVE HISTORY",   getattr(global_state.settings, 'save_history', False))

        self.exit_menu_btn  = ButtonWidget("EXIT TO MAIN MENU", self._on_exit_to_menu)
        self.exit_app_btn   = ButtonWidget("EXIT APPLICATION",  self._on_exit_app)

        self.menu = MenuList([], self.fonts)
        self._last_demo_mode = None
        self._refresh_menu_items()

    # ── Menu item list (depends on demo mode) ─────────────────────────────────

    def _refresh_menu_items(self):
        current_demo = getattr(global_state, 'demo_mode', False)
        if self._last_demo_mode == current_demo:
            return

        self._last_demo_mode = current_demo
        items = []

        if not current_demo:
            items.extend([self.ip_input, self.port_input])

        items.extend([self.oil_slider, self.coolant_slider, self.mph_toggle])

        if not current_demo:
            items.append(self.history_toggle)

        items.extend([self.exit_menu_btn, self.exit_app_btn])

        self.menu.items = items
        if self.menu.selected_idx >= len(items):
            self.menu.selected_idx = max(0, len(items) - 1)

    # ── Actions ───────────────────────────────────────────────────────────────

    def _save_settings(self):
        old_ip   = getattr(global_state.settings, 'ip',   '127.0.0.1')
        old_port = getattr(global_state.settings, 'port', 35000)

        global_state.settings.ip          = self.ip_input.value
        global_state.settings.port        = self.port_input.value
        global_state.settings.oil_warn    = self.oil_slider.value
        global_state.settings.coolant_warn = self.coolant_slider.value
        global_state.settings.is_mph       = self.mph_toggle.value
        global_state.settings.is_fahrenheit = self.mph_toggle.value
        global_state.settings.save_history  = self.history_toggle.value
        global_state.save()

        return (old_ip != self.ip_input.value) or (old_port != self.port_input.value)

    def _on_exit_to_menu(self):
        """Save settings, stop client, return to connection screen."""
        self._save_settings()
        from core.obd_client import obd_client
        obd_client.stop()
        global_state.connection_status = "disconnected"
        global_state.screen = AppScreen.CONNECTION

    def _on_exit_app(self):
        """Save settings and quit the application."""
        self._save_settings()
        import sys
        pygame.quit()
        sys.exit(0)

    # ── Screen interface ──────────────────────────────────────────────────────

    def draw(self, surface, fps=0):
        self._refresh_menu_items()
        from config import settings as C
        from ui.panels import draw_top_bar, draw_tab_bar

        surface.fill(C.BG)
        draw_top_bar(surface, "SETTINGS", "", False, self.fonts)
        self.menu.draw(surface, C.WIDTH // 2 - 200, 70, 45, 400)

        vers = self.fonts.unit.render("v1.0 - ECU Instrumenter", True, C.DIM)
        surface.blit(vers, (C.WIDTH // 2 - vers.get_width() // 2, C.HEIGHT - 55))

        draw_tab_bar(surface, 3, self.fonts)

    def handle_event(self, event):
        """Delegate entirely to the MenuList.

        MenuList returns True for UP/DOWN/CONFIRM and for Left/Right when an
        editable widget is focused.  It returns False for Left/Right when a
        ButtonWidget is focused, letting app.py perform tab switching.

        B/Escape is intentionally ignored — use the in-menu exit buttons.
        """
        return self.menu.handle_event(event)
