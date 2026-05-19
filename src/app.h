#ifndef ECU_INSTRUMENTER_APP_H
#define ECU_INSTRUMENTER_APP_H

#include <SDL.h>

#include <string>
#include <vector>

const int W = 640;
const int H = 480;
const unsigned int BOOT_SIGNAL_MS = 3000;
const size_t MAX_HISTORY = 96;
const size_t MAX_PACKETS = 120;
const int PACKET_ROWS = 18;

struct Color {
    Uint8 r, g, b, a;
};

struct Telemetry {
    int rpm;
    int speed;
    int coolant;
    int intake;
    int throttle;
    int load;
    float voltage;
    float stft;
    float ltft;
    int latency;
};

struct Packet {
    unsigned int tick;
    std::string req;
    std::string res;
    int latency;
    bool anomaly;
};

struct Dtc {
    std::string code;
    std::string desc;
    std::string status;
    int mileage;
    int rpm;
    int load;
};

enum Screen {
    SCREEN_BOOT,
    SCREEN_MENU,
    SCREEN_LIVE,
    SCREEN_FAULTS,
    SCREEN_PACKET,
    SCREEN_SYSTEM
};

extern SDL_Window *win;
extern SDL_Renderer *ren;
extern Screen screenId;
extern int menuIndex;
extern bool running;
extern bool overlay;
extern bool packetPaused;
extern unsigned int bootStart;
extern unsigned int lastTelemetry;
extern unsigned int frame;
extern Telemetry tel;
extern std::vector<int> rpmHist;
extern std::vector<int> latencyHist;
extern std::vector<int> coolantHist;
extern std::vector<int> voltageHist;
extern std::vector<Packet> packets;
extern std::vector<Packet> pausedPackets;
extern std::vector<Dtc> dtcs;
extern int selectedDtc;
extern unsigned int packetSeq;
extern unsigned int lastDtcClear;

extern const Color BG;
extern const Color PANEL;
extern const Color PANEL2;
extern const Color LINE;
extern const Color TEXT;
extern const Color MUTED;
extern const Color GREEN;
extern const Color AMBER;
extern const Color CYAN;
extern const Color RED;

void color(Color c);
void rect(int x, int y, int w, int h, Color c, bool fill);
void line(int x1, int y1, int x2, int y2, Color c);
const unsigned char *glyph(char ch);
void text(int x, int y, const std::string &s, Color c, int scale);
std::string fmt(const char *label, int value, const char *unit);
std::string fmtf(const char *label, float value, const char *unit);
void scanlines();
void graph(int x, int y, int w, int h, const std::vector<int> &v, int lo, int hi, Color c, const char *unit, int divisor);
void graphBars(int x, int y, int w, int h, const std::vector<int> &v, int lo, int hi, Color c);
void frameShell(const std::string &title, const std::string &right);
void panel(int x, int y, int w, int h, const std::string &label, Color c);
void updateTelemetry(unsigned int now);
void drawBoot(unsigned int now);
void drawMenu(unsigned int now);
void drawLive(unsigned int now);
void drawFaults();
void drawPacket(unsigned int now);
void drawSystem(unsigned int now);
void drawOverlay(unsigned int now);
void exportPackets();
void enterMenu();
void action(SDL_Keycode key);

#endif
