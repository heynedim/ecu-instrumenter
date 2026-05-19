#include <arpa/inet.h>
#include <cerrno>
#include <csignal>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <ctime>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace {

bool running = true;

enum SimMode {
    MODE_NORMAL,
    MODE_CORRUPT,
    MODE_DROP,
    MODE_TIMEOUT,
    MODE_SLOW
};

void onSignal(int) {
    running = false;
}

std::string normalize(const std::string &cmd) {
    std::string out;
    for (size_t i = 0; i < cmd.size(); ++i) {
        unsigned char ch = (unsigned char)cmd[i];
        if (std::isxdigit(ch) || std::isalpha(ch)) out += (char)std::toupper(ch);
    }
    return out;
}

void sendText(int fd, const std::string &s) {
    send(fd, s.c_str(), s.size(), 0);
}

std::string hex2(int value) {
    char buf[4];
    std::snprintf(buf, sizeof(buf), "%02X", value & 0xff);
    return std::string(buf);
}

SimMode parseMode(const char *value) {
    if (!value) return MODE_NORMAL;
    std::string mode(value);
    for (size_t i = 0; i < mode.size(); ++i) mode[i] = (char)std::tolower((unsigned char)mode[i]);
    if (mode == "corrupt") return MODE_CORRUPT;
    if (mode == "drop") return MODE_DROP;
    if (mode == "timeout") return MODE_TIMEOUT;
    if (mode == "slow") return MODE_SLOW;
    return MODE_NORMAL;
}

const char *modeName(SimMode mode) {
    switch (mode) {
        case MODE_CORRUPT: return "corrupt";
        case MODE_DROP: return "drop";
        case MODE_TIMEOUT: return "timeout";
        case MODE_SLOW: return "slow";
        default: return "normal";
    }
}

std::string responseFor(const std::string &cmd) {
    double t = (double)std::time(NULL);
    int rpm = 850 + (int)(std::fabs(std::sin(t * 0.31)) * 2600.0);
    int speed = (int)(std::fabs(std::sin(t * 0.11)) * 85.0);
    int coolant = 84 + (int)(std::sin(t * 0.03) * 6.0);
    int intake = 31 + (int)(std::sin(t * 0.07) * 5.0);
    int throttle = 5 + (int)(std::fabs(std::sin(t * 0.49)) * 70.0);
    int load = 18 + (int)(std::fabs(std::sin(t * 0.29)) * 72.0);
    int voltageMv = 13750 + (int)(std::sin(t * 0.19) * 260.0);

    if (cmd.empty()) return ">";
    if (cmd.size() >= 2 && cmd[0] == 'A' && cmd[1] == 'T') return "OK\r>";
    if (cmd == "010C") {
        int raw = rpm * 4;
        return "41 0C " + hex2(raw / 256) + " " + hex2(raw) + "\r>";
    }
    if (cmd == "010D") return "41 0D " + hex2(speed) + "\r>";
    if (cmd == "0105") return "41 05 " + hex2(coolant + 40) + "\r>";
    if (cmd == "010F") return "41 0F " + hex2(intake + 40) + "\r>";
    if (cmd == "0111") return "41 11 " + hex2((throttle * 255) / 100) + "\r>";
    if (cmd == "0142") return "41 42 " + hex2(voltageMv / 256) + " " + hex2(voltageMv) + "\r>";
    if (cmd == "0104") return "41 04 " + hex2((load * 255) / 100) + "\r>";
    return "NO DATA\r>";
}

} // namespace

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? std::atoi(argv[2]) : 35000;
    SimMode mode = parseMode(argc > 3 ? argv[3] : NULL);
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        std::perror("socket");
        return 1;
    }
    int yes = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        std::fprintf(stderr, "Invalid host: %s\n", host);
        close(server);
        return 1;
    }
    if (bind(server, (sockaddr *)&addr, sizeof(addr)) != 0) {
        std::perror("bind");
        close(server);
        return 1;
    }
    if (listen(server, 4) != 0) {
        std::perror("listen");
        close(server);
        return 1;
    }

    std::printf("OBD simulator listening on %s:%d mode=%s\n", host, port, modeName(mode));
    std::fflush(stdout);
    while (running) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int client = accept(server, (sockaddr *)&clientAddr, &clientLen);
        if (client < 0) {
            if (errno == EINTR) continue;
            std::perror("accept");
            break;
        }
        std::printf("client connected\n");
        std::string buf;
        char chunk[256];
        while (running) {
            ssize_t n = recv(client, chunk, sizeof(chunk) - 1, 0);
            if (n <= 0) break;
            chunk[n] = 0;
            buf.append(chunk, (size_t)n);
            size_t pos;
            while ((pos = buf.find('\r')) != std::string::npos || (pos = buf.find('\n')) != std::string::npos) {
                std::string cmd = normalize(buf.substr(0, pos));
                buf.erase(0, pos + 1);
                if (mode == MODE_DROP) {
                    std::printf("%s -> DROP\n", cmd.c_str());
                    std::fflush(stdout);
                    break;
                }
                if (mode == MODE_TIMEOUT) {
                    std::printf("%s -> TIMEOUT\n", cmd.c_str());
                    std::fflush(stdout);
                    continue;
                }
                if (mode == MODE_SLOW) usleep(1500 * 1000);
                std::string res = mode == MODE_CORRUPT ? "41 FF ?? BAD\r>" : responseFor(cmd);
                std::printf("%s -> %s\n", cmd.c_str(), res.c_str());
                std::fflush(stdout);
                sendText(client, res);
            }
            if (mode == MODE_DROP) break;
        }
        close(client);
        std::printf("client disconnected\n");
    }
    close(server);
    return 0;
}
