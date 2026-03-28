# -*- coding: utf-8 -*-

class BaseScreen(object):
    """Lightweight base class for all UI screens.

    Contract:
    - draw(surface, fps=0)   → render the screen
    - update(dt)             → per-frame logic (optional)
    - handle_event(event)    → return True if event consumed, False to let
                               app.py handle global keys (tab switching etc.)
    """

    def __init__(self, fonts):
        self.fonts = fonts

    def draw(self, surface, fps=0):
        pass

    def update(self, dt):
        pass

    def handle_event(self, event):
        return False
