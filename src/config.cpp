#include "app.h"

#include <cstdlib>
#include <fstream>

namespace {

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool parseBool(const std::string &value) {
    return value == "true" || value == "1" || value == "yes" || value == "on";
}

} // namespace

void loadConfig(const char *path) {
    std::ifstream in(path);
    if (!in) return;

    bool inObd = false;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line == "[obd]") {
            inObd = true;
            continue;
        }
        if (!inObd) continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));
        if (key == "host" && !value.empty()) appConfig.obdHost = value;
        else if (key == "port") appConfig.obdPort = std::atoi(value.c_str());
        else if (key == "connect_timeout_ms") appConfig.connectTimeoutMs = std::atoi(value.c_str());
        else if (key == "poll_interval_ms") appConfig.pollIntervalMs = std::atoi(value.c_str());
        else if (key == "fallback_sim") appConfig.fallbackSim = parseBool(value);
    }

    if (appConfig.obdPort <= 0) appConfig.obdPort = 35000;
    if (appConfig.connectTimeoutMs <= 0) appConfig.connectTimeoutMs = 900;
    if (appConfig.pollIntervalMs < 50) appConfig.pollIntervalMs = 50;
}
