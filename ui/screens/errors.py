# -*- coding: utf-8 -*-
import pygame
from ui.screens.base import BaseScreen
from core.state import global_state, AppScreen
from ui.input_map import ACTION_X, UP, DOWN

DTC_DB = {
    "P0171": "System Too Lean (Bank 1)",
    "P0172": "System Too Rich (Bank 1)",
    "P0300": "Random/Multiple Cylinder Misfire Detected",
    "P0420": "Catalyst System Efficiency Below Threshold",
    "P0442": "Evaporative Emission System Leak Detected (small leak)",
    "P0141": "O2 Sensor Heater Circuit (Bank 1 Sensor 2)",
    "P0113": "Intake Air Temperature Sensor 1 Circuit High",
    "P0102": "Mass or Volume Air Flow Circuit Low Input",
}


class ErrorsScreen(BaseScreen):
    def __init__(self, fonts):
        super(ErrorsScreen, self).__init__(fonts)
        self.scroll_y = 0

    def draw(self, surface, fps=0):
        from config import settings as C
        from ui.panels import draw_top_bar, draw_tab_bar

        surface.fill(C.BG)
        draw_top_bar(surface, "ENGINE ERROR CODES", "", False, self.fonts)

        hint = "[X] Clear Faults"
        hint_surf = self.fonts.unit.render(hint, True, C.DIM)
        surface.blit(hint_surf, (C.WIDTH // 2 - hint_surf.get_width() // 2, 50))

        dtcs = getattr(global_state.telemetry, 'dtcs', [])
        y = 75 - self.scroll_y

        if not dtcs:
            msg = self.fonts.value.render("NO FAULT CODES DETECTED", True, C.GREEN)
            surface.blit(msg, (C.WIDTH // 2 - msg.get_width() // 2,
                               C.HEIGHT // 2 - msg.get_height() // 2))
        else:
            for code in dtcs:
                desc   = DTC_DB.get(code, "Unknown Engine Code")
                c_surf = self.fonts.label.render(code, True, C.RED)
                d_surf = self.fonts.unit.render(desc, True, C.WHITE)

                pygame.draw.rect(surface, C.PANEL,  (C.PAD, y, C.WIDTH - C.PAD * 2, 60))
                pygame.draw.rect(surface, C.BORDER, (C.PAD, y, C.WIDTH - C.PAD * 2, 60), 1)
                surface.blit(c_surf, (C.PAD + 15, y + 15))
                surface.blit(d_surf, (C.PAD + 15, y + 35))
                y += 70

        draw_tab_bar(surface, 2, self.fonts)

    def handle_event(self, event):
        """Handles fault-clear and scroll only.
        Tab switching (Left/Right, L/R shoulder) is delegated to app.py."""
        if event.type != pygame.KEYDOWN:
            return False

        key = event.key

        if key in ACTION_X:            # X = clear faults
            global_state.telemetry.dtcs = []
            self.scroll_y = 0
            return True

        if key in UP:
            self.scroll_y = max(0, self.scroll_y - 30)
            return True

        if key in DOWN:
            dtcs       = getattr(global_state.telemetry, 'dtcs', [])
            max_scroll = max(0, len(dtcs) * 70 - 250)
            self.scroll_y = min(max_scroll, self.scroll_y + 30)
            return True

        return False
