#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

bool waitForReadable(int fd, int timeoutMs) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return select(fd + 1, &rfds, NULL, NULL, &tv) > 0 && FD_ISSET(fd, &rfds);
}

std::string compactHex(const std::string &raw) {
    std::string out;
    for (size_t i = 0; i < raw.size(); ++i) {
        char ch = raw[i];
        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) out += ch;
        else if (ch >= 'a' && ch <= 'f') out += (char)(ch - 'a' + 'A');
    }
    return out;
}

} // namespace

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? std::atoi(argv[2]) : 35000;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::perror("socket");
        return 1;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        std::fprintf(stderr, "FAIL: invalid host %s\n", host);
        close(fd);
        return 1;
    }

    std::printf("Connecting to %s:%d...\n", host, port);
    std::fflush(stdout);
    if (connect(fd, (sockaddr *)&addr, sizeof(addr)) != 0) {
        std::fprintf(stderr, "FAIL: connect error: %s\n", std::strerror(errno));
        close(fd);
        return 1;
    }

    const char *req = "010C\r";
    if (send(fd, req, std::strlen(req), 0) != (ssize_t)std::strlen(req)) {
        std::fprintf(stderr, "FAIL: write error\n");
        close(fd);
        return 1;
    }

    if (!waitForReadable(fd, 1200)) {
        std::fprintf(stderr, "FAIL: timed out waiting for OBD response\n");
        close(fd);
        return 1;
    }

    char buf[256];
    ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
    close(fd);
    if (n <= 0) {
        std::fprintf(stderr, "FAIL: empty response\n");
        return 1;
    }
    buf[n] = 0;
    std::string raw(buf);
    std::string hex = compactHex(raw);
    std::printf("Response: %s\n", raw.c_str());
    if (hex.find("410C") == std::string::npos) {
        std::fprintf(stderr, "FAIL: expected RPM response 41 0C\n");
        return 1;
    }
    std::printf("PASS: OBD endpoint replied to PID 010C\n");
    return 0;
}
