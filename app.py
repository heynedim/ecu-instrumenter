# -*- coding: utf-8 -*-
# -*- coding: utf-8 -*-
from __future__ import division
import sys
import pygame
import logging

# Suppress harmless missing hashlib warnings on the Miyoo Python 2.7 build
logging.getLogger().setLevel(logging.CRITICAL)

from config import settings as C
from core.state import global_state, AppScreen
from ui.fonts import UIFonts
from ui.input_map import TAB_PREV, TAB_NEXT, LEFT, RIGHT
from ui.screens.connection import ConnectionScreen
from ui.screens.dashboard import DashboardScreen
from ui.screens.logs import LogScreen
from ui.screens.settings import SettingsScreen
from ui.screens.loading import LoadingScreen
from ui.screens.errors import ErrorsScreen

# Tab order: 0=Dashboard, 1=Logs, 2=Errors, 3=Settings
TAB_SCREENS = [AppScreen.DASHBOARD, AppScreen.LOG, AppScreen.ERRORS, AppScreen.SETTINGS]


def _switch_tab(direction):
    """Rotate through tab screens by +1 or -1."""
    cur = global_state.screen
    if cur in TAB_SCREENS:
        idx = TAB_SCREENS.index(cur)
        global_state.screen = TAB_SCREENS[(idx + direction) % len(TAB_SCREENS)]


def main():
    pygame.init()
    pygame.display.set_caption(C.APP_NAME)
    screen = pygame.display.set_mode(
        (C.WIDTH, C.HEIGHT), pygame.HWSURFACE | pygame.DOUBLEBUF
    )
    clock = pygame.time.Clock()

    fonts = UIFonts.create()
    global_state.load()

    screens = {
        AppScreen.CONNECTION: ConnectionScreen(fonts),
        AppScreen.LOADING:    LoadingScreen(fonts),
        AppScreen.DASHBOARD:  DashboardScreen(fonts),
        AppScreen.LOG:        LogScreen(fonts),
        AppScreen.ERRORS:     ErrorsScreen(fonts),
        AppScreen.SETTINGS:   SettingsScreen(fonts),
    }

    running = True
    while running:
        dt  = clock.tick(C.FPS) / 1000.0
        fps = clock.get_fps() or float(C.FPS)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                continue

            # Let the active screen handle the event first.
            # If it returns False (didn't consume), we handle global keys.
            consumed = screens[global_state.screen].handle_event(event)

            if not consumed and event.type == pygame.KEYDOWN:
                key = event.key
                # Miyoo L/R shoulder → always switch tabs
                # Mac Left/Right arrow → switch tabs (only reached when screen
                # returned False, meaning no editable widget consumed the key)
                if key in TAB_PREV or key in LEFT:
                    _switch_tab(-1)
                elif key in TAB_NEXT or key in RIGHT:
                    _switch_tab(+1)

        # Update & draw active screen
        active = screens[global_state.screen]
        active.update(dt)
        active.draw(screen, fps)

        pygame.display.flip()

    pygame.quit()
    sys.exit(0)


if __name__ == "__main__":
    main()
