# -*- coding: utf-8 -*-
import pygame
from ui.screens.base import BaseScreen
from core.state import global_state, AppScreen
from core.logger import logger
from ui.input_map import ACTION_X, ACTION_Y, UP, DOWN


class LogScreen(BaseScreen):
    def __init__(self, fonts):
        super(LogScreen, self).__init__(fonts)
        self.scroll_y   = 0
        self.auto_scroll = True

    def draw(self, surface, fps=0):
        from config import settings as C
        from ui.panels import draw_top_bar, draw_tab_bar

        surface.fill(C.BG)
        draw_top_bar(surface, "SYSTEM LOGS", "", False, self.fonts)

        hint = "[X] Backup Logs  |  [Y] Clear"
        hint_surf = self.fonts.unit.render(hint, True, C.DIM)
        surface.blit(hint_surf, (C.WIDTH // 2 - hint_surf.get_width() // 2, 50))

        y_start = 75
        y_max   = C.HEIGHT - 50
        y       = y_start - self.scroll_y

        for entry in logger.entries:
            if y > y_max:
                break
            if y > 40:
                color = C.WHITE
                if   entry.type_ == "warning":    color = C.ORANGE
                elif entry.type_ == "error":      color = C.RED
                elif entry.type_ == "connection": color = C.GREEN
                elif entry.type_ == "pid":        color = C.CYAN

                type_str = entry.type_.upper()
                if   entry.type_ == "warning":    type_str = "WARN"
                elif entry.type_ == "error":      type_str = "ERR"
                elif entry.type_ == "connection": type_str = "INFO"
                elif entry.type_ == "pid":        type_str = "DATA"

                ts  = int(entry.timestamp)
                m   = (ts // 60) % 60
                s   = ts % 60
                msg = "[14:{0:02d}:{1:02d}]   {2}:   {3}".format(m, s, type_str, entry.message)
                surface.blit(self.fonts.unit.render(msg, True, color), (C.PAD, y))
            y += 25

        if self.auto_scroll:
            self.scroll_y = max(0, len(logger.entries) * 25 - (y_max - y_start))

        draw_tab_bar(surface, 1, self.fonts)

    def handle_event(self, event):
        """Handles log-specific actions only.
        Tab switching (Left/Right, L/R shoulder) is delegated to app.py."""
        if event.type != pygame.KEYDOWN:
            return False

        from config import settings as C
        y_start = 50
        y_max   = C.HEIGHT - 50
        key     = event.key

        if key in ACTION_Y:            # Y = clear log
            logger.clear()
            self.scroll_y = 0
            return True

        if key in ACTION_X:            # X = export log
            self._export_log()
            return True

        if key in UP:
            self.scroll_y = max(0, self.scroll_y - 25)
            self.auto_scroll = False
            return True

        if key in DOWN:
            limit = max(0, len(logger.entries) * 25 - (y_max - y_start))
            self.scroll_y = min(limit, self.scroll_y + 25)
            if self.scroll_y >= limit:
                self.auto_scroll = True
            return True

        return False

    def _export_log(self):
        try:
            with open("ecu_log.txt", "w") as f:
                for e in logger.entries:
                    f.write("[{0:.1f}] {1}: {2}\n".format(e.timestamp, e.type_, e.message))
            logger.log("info", "Log exported.")
        except Exception:
            pass
