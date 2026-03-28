# -*- coding: utf-8 -*-
import pygame
from ui.screens.base import BaseScreen
from core.state import global_state, AppScreen
from core.obd_client import obd_client
from ui.widgets import MenuList, ButtonWidget


class ConnectionScreen(BaseScreen):
    """Initial screen — shown before any connection is established.
    Not part of the tab rotation; navigates forward to Loading or Settings."""

    def __init__(self, fonts):
        super(ConnectionScreen, self).__init__(fonts)

        btn_connect  = ButtonWidget("CONNECT TO ECU",  self._on_connect)
        btn_demo     = ButtonWidget("START DEMO MODE", self._on_demo)
        btn_settings = ButtonWidget("SETTINGS",        self._on_settings)

        self.menu = MenuList([btn_connect, btn_demo, btn_settings], fonts)

    def _on_settings(self):
        global_state.screen = AppScreen.SETTINGS

    def _on_connect(self):
        ip   = getattr(global_state.settings, 'ip',   '127.0.0.1')
        port = getattr(global_state.settings, 'port', 35000)
        global_state.settings.was_connected = True
        global_state.demo_mode = False
        global_state.save()
        obd_client.start(ip, port)
        global_state.screen = AppScreen.LOADING

    def _on_demo(self):
        global_state.demo_mode = True
        global_state.settings.was_connected = True
        global_state.telemetry.dtcs = ["P0171", "P0300", "P0420"]
        global_state.save()
        global_state.connection_status = "connected"
        global_state.screen = AppScreen.LOADING

    def draw(self, surface, fps=0):
        from config import settings as C
        surface.fill(C.BG)

        name = self.fonts.title.render("ECU INSTRUMENTER", True, C.RED)
        surface.blit(name, (C.WIDTH // 2 - name.get_width() // 2, 80))

        self.menu.draw(surface, C.WIDTH // 2 - 200, 180, 60, 400)

        status_color = C.DIM
        if   global_state.connection_status == "failed":    status_color = C.RED
        elif global_state.connection_status == "connected": status_color = C.GREEN

        status_text = "STATUS: " + global_state.connection_status.upper()
        status = self.fonts.button.render(status_text, True, status_color)
        surface.blit(status, (C.WIDTH // 2 - status.get_width() // 2, 420))

    def handle_event(self, event):
        """The connection screen is not a tab screen.
        All navigation is MenuList-driven (UP/DOWN/CONFIRM)."""
        return self.menu.handle_event(event)
