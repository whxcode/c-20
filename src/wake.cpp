#include "include/wake.h"

#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iostream>

#include "include/tools/file.h"
#include "include/tools/net.h"

Wake::Wake() {
    int fds[2]{0};
    if (pipe(fds) < 0) {
        file::printError("Wake::Wake");
    }

    cReadFd = fds[0];
    cWriteFd = fds[1];

    net::setNonBlocking(cReadFd);
    net::setNonBlocking(cWriteFd);
}

Wake::~Wake() {
    if (cReadFd >= 0) {
        ::close(cReadFd);
    }

    if (cWriteFd >= 0) {
        ::close(cWriteFd);
    }
}

bool Wake::notify() const {
    char value{1};
    for (;;) {
        const auto written = ::write(cWriteFd, &value, 1);
        if (written == 1) {
            return true;
        }

        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        }
        file::printError("Wake::notify error:");
        return false;
    }
}

void Wake::consume() {
    char value{0};
    for (;;) {
        const auto readSize = ::read(cReadFd, &value, 1);

        if (readSize < 0) {
            if (errno == EINTR) {
                continue;
            }

            if ((errno == EAGAIN || errno == EWOULDBLOCK)) {
                return;
            }

            file::printError("Wake::consume error:");
        }
    }
}

int Wake::writeFd() const {
    return cWriteFd;
}

int Wake::readFd() const {
    return cReadFd;
}
