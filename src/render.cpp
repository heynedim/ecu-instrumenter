#include "app.h"

#include <algorithm>
#include <cstdio>

namespace {

Color obdStatusColor() {
    if (obdState == OBD_ONLINE) return GREEN;
    if (obdState == OBD_CONNECTING) return AMBER;
    if (obdState == OBD_FALLBACK) return CYAN;
    return RED;
}

std::string endpointText() {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s:%d", appConfig.obdHost.c_str(), appConfig.obdPort);
    return std::string(buf);
}

bool obdFaultActive() {
    return !obdFaultText.empty() && obdFaultText != "NONE";
}

const char *bootRowStatus(int row, unsigned int elapsed) {
    (void)elapsed;
    if (obdState == OBD_CONNECTING) return "WAIT";
    if (row == 0) {
        if (obdBootCheck.adapterLinked) return "OK";
        if (obdState == OBD_FALLBACK || obdState == OBD_DISCONNECTED || obdFaultActive()) return "FAIL";
        return "WAIT";
    }
    if (row == 1) {
        if (obdBootCheck.busActivity) return "OK";
        if (obdState == OBD_FALLBACK) return "SIM";
        if (obdBootCheck.adapterLinked) return "WAIT";
        return "NO";
    }
    if (row == 2) {
        if (obdBootCheck.responseWindow) return "OK";
        if (obdFaultActive()) return obdFaultText.c_str();
        if (obdBootCheck.adapterLinked || obdBootCheck.busActivity) return "WAIT";
        return "NO";
    }
    if (row == 3) {
        if (obdBootCheck.mode01Session) return "OK";
        if (obdBootCheck.responseWindow) return "BAD FRAME";
        if (obdFaultActive()) return "NO";
        if (obdBootCheck.adapterLinked) return "WAIT";
        return "NO";
    }
    if (obdBootCheck.validStreak >= 3) return "LOCKED";
    if (obdBootCheck.mode01Session) return "WAIT";
    return obdBootCheck.adapterLinked ? "WAIT" : "NO";
}

Color bootRowColor(int row, unsigned int elapsed) {
    const char *status = bootRowStatus(row, elapsed);
    if (std::string(status) == "OK" || std::string(status) == "LOCKED") return row == 4 ? CYAN : GREEN;
    if (std::string(status) == "WAIT") return MUTED;
    if (std::string(status) == "SIM") return CYAN;
    return RED;
}

} // namespace

void drawBoot(unsigned int now) {
    unsigned int e = now - bootStart;

    rect(0, 0, W, H, BG, true);
    text(86, 72, "ECU-INSTRUMENTER V0.1", GREEN, 2);
    text(89, 103, "PROTOTYPE OBD DIAGNOSTICS TERMINAL / MIYOO MINI PLUS", MUTED, 1);
    rect(84, 122, 470, 1, LINE, true);
    text(96, 146, "WAITING FOR OBD SIGNAL", AMBER, 1);
    const char *lines[] = {"ADAPTER LINK", "VEHICLE BUS ACTIVITY", "ECU RESPONSE WINDOW", "OBD MODE 01 SESSION", "SIGNAL LOCK ACQUIRED"};
    for (int i = 0; i < 5; ++i) {
        int y = 180 + i * 26;
        const char *status = bootRowStatus(i, e);
        Color rowColor = bootRowColor(i, e);
        text(96, y, lines[i], rowColor, 1);
        text(430, y, status, rowColor, 1);
    }
    text(96, 330, "OBD TCP", AMBER, 1);
    text(188, 330, endpointText(), MUTED, 1);
    text(96, 358, obdStatusText, obdStatusColor(), 1);
    if (obdFaultActive()) text(310, 358, obdFaultText, AMBER, 1);
}

