#include "app.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

int sockFd = -1;
bool connecting = false;
unsigned int connectStarted = 0;
unsigned int nextConnectAt = 0;
unsigned int nextPollAt = 0;
unsigned int requestSentAt = 0;
int pidIndex = 0;
std::string pendingReq;
std::string rxBuffer;

const char *PIDS[] = {"010C", "010D", "0105", "010F", "0111", "0142", "0104"};

void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void closeSocket() {
    if (sockFd >= 0) close(sockFd);
    sockFd = -1;
    connecting = false;
    pendingReq.clear();
    rxBuffer.clear();
}

void resetBootChecks() {
    obdBootCheck.adapterLinked = false;
    obdBootCheck.busActivity = false;
    obdBootCheck.responseWindow = false;
    obdBootCheck.mode01Session = false;
    obdBootCheck.validStreak = 0;
}

void markAdapterLinked(unsigned int now) {
    (void)now;
    obdBootCheck.adapterLinked = true;
    obdBootCheck.busActivity = false;
    obdBootCheck.responseWindow = false;
    obdBootCheck.mode01Session = false;
    obdBootCheck.validStreak = 0;
}

std::string compactHex(const std::string &raw) {
    std::string out;
    for (size_t i = 0; i < raw.size(); ++i) {
        unsigned char ch = (unsigned char)raw[i];
        if (std::isxdigit(ch)) out += (char)std::toupper(ch);
    }
    return out;
}

std::string cleanDisplay(const std::string &raw) {
    std::string out;
    bool lastSpace = false;
    for (size_t i = 0; i < raw.size(); ++i) {
        char ch = raw[i];
        if (ch == '\r' || ch == '\n' || ch == '>') ch = ' ';
        if (ch == ' ') {
            if (!lastSpace && !out.empty()) out += ch;
            lastSpace = true;
        } else {
            out += ch;
            lastSpace = false;
        }
    }
    while (!out.empty() && out[out.size() - 1] == ' ') out.erase(out.size() - 1);
    return out.empty() ? "NO DATA" : out;
}

int hexByte(const std::string &s, size_t pos) {
    if (pos + 2 > s.size()) return -1;
    char tmp[3] = {s[pos], s[pos + 1], 0};
    return (int)std::strtol(tmp, NULL, 16);
}

bool applyObdResponse(const std::string &req, const std::string &raw) {
    std::string hex = compactHex(raw);
    if (req.size() < 4) return false;
    std::string marker = "41" + req.substr(2, 2);
    size_t pos = hex.find(marker);
    if (pos == std::string::npos) return false;
    pos += marker.size();
    int a = hexByte(hex, pos);
    int b = hexByte(hex, pos + 2);
    if (a < 0) return false;

    if (req == "010C") {
        if (b < 0) return false;
        tel.rpm = ((a * 256) + b) / 4;
    } else if (req == "010D") {
        tel.speed = a;
    } else if (req == "0105") {
        tel.coolant = a - 40;
    } else if (req == "010F") {
        tel.intake = a - 40;
    } else if (req == "0111") {
        tel.throttle = (a * 100) / 255;
    } else if (req == "0142") {
        if (b < 0) return false;
        tel.voltage = (float)((a * 256) + b) / 1000.0f;
    } else if (req == "0104") {
        tel.load = (a * 100) / 255;
    }
    return true;
}

void recordPacket(unsigned int now, const std::string &req, const std::string &raw, bool anomaly) {
    Packet p;
    p.tick = now;
    p.req = req;
    p.res = cleanDisplay(raw);
    p.latency = (int)(now - requestSentAt);
    p.anomaly = anomaly;
    packets.push_back(p);
    if (packets.size() > MAX_PACKETS) packets.erase(packets.begin());
    obdLastPid = req;
    obdLastResponse = p.res;
    obdLastLatency = p.latency;
    obdLastRx = now;
    tel.latency = p.latency;
}

void markDisconnected(unsigned int now, const char *status) {
    closeSocket();
    resetBootChecks();
    obdState = appConfig.fallbackSim ? OBD_FALLBACK : OBD_DISCONNECTED;
    obdStatusText = appConfig.fallbackSim ? "FALLBACK SIM" : status;
    obdFaultText = status;
    nextConnectAt = now + 1000;
}

