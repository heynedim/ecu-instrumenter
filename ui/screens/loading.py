# -*- coding: utf-8 -*-
import pygame
import time
from ui.screens.base import BaseScreen
from core.state import global_state, AppScreen
from ui.input_map import CANCEL


class LoadingScreen(BaseScreen):
    """Shown while a connection attempt is in progress.

    The only user action available is CANCEL (B button / Escape) to abort
    the connection and return to the connection screen.
    """

    def __init__(self, fonts):
        super(LoadingScreen, self).__init__(fonts)
        self.start_time  = None
        self.failed_time = None

    def on_enter(self):
        self.start_time  = time.time()
        self.failed_time = None

    def update(self, dt):
        if self.start_time is None:
            self.start_time  = time.time()
            self.failed_time = None

        if global_state.connection_status in ("failed", "disconnected"):
            if self.failed_time is None:
                self.failed_time = time.time()
            elif time.time() - self.failed_time > 2.0:
                global_state.screen = AppScreen.CONNECTION
                self.start_time  = None
                self.failed_time = None
        elif time.time() - self.start_time >= 1.0:
            if global_state.connection_status == "connected":
                global_state.screen = AppScreen.DASHBOARD
                self.start_time = None

    def draw(self, surface, fps=0):
        from config import settings as C
        surface.fill(C.BG)

        if global_state.connection_status in ("failed", "disconnected"):
            msg  = "CONNECTION FAILED!"
            text = self.fonts.value.render(msg, True, C.RED)
            surface.blit(text, (C.WIDTH // 2 - text.get_width() // 2,
                                C.HEIGHT // 2 - text.get_height() // 2))
            hint = self.fonts.label.render("Check Settings or ECU Power", True, C.DIM)
            surface.blit(hint, (C.WIDTH // 2 - hint.get_width() // 2,
                                C.HEIGHT // 2 + 30))
        else:
            dots = "." * (int(time.time() * 3) % 4)
            msg  = "Connecting to ECU" + dots
            text = self.fonts.value.render(msg, True, C.CYAN)
            surface.blit(text, (C.WIDTH // 2 - text.get_width() // 2,
                                C.HEIGHT // 2 - text.get_height() // 2))
            hint = self.fonts.label.render("Press B to cancel", True, C.DIM)
            surface.blit(hint, (C.WIDTH // 2 - hint.get_width() // 2,
                                C.HEIGHT // 2 + 30))

    def handle_event(self, event):
        """Only CANCEL (B / Escape) is accepted to abort the connection."""
        if event.type == pygame.KEYDOWN and event.key in CANCEL:
            from core.obd_client import obd_client
            obd_client.stop()
            global_state.connection_status = "disconnected"
            global_state.screen = AppScreen.CONNECTION
            self.start_time  = None
            self.failed_time = None
            return True
        return False
