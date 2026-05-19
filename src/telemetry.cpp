#include "app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>

namespace {

template <typename T>
void pushLimited(std::vector<T> &items, const T &item, size_t limit) {
    items.push_back(item);
    if (items.size() > limit) items.erase(items.begin());
}

void pushTelemetryHistory() {
    pushLimited(rpmHist, tel.rpm, MAX_HISTORY);
    pushLimited(latencyHist, tel.latency, MAX_HISTORY);
    pushLimited(coolantHist, tel.coolant, MAX_HISTORY);
    pushLimited(voltageHist, (int)(tel.voltage * 10.0f + 0.5f), MAX_HISTORY);
}

void updateSimTelemetry(unsigned int now) {
    double t = now / 1000.0;
    tel.rpm = 850 + (int)(std::fabs(std::sin(t * 0.9)) * 2650.0) + (std::rand() % 55);
    tel.speed = (int)(std::fabs(std::sin(t * 0.31)) * 74.0);
    tel.coolant = 82 + (int)(std::sin(t * 0.08) * 7.0);
    tel.intake = 29 + (int)(std::sin(t * 0.17) * 5.0);
    tel.throttle = 4 + (int)(std::fabs(std::sin(t * 1.4)) * 73.0);
    tel.load = 18 + (int)(std::fabs(std::sin(t * 0.73)) * 71.0);
    tel.voltage = 13.7f + (float)(std::sin(t * 0.7) * 0.28);
    tel.stft = (float)(std::sin(t * 0.41) * 5.2);
    tel.ltft = (float)(std::cos(t * 0.27) * 2.4);
    tel.latency = 11 + (int)(std::fabs(std::sin(t * 2.1)) * 18.0) + (std::rand() % 4);
}

} // namespace

void updateTelemetry(unsigned int now) {
    if (now - lastTelemetry < (unsigned int)appConfig.pollIntervalMs) return;
    lastTelemetry = now;

    if (pollObdClient(now)) {
        pushTelemetryHistory();
        return;
    }

    if (obdState == OBD_ONLINE || !appConfig.fallbackSim) return;

    updateSimTelemetry(now);
    pushTelemetryHistory();

    const char *pids[] = {"010C", "010D", "0105", "010F", "0111", "0142", "0104"};
    int idx = (int)(packetSeq % 7);
    bool anomaly = packetSeq % 47 == 0;
    char res[64];
    std::snprintf(res, sizeof(res), "41 %c%c %02X %02X%s", pids[idx][2], pids[idx][3],
                  (tel.rpm / 32 + (int)packetSeq) & 0xff,
                  (tel.speed + (int)packetSeq * 3) & 0xff,
                  anomaly ? " ?7" : "");
    Packet p;
    p.tick = now;
    p.req = pids[idx];
    p.res = res;
    p.latency = tel.latency;
    p.anomaly = anomaly;
    pushLimited(packets, p, MAX_PACKETS);
    ++packetSeq;
}

void exportPackets() {
    mkdir("data", 0755);
    std::ofstream out("data/packet_capture.log", std::ios::out | std::ios::app);
    const std::vector<Packet> &lines = packetPaused ? pausedPackets : packets;
    for (size_t i = 0; i < lines.size(); ++i) {
        out << lines[i].tick << " " << lines[i].req << " " << lines[i].res << " "
            << lines[i].latency << " " << (lines[i].anomaly ? "ANOMALY" : "OK") << "\n";
    }
}