void startConnect(unsigned int now) {
    if (now < nextConnectAt) return;

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0) {
        markDisconnected(now, "SOCKET FAIL");
        return;
    }
    setNonBlocking(sockFd);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)appConfig.obdPort);
    if (inet_pton(AF_INET, appConfig.obdHost.c_str(), &addr.sin_addr) != 1) {
        markDisconnected(now, "BAD HOST");
        return;
    }

    int rc = connect(sockFd, (sockaddr *)&addr, sizeof(addr));
    if (rc == 0) {
        connecting = false;
        obdState = OBD_ONLINE;
        obdStatusText = "SERVER ONLINE";
        obdFaultText = "NONE";
        markAdapterLinked(now);
        nextPollAt = now;
        return;
    }
    if (errno == EINPROGRESS) {
        connecting = true;
        connectStarted = now;
        obdState = OBD_CONNECTING;
        obdStatusText = "CONNECTING";
        return;
    }
    markDisconnected(now, "NO SIGNAL");
}

void pollConnect(unsigned int now) {
    if (!connecting || sockFd < 0) return;
    if (now - connectStarted > (unsigned int)appConfig.connectTimeoutMs) {
        markDisconnected(now, "NO SIGNAL");
        return;
    }
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sockFd, &wfds);
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int rc = select(sockFd + 1, NULL, &wfds, NULL, &tv);
    if (rc <= 0 || !FD_ISSET(sockFd, &wfds)) return;

    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(sockFd, SOL_SOCKET, SO_ERROR, &err, &len) != 0 || err != 0) {
        markDisconnected(now, "NO SIGNAL");
        return;
    }
    connecting = false;
    obdState = OBD_ONLINE;
    obdStatusText = "SERVER ONLINE";
    obdFaultText = "NONE";
    markAdapterLinked(now);
    nextPollAt = now;
}

bool readResponse(unsigned int now) {
    if (sockFd < 0 || pendingReq.empty()) return false;
    char buf[256];
    for (;;) {
        ssize_t n = recv(sockFd, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = 0;
            rxBuffer.append(buf, (size_t)n);
            obdBootCheck.busActivity = true;
            continue;
        }
        if (n == 0) {
            markDisconnected(now, "NO SIGNAL");
            return false;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
        markDisconnected(now, "READ FAIL");
        return false;
    }

    size_t end = rxBuffer.find('>');
    if (end == std::string::npos) end = rxBuffer.find('\n');
    if (end == std::string::npos) {
        if (now - requestSentAt > 1000) markDisconnected(now, "TIMEOUT");
        return false;
    }

    std::string raw = rxBuffer.substr(0, end + 1);
    rxBuffer.erase(0, end + 1);
    std::string req = pendingReq;
    pendingReq.clear();
    obdBootCheck.responseWindow = true;
    bool ok = applyObdResponse(req, raw);
    recordPacket(now, req, raw, !ok);
    obdBootCheck.mode01Session = ok;
    obdBootCheck.validStreak = ok ? obdBootCheck.validStreak + 1 : 0;
    obdState = OBD_ONLINE;
    obdStatusText = ok ? "SERVER ONLINE" : "BAD FRAME";
    obdFaultText = ok ? "NONE" : "BAD FRAME";
    return ok;
}

void sendNextPid(unsigned int now) {
    if (sockFd < 0 || connecting || !pendingReq.empty() || now < nextPollAt) return;
    std::string req = PIDS[pidIndex % (int)(sizeof(PIDS) / sizeof(PIDS[0]))];
    std::string wire = req + "\r";
    ssize_t n = send(sockFd, wire.c_str(), wire.size(), 0);
    if (n != (ssize_t)wire.size()) {
        markDisconnected(now, "WRITE FAIL");
        return;
    }
    pendingReq = req;
    requestSentAt = now;
    nextPollAt = now + (unsigned int)appConfig.pollIntervalMs;
    ++pidIndex;
}

} // namespace

void initObdClient() {
    signal(SIGPIPE, SIG_IGN);
    nextConnectAt = 0;
}

void shutdownObdClient() {
    closeSocket();
}

bool pollObdClient(unsigned int now) {
    if (sockFd < 0) startConnect(now);
    pollConnect(now);
    bool updated = readResponse(now);
    sendNextPid(now);
    return updated;
}
