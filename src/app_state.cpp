#include "app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

struct Glyph {
    char ch;
    unsigned char rows[7];
};

const unsigned char SPACE[7] = {0, 0, 0, 0, 0, 0, 0};
const unsigned char UNKNOWN[7] = {14, 17, 1, 2, 4, 0, 4};
const Glyph GLYPHS[] = {
    {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}}, {'C',{14,17,16,16,16,17,14}},
    {'D',{30,17,17,17,17,17,30}}, {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
    {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}}, {'I',{14,4,4,4,4,4,14}},
    {'J',{7,2,2,2,18,18,12}}, {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
    {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}}, {'O',{14,17,17,17,17,17,14}},
    {'P',{30,17,17,30,16,16,16}}, {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
    {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}}, {'U',{17,17,17,17,17,17,14}},
    {'V',{17,17,17,17,17,10,4}}, {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
    {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
    {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}}, {'2',{14,17,1,2,4,8,31}},
    {'3',{30,1,1,14,1,1,30}}, {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
    {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}}, {'8',{14,17,17,14,17,17,14}},
    {'9',{14,17,17,15,1,1,14}}, {'.',{0,0,0,0,0,12,12}}, {',',{0,0,0,0,0,4,8}},
    {':',{0,4,4,0,4,4,0}}, {';',{0,4,4,0,4,4,8}}, {'-',{0,0,0,31,0,0,0}}, {'_',{0,0,0,0,0,0,31}},
    {'/',{1,1,2,4,8,16,16}}, {'%',{24,25,2,4,8,19,3}}, {'+',{0,4,4,31,4,4,0}}, {'=',{0,0,31,0,31,0,0}},
    {'<',{2,4,8,16,8,4,2}}, {'>',{8,4,2,1,2,4,8}}, {'[',{14,8,8,8,8,8,14}}, {']',{14,2,2,2,2,2,14}},
    {'(',{2,4,8,8,8,4,2}}, {')',{8,4,2,2,2,4,8}}, {'#',{10,31,10,10,31,10,0}}, {'*',{0,21,14,31,14,21,0}},
    {'?',{14,17,1,2,4,0,4}}, {'!',{4,4,4,4,4,0,4}}, {'|',{4,4,4,4,4,4,4}}, {'@',{14,17,23,21,23,16,14}},
    {'&',{12,18,20,8,21,18,13}}, {'\'',{4,4,8,0,0,0,0}}
};

template <typename T>
void pushLimited(std::vector<T> &items, const T &item, size_t limit) {
    items.push_back(item);
    if (items.size() > limit) items.erase(items.begin());
}

} // namespace

SDL_Window *win = NULL;
SDL_Renderer *ren = NULL;
Screen screenId = SCREEN_BOOT;
int menuIndex = 0;
bool running = true;
bool overlay = false;
bool packetPaused = false;
unsigned int bootStart = 0;
unsigned int lastTelemetry = 0;
unsigned int frame = 0;
AppConfig appConfig = {"127.0.0.1", 35000, 900, 120, true};
ObdLinkState obdState = OBD_DISCONNECTED;
std::string obdStatusText = "NO SIGNAL";
std::string obdFaultText = "NONE";
std::string obdLastPid = "----";
std::string obdLastResponse = "----";
int obdLastLatency = 0;
unsigned int obdLastRx = 0;
ObdBootCheck obdBootCheck = {false, false, false, false, 0};
Telemetry tel;
std::vector<int> rpmHist;
std::vector<int> latencyHist;
std::vector<int> coolantHist;
std::vector<int> voltageHist;
std::vector<Packet> packets;
std::vector<Packet> pausedPackets;
std::vector<Dtc> dtcs;
int selectedDtc = 0;
unsigned int packetSeq = 0;
unsigned int lastDtcClear = 0;

const Color BG = {2, 5, 4, 255};
const Color PANEL = {8, 13, 12, 255};
const Color PANEL2 = {12, 18, 17, 255};
const Color LINE = {32, 54, 47, 255};
const Color TEXT = {218, 236, 220, 255};
const Color MUTED = {92, 125, 105, 255};
const Color GREEN = {126, 238, 154, 255};
const Color AMBER = {255, 184, 76, 255};
const Color CYAN = {96, 202, 214, 255};
const Color RED = {255, 82, 82, 255};

void color(Color c) {
    SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
}

void rect(int x, int y, int w, int h, Color c, bool fill) {
    SDL_Rect r = {x, y, w, h};
    color(c);
    if (fill) SDL_RenderFillRect(ren, &r); else SDL_RenderDrawRect(ren, &r);
}

void line(int x1, int y1, int x2, int y2, Color c) {
    color(c);
    SDL_RenderDrawLine(ren, x1, y1, x2, y2);
}

const unsigned char *glyph(char ch) {
    if (ch == ' ') return SPACE;
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    for (size_t i = 0; i < sizeof(GLYPHS) / sizeof(GLYPHS[0]); ++i) {
        if (GLYPHS[i].ch == ch) return GLYPHS[i].rows;
    }
    return UNKNOWN;
}

