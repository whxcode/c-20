#pragma once

#include <cstdint>

namespace net {

enum Event : std::uint32_t {
    None = 0,
    Read = 1U << 0,
    Write = 1U << 1,
    Error = 1U << 2,
    EdgeTriggered = 1U << 3,
};

enum class Operation {
    Add,
    Modify,
    Delete,
};

struct ReadyEvent {
    int fd{-1};
    std::uint32_t events{None};
};

int create();
int ctl(int eventFd, Operation operation, int fd, std::uint32_t events);
int wait(int eventFd, ReadyEvent* events, int maxEvents, int timeoutMs);
int setNonBlocking(int fd);

}  // namespace net
