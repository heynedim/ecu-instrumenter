#include "app.h"

void enterMenu() {
    if (menuIndex == 0) screenId = SCREEN_LIVE;
    else if (menuIndex == 1) screenId = SCREEN_FAULTS;
    else if (menuIndex == 2) screenId = SCREEN_PACKET;
    else screenId = SCREEN_SYSTEM;
}

void action(SDL_Keycode key) {
    bool up = key == SDLK_UP;
    bool down = key == SDLK_DOWN;
    bool a = key == SDLK_z || key == SDLK_SPACE || key == SDLK_RETURN;
    bool b = key == SDLK_b || key == SDLK_ESCAPE;
    bool start = key == SDLK_TAB;
    bool select = key == SDLK_BACKSPACE;
    bool x = key == SDLK_x || key == SDLK_LSHIFT;
    bool y = key == SDLK_y;
    if (screenId == SCREEN_BOOT && (a || start)) screenId = SCREEN_MENU;
    else if (start) overlay = !overlay;
    else if (select) screenId = SCREEN_PACKET;
    else if (screenId == SCREEN_MENU) {
        if (up) menuIndex = (menuIndex + 3) % 4;
        else if (down) menuIndex = (menuIndex + 1) % 4;
        else if (a || key == SDLK_RIGHT) enterMenu();
        else if (b) running = false;
    } else {
        if (b) screenId = SCREEN_MENU;
        if (screenId == SCREEN_FAULTS && up && !dtcs.empty()) {
            selectedDtc = (selectedDtc + (int)dtcs.size() - 1) % (int)dtcs.size();
        }
        if (screenId == SCREEN_FAULTS && down && !dtcs.empty()) {
            selectedDtc = (selectedDtc + 1) % (int)dtcs.size();
        }
        if (screenId == SCREEN_FAULTS && x && !dtcs.empty()) {
            dtcs.erase(dtcs.begin() + selectedDtc);
            if (selectedDtc >= (int)dtcs.size()) selectedDtc = (int)dtcs.size() - 1;
            if (selectedDtc < 0) selectedDtc = 0;
            lastDtcClear = SDL_GetTicks();
        }
        if (screenId == SCREEN_FAULTS && y && !dtcs.empty()) {
            dtcs.clear();
            selectedDtc = 0;
            lastDtcClear = SDL_GetTicks();
        }
        if (screenId == SCREEN_PACKET && a) {
            packetPaused = !packetPaused;
            if (packetPaused) pausedPackets = packets;
        }
        if (screenId == SCREEN_PACKET && x) exportPackets();
    }
}