void drawMenu(unsigned int now) {
    (void)now;
    frameShell("ECU-INSTRUMENTER V0.1", "OEM ENG FW 0.1 / SIM OBD");
    text(48, 58, "SELECT MODULE", AMBER, 1);
    const char *items[] = {"LIVE TELEMETRY", "FAULT CODES", "PACKET MONITOR", "SYSTEM", "EXIT"};
    for (int i = 0; i < 5; ++i) {
        int y = 92 + i * 42;
        bool s = i == menuIndex;
        rect(42, y - 8, 252, 27, s ? LINE : PANEL, true);
        text(58, y, s ? ">" : " ", s ? AMBER : MUTED, 1);
        text(78, y, items[i], i == 4 ? (s ? RED : AMBER) : (s ? GREEN : MUTED), 1);
    }
    text(362, 82, "VEHICLE LINK", CYAN, 1);
    line(362, 102, 568, 102, LINE);
    text(362, 124, "OBD TCP", AMBER, 1);
    text(362, 148, endpointText(), MUTED, 1);
    text(362, 172, obdStatusText, obdStatusColor(), 1);
    text(362, 196, "LAST FAULT         " + obdFaultText, obdFaultText == "NONE" ? MUTED : AMBER, 1);
    text(362, 220, "LAST PID           " + obdLastPid, MUTED, 1);
    text(362, 244, fmt("LATENCY", obdLastLatency, "MS"), MUTED, 1);
}

void drawLive(unsigned int now) {
    (void)now;
    frameShell("LIVE TELEMETRY", "NON-BLOCKING POLL / 60HZ UI");
    text(32, 56, fmt("RPM", tel.rpm, ""), GREEN, 1);
    text(32, 82, fmt("SPEED", tel.speed, "KMH"), GREEN, 1);
    text(32, 108, fmt("COOLANT", tel.coolant, "C"), tel.coolant > 98 ? AMBER : GREEN, 1);
    text(32, 134, fmt("INTAKE", tel.intake, "C"), MUTED, 1);
    text(228, 56, fmt("THROTTLE", tel.throttle, "%"), GREEN, 1);
    text(228, 82, fmtf("VOLTAGE", tel.voltage, "V"), tel.voltage < 12.8f ? AMBER : GREEN, 1);
    text(228, 108, fmtf("STFT", tel.stft, "%"), MUTED, 1);
    text(228, 134, fmtf("LTFT", tel.ltft, "%"), MUTED, 1);
    text(424, 56, fmt("LOAD", tel.load, "%"), GREEN, 1);
    text(424, 82, fmt("LATENCY", tel.latency, "MS"), tel.latency > 31 ? AMBER : CYAN, 1);
    graph(32, 174, 266, 70, rpmHist, 0, 5000, GREEN, "", 1);
    graph(334, 174, 266, 70, latencyHist, 0, 80, CYAN, "MS", 1);
    graph(32, 288, 266, 70, coolantHist, 60, 115, AMBER, "C", 1);
    graph(334, 288, 266, 70, voltageHist, 110, 150, GREEN, "V", 10);
}

void drawFaults() {
    frameShell("FAULT CODES", "DTC READ / FREEZE FRAME / HISTORY");
    if (dtcs.empty()) {
        panel(82, 92, 476, 170, "DIAGNOSTIC RESULT", GREEN);
        text(238, 158, "NO STORED DTC", GREEN, 2);
        text(158, 224, "LOCAL DTC LIST CLEAR / ECU CLEAR COMMAND READY", MUTED, 1);
        if (lastDtcClear != 0 && SDL_GetTicks() - lastDtcClear < 2200) {
            text(202, 300, "SELECTED DTC CLEARED", AMBER, 1);
        }
        return;
    }
    if (selectedDtc >= (int)dtcs.size()) selectedDtc = (int)dtcs.size() - 1;
    if (selectedDtc < 0) selectedDtc = 0;
    for (int i = 0; i < (int)dtcs.size(); ++i) {
        int y = 64 + i * 40;
        bool s = i == selectedDtc;
        std::string row = dtcs[(size_t)i].code + " " + dtcs[(size_t)i].desc;
        rect(28, y - 8, 286, 28, s ? Color{80, 50, 20, 255} : PANEL2, true);
        text(42, y, row, dtcs[(size_t)i].code[0] == 'U' ? RED : (s ? AMBER : Color{154, 101, 35, 255}), 1);
    }
    panel(342, 62, 244, 224, "EXPANDED DETAIL PANEL", AMBER);
    const Dtc &d = dtcs[(size_t)selectedDtc];
    text(358, 84, d.code + " SELECTED", AMBER, 1);
    text(358, 114, "STATUS        " + d.status, MUTED, 1);
    text(358, 138, fmt("MILEAGE", d.mileage, "KM"), MUTED, 1);
    text(358, 162, fmt("FREEZE RPM", d.rpm, ""), MUTED, 1);
    text(358, 186, fmt("FREEZE LOAD", d.load, "%"), MUTED, 1);
    text(358, 232, "X CLEAR SELECTED DTC", AMBER, 1);
    text(358, 256, "Y CLEAR ALL LOCAL", MUTED, 1);
    text(32, 324, "HISTORICAL CODE LOG", AMBER, 1);
    text(32, 350, "UP/DOWN SELECT  X CLEAR ONLY HIGHLIGHTED CODE", MUTED, 1);
    if (lastDtcClear != 0 && SDL_GetTicks() - lastDtcClear < 2200) {
        text(32, 372, "LOCAL CLEAR COMPLETE / REAL ECU CLEAR REQUIRES MODE 04", AMBER, 1);
    } else {
        text(32, 372, "18:47:03  " + d.code + "  FREEZE FRAME AVAILABLE", MUTED, 1);
    }
}