void text(int x, int y, const std::string &s, Color c, int scale) {
    color(c);
    int cx = x;
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char *g = glyph(s[i]);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (g[row] & (1 << (4 - col))) {
                    SDL_Rect p = {cx + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(ren, &p);
                    if (scale == 1 && col < 4) {
                        SDL_Rect b = {cx + col + 1, y + row, 1, 1};
                        SDL_RenderFillRect(ren, &b);
                    }
                }
            }
        }
        cx += 6 * scale;
    }
}

std::string fmt(const char *label, int value, const char *unit) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%-11s %5d %s", label, value, unit);
    return std::string(buf);
}

std::string fmtf(const char *label, float value, const char *unit) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%-11s %5.1f %s", label, value, unit);
    return std::string(buf);
}

void scanlines() {
    Color c = {0, 0, 0, 42};
    for (int y = (int)frame % 2; y < H; y += 3) rect(0, y, W, 1, c, true);
}

void graph(int x, int y, int w, int h, const std::vector<int> &v, int lo, int hi, Color c, const char *unit, int divisor) {
    rect(x, y, w, h, LINE, false);
    for (int gx = x + 16; gx < x + w; gx += 16) line(gx, y, gx, y + h, Color{12, 26, 22, 160});
    for (int gy = y + 12; gy < y + h; gy += 12) line(x, gy, x + w, gy, Color{12, 26, 22, 160});
    if (v.size() < 2 || hi <= lo) {
        text(x + 8, y + h / 2 - 4, "NO SAMPLE", MUTED, 1);
        return;
    }
    int minSeen = v[0];
    int maxSeen = v[0];
    for (size_t i = 1; i < v.size(); ++i) {
        minSeen = std::min(minSeen, v[i]);
        maxSeen = std::max(maxSeen, v[i]);
    }
    int lx = x;
    int ly = y + h;
    int lastX = x;
    int lastY = y + h;
    for (int i = 0; i < (int)v.size(); ++i) {
        int val = std::max(lo, std::min(hi, v[(size_t)i]));
        int px = x + (i * (w - 1)) / std::max(1, (int)v.size() - 1);
        int py = y + h - 1 - ((val - lo) * (h - 2)) / (hi - lo);
        if (i > 0) line(lx, ly, px, py, c);
        lx = px;
        ly = py;
        lastX = px;
        lastY = py;
    }
    rect(lastX - 2, lastY - 2, 5, 5, c, true);
    char buf[48];
    if (divisor > 1) {
        std::snprintf(buf, sizeof(buf), "%02d.%d%s", v.back() / divisor, v.back() % divisor, unit);
    } else {
        std::snprintf(buf, sizeof(buf), "%03d%s", v.back(), unit);
    }
    text(x + 7, y + 6, buf, c, 1);
    if (divisor > 1) {
        std::snprintf(buf, sizeof(buf), "MIN %02d.%d MAX %02d.%d", minSeen / divisor, minSeen % divisor, maxSeen / divisor, maxSeen % divisor);
    } else {
        std::snprintf(buf, sizeof(buf), "MIN %03d MAX %03d", minSeen, maxSeen);
    }
    text(x + 7, y + h - 14, buf, MUTED, 1);
}

void graphBars(int x, int y, int w, int h, const std::vector<int> &v, int lo, int hi, Color c) {
    rect(x, y, w, h, LINE, false);
    int plotTop = y + 18;
    int plotBottom = y + h - 6;
    int plotH = std::max(8, plotBottom - plotTop);
    for (int gy = plotTop + 10; gy < plotBottom; gy += 10) line(x, gy, x + w, gy, Color{12, 26, 22, 160});
    if (v.empty() || hi <= lo) {
        text(x + 8, y + h / 2 - 4, "NO SAMPLE", MUTED, 1);
        return;
    }
    int count = std::min((int)v.size(), 32);
    int start = (int)v.size() - count;
    int barW = std::max(2, (w - 12) / std::max(1, count));
    for (int i = 0; i < count; ++i) {
        int value = std::max(lo, std::min(hi, v[(size_t)(start + i)]));
        int bh = 2 + ((value - lo) * (plotH - 2)) / (hi - lo);
        int bx = x + 6 + i * barW;
        rect(bx, plotBottom - bh, std::max(1, barW - 1), bh, c, true);
    }
    char buf[48];
    std::snprintf(buf, sizeof(buf), "%03dMS  TOP %03d", v.back(), hi);
    text(x + 7, y + 6, buf, c, 1);
}

void frameShell(const std::string &title, const std::string &right) {
    rect(0, 0, W, H, BG, true);
    rect(8, 8, 624, 464, LINE, false);
    line(8, 33, 632, 33, LINE);
    text(18, 17, title, TEXT, 1);
    text(452, 18, right, MUTED, 1);
    text(18, 458, "B BACK  START OVR  SELECT PKT", MUTED, 1);
}

void panel(int x, int y, int w, int h, const std::string &label, Color c) {
    rect(x, y, w, h, PANEL2, true);
    rect(x, y, w, h, LINE, false);
    line(x + 1, y + 1, x + w - 2, y + 1, c);
    if (!label.empty()) text(x + 10, y + 9, label, MUTED, 1);
}
