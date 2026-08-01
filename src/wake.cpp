#include "include/wake.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>

#include <sys/eventfd.h>
#include <unistd.h>

Wake::Wake() : cFd(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {
    if (cFd < 0) {
        std::perror("eventfd");
    }
}

Wake::~Wake() {
    if (cFd >= 0) {
        ::close(cFd);
    }
}

int Wake::fd() const {
    return cFd;
}

bool Wake::notify() const {
    const std::uint64_t one = 1;
    for (;;) {
        const auto written = ::write(cFd, &one, sizeof(one));
        if (written == sizeof(one)) {
            return true;
        }
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return true;
        }
        std::perror("eventfd write");
        return false;
    }
}

void Wake::consume() {
    std::uint64_t ignored = 0;
    for (;;) {
        const auto readSize = ::read(cFd, &ignored, sizeof(ignored));
        if (readSize == sizeof(ignored)) {
            return;
        }
        if (readSize < 0 && errno == EINTR) {
            continue;
        }
        if (readSize < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return;
        }
        std::perror("eventfd read");
        return;
    }
}