void drawPacket(unsigned int now) {
    (void)now;
    const std::vector<Packet> &lines = packetPaused ? pausedPackets : packets;
    int latencyTop = 80;
    if (!latencyHist.empty()) {
        int count = std::min((int)latencyHist.size(), 32);
        int start = (int)latencyHist.size() - count;
        for (int i = start; i < (int)latencyHist.size(); ++i) latencyTop = std::max(latencyTop, latencyHist[(size_t)i]);
        latencyTop = ((latencyTop + 39) / 40) * 40;
    }
    frameShell("PACKET MONITOR", packetPaused ? "STREAM PAUSED / RAW OBD" : "LIVE RAW OBD STREAM");
    text(28, 54, "TIME      REQ    RESPONSE                 LAT  FLG", AMBER, 1);
    int start = lines.size() > (size_t)PACKET_ROWS ? (int)lines.size() - PACKET_ROWS : 0;
    for (int i = start; i < (int)lines.size(); ++i) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%06u  %-5s  %-22s %03d  %s", lines[(size_t)i].tick,
                      lines[(size_t)i].req.c_str(), lines[(size_t)i].res.c_str(),
                      lines[(size_t)i].latency, lines[(size_t)i].anomaly ? "ANM" : "OK");
        text(28, 76 + (i - start) * 18, buf, lines[(size_t)i].anomaly ? AMBER : MUTED, 1);
    }
    panel(438, 76, 154, 70, "", LINE);
    text(452, 94, fmt("PKT", (int)lines.size(), ""), GREEN, 1);
    text(452, 118, packetPaused ? "A RESUME" : "A PAUSE", AMBER, 1);
    text(452, 140, "X EXPORT", MUTED, 1);
    graphBars(438, 176, 154, 54, latencyHist, 0, latencyTop, CYAN);
    text(438, 238, "LATENCY BAR SCOPE", MUTED, 1);
}

void drawSystem(unsigned int now) {
    frameShell("SYSTEM", "CONFIG / THEME / DEVICE STATUS");
    text(34, 64, "DISPLAY          640X480 LOGICAL", MUTED, 1);
    text(34, 90, "RENDERER         SDL2 FRAMEBUFFER READY", MUTED, 1);
    text(34, 116, "THEME            PHOSPHOR LOW-GLOW", MUTED, 1);
    text(34, 142, "NETWORK          " + endpointText(), MUTED, 1);
    text(34, 168, "FILESYSTEM       SD CARD LOG ROOT", MUTED, 1);
    text(34, 194, "OBD STATUS       " + obdStatusText, obdStatusColor(), 1);
    text(34, 220, "LAST FAULT       " + obdFaultText, obdFaultText == "NONE" ? MUTED : AMBER, 1);
    text(34, 246, "FALLBACK SIM     " + std::string(appConfig.fallbackSim ? "ENABLED" : "DISABLED"), MUTED, 1);
    text(34, 272, fmt("UPTIME", (int)(now / 1000), "SEC"), GREEN, 1);
}

void drawOverlay(unsigned int now) {
    rect(112, 104, 416, 222, Color{0, 0, 0, 224}, true);
    rect(112, 104, 416, 222, CYAN, false);
    text(138, 128, "QUICK OVERLAY", CYAN, 1);
    text(138, 166, "A CONFIRM     B BACK", MUTED, 1);
    text(138, 190, "SELECT PACKET MONITOR", MUTED, 1);
    text(138, 214, "X EXPORT WHEN AVAILABLE", MUTED, 1);
    text(138, 238, "D-PAD ONLY TARGET UX", AMBER, 1);
    text(138, 280, fmt("UPTIME", (int)(now / 1000), "SEC"), GREEN, 1);
}
