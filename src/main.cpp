#include "app.h"

#include <cstdlib>
#include <ctime>

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    std::srand((unsigned int)std::time(NULL));
    tel.rpm = 820;
    tel.speed = 0;
    tel.coolant = 83;
    tel.intake = 31;
    tel.throttle = 4;
    tel.load = 18;
    tel.voltage = 14.1f;
    tel.stft = 1.2f;
    tel.ltft = -0.4f;
    tel.latency = 16;
    dtcs.push_back(Dtc{"P0133", "O2 SENSOR SLOW RESPONSE", "STORED", 184231, 2140, 61});
    dtcs.push_back(Dtc{"P0301", "CYLINDER 1 MISFIRE", "PENDING", 184238, 910, 22});
    dtcs.push_back(Dtc{"P0420", "CATALYST EFFICIENCY", "STORED", 184240, 2660, 54});
    dtcs.push_back(Dtc{"U0100", "ECM COMMS LOST", "HISTORY", 184244, 0, 0});
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) != 0) return 1;
    win = SDL_CreateWindow("ECU-INSTRUMENTER", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
    if (!win) return 1;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!ren) return 1;
    SDL_RenderSetLogicalSize(ren, W, H);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    bootStart = SDL_GetTicks();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            else if (e.type == SDL_KEYDOWN && !e.key.repeat) action(e.key.keysym.sym);
        }
        unsigned int now = SDL_GetTicks();
        updateTelemetry(now);
        if (screenId == SCREEN_BOOT && now - bootStart > BOOT_SIGNAL_MS) screenId = SCREEN_MENU;
        if (screenId == SCREEN_BOOT) drawBoot(now);
        else if (screenId == SCREEN_MENU) drawMenu(now);
        else if (screenId == SCREEN_LIVE) drawLive(now);
        else if (screenId == SCREEN_FAULTS) drawFaults();
        else if (screenId == SCREEN_PACKET) drawPacket(now);
        else if (screenId == SCREEN_SYSTEM) drawSystem(now);
        if (overlay) drawOverlay(now);
        scanlines();
        SDL_RenderPresent(ren);
        ++frame;
        SDL_Delay(1);
    }
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
