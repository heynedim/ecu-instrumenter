# -*- coding: utf-8 -*-
from __future__ import division
import pygame
from ui.screens.base import BaseScreen
from core.state import global_state, AppScreen


class DashboardScreen(BaseScreen):
    def __init__(self, fonts):
        super(DashboardScreen, self).__init__(fonts)
        from sim.simulator import Simulator
        self.simulator = Simulator()

    def update(self, dt):
        if global_state.demo_mode:
            global_state.telemetry = self.simulator.update(dt)

    def handle_event(self, event):
        """Dashboard only handles the debug demo-mode toggle (D key).
        Tab switching (Left/Right, L/R shoulder) is handled by app.py."""
        if event.type == pygame.KEYDOWN:
            if event.key == pygame.K_d:
                global_state.demo_mode = not global_state.demo_mode
                global_state.save()
                return True
        return False

    def draw(self, surface, fps=0):
        from config import settings as C
        from ui.panels import draw_top_bar, draw_tab_bar, draw_card, draw_afr_full

        surface.fill(C.BG)
        draw_top_bar(surface, "DASHBOARD", "OBDII SYNCED", True, self.fonts)

        t = global_state.telemetry
        p = C.PAD
        gap = p

        # Row 1 — RPM + Speed
        r1_y = 50
        r1_h = 140
        w_total = C.WIDTH - (2 * p)
        w_rpm = int(w_total * 0.6)
        w_spd = w_total - w_rpm - gap

        rpm_rect = pygame.Rect(p, r1_y, w_rpm, r1_h)
        draw_card(surface, rpm_rect, "ENGINE RPM", "{0:.0f}".format(t.rpm),
                  "RPM", self.fonts.huge, C.WHITE, self.fonts)

        spd_val  = t.speed * 0.621371 if getattr(global_state.settings, 'is_mph', True) else t.speed
        spd_unit = "MPH" if getattr(global_state.settings, 'is_mph', True) else "KM/H"
        spd_rect = pygame.Rect(p + w_rpm + gap, r1_y, w_spd, r1_h)
        draw_card(surface, spd_rect, "SPEED", "{0:.0f}".format(spd_val),
                  spd_unit, self.fonts.huge, C.WHITE, self.fonts)

        # Row 2 — Throttle / Oil / Coolant
        r2_y    = r1_y + r1_h + gap
        r2_h    = 100
        w_third = (w_total - 2 * gap) // 3

        thr_rect = pygame.Rect(p, r2_y, w_third, r2_h)
        draw_card(surface, thr_rect, "THROTTLE", "{0:.0f}".format(t.throttle),
                  "%", self.fonts.value, C.WHITE, self.fonts,
                  progress_ratio=(t.throttle / 100.0))

        oil_rect  = pygame.Rect(p + w_third + gap, r2_y, w_third, r2_h)
        oil_temp  = t.coolant + 20
        is_f      = getattr(global_state.settings, 'is_fahrenheit', False)
        oil_unit  = "°F" if is_f else "°C"
        oil_v     = oil_temp * 9 / 5 + 32 if is_f else oil_temp
        oil_warn  = getattr(global_state.settings, 'oil_warn', 130)

        if oil_temp < 50:
            oil_color  = C.CYAN
            is_oil_hot = False
        elif oil_temp >= oil_warn:
            oil_color  = C.RED
            is_oil_hot = True
        elif oil_temp >= oil_warn - 15:
            oil_color  = C.ORANGE
            is_oil_hot = False
        else:
            oil_color  = C.WHITE
            is_oil_hot = False

        draw_card(surface, oil_rect, "OIL TEMP", "{0:.0f}".format(oil_v),
                  oil_unit, self.fonts.value, oil_color, self.fonts, alert=is_oil_hot)

        cool_rect   = pygame.Rect(p + 2 * (w_third + gap), r2_y, w_third, r2_h)
        cool_unit   = "°F" if is_f else "°C"
        cool_v      = t.coolant * 9 / 5 + 32 if is_f else t.coolant
        warn_thresh = getattr(global_state.settings, 'coolant_warn', 105)
        is_hot      = (t.coolant >= warn_thresh)
        cool_color  = C.RED if is_hot else C.ORANGE
        draw_card(surface, cool_rect, "COOLANT", "{0:.0f}".format(cool_v),
                  cool_unit, self.fonts.value, cool_color, self.fonts, alert=is_hot)

        # Row 3 — AFR
        r3_y    = r2_y + r2_h + gap
        r3_h    = C.HEIGHT - r3_y - C.PAD - 35
        afr_rect = pygame.Rect(p, r3_y, w_total, r3_h)
        draw_afr_full(surface, afr_rect, t.afr, self.fonts)

        draw_tab_bar(surface, 0, self.fonts)
