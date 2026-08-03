#include "include/tools/net.h"

#include <cerrno>
#include <cstdio>
#include <vector>

#ifdef __linux__
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <fcntl.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace net {
namespace {

#ifdef __linux__
std::uint32_t toNativeEvents(std::uint32_t events) {
    std::uint32_t nativeEvents = 0;
    if (events & Event::Read) nativeEvents |= EPOLLIN;
    if (events & Event::Write) nativeEvents |= EPOLLOUT;
    if (events & Event::Error) nativeEvents |= EPOLLERR;
    if (events & Event::EdgeTriggered) nativeEvents |= EPOLLET;
    return nativeEvents;
}

int toNativeOperation(Operation operation) {
    switch (operation) {
        case Operation::Add:
            return EPOLL_CTL_ADD;
        case Operation::Modify:
            return EPOLL_CTL_MOD;
        case Operation::Delete:
            return EPOLL_CTL_DEL;
    }
    return EPOLL_CTL_DEL;
}
#endif

}  // namespace

int create() {
#ifdef __linux__
    return ::epoll_create1(EPOLL_CLOEXEC);
#elif defined(__APPLE__)
    return ::kqueue();
#endif
}

int ctl(int eventFd, Operation operation, int fd, std::uint32_t events) {
#ifdef __linux__
    epoll_event event{};
    event.events = toNativeEvents(events);
    event.data.fd = fd;
    const int result = ::epoll_ctl(eventFd, toNativeOperation(operation), fd, &event);
    if (result < 0) {
        std::perror("epoll_ctl");
    }
    return result;
#elif defined(__APPLE__)
    if (operation == Operation::Delete) {
        struct kevent changes[2]{};
        EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
        return ::kevent(eventFd, changes, 2, nullptr, 0, nullptr);
    }

    struct kevent changes[2]{};
    int changeCount = 0;
    const std::uint16_t edgeFlag = (events & Event::EdgeTriggered) ? EV_CLEAR : 0;

    if (events & Event::Read) {
        EV_SET(&changes[changeCount++], fd, EVFILT_READ, EV_ADD | EV_ENABLE | edgeFlag, 0, 0,
               nullptr);
    } else if (operation == Operation::Modify) {
        EV_SET(&changes[changeCount++], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    }

    if (events & Event::Write) {
        EV_SET(&changes[changeCount++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | edgeFlag, 0, 0,
               nullptr);
    } else if (operation == Operation::Modify) {
        EV_SET(&changes[changeCount++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    }

    return changeCount == 0 ? 0 : ::kevent(eventFd, changes, changeCount, nullptr, 0, nullptr);
#endif
}

int wait(int eventFd, ReadyEvent* events, int maxEvents, int timeoutMs) {
    if (maxEvents <= 0) {
        return 0;
    }

#ifdef __linux__
    std::vector<epoll_event> nativeEvents(static_cast<size_t>(maxEvents));
    const int readyCount = ::epoll_wait(eventFd, nativeEvents.data(), maxEvents, timeoutMs);
    if (readyCount < 0) {
        return readyCount;
    }

    for (int index = 0; index < readyCount; ++index) {
        const auto eventIndex = static_cast<size_t>(index);
        const auto nativeFlags = nativeEvents[eventIndex].events;
        events[eventIndex].fd = nativeEvents[eventIndex].data.fd;
        events[eventIndex].events = None;
        if ((nativeFlags & EPOLLIN) != 0U) events[eventIndex].events |= Event::Read;
        if ((nativeFlags & EPOLLOUT) != 0U) events[eventIndex].events |= Event::Write;
        if ((nativeFlags & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0U) {
            events[eventIndex].events |= Event::Error;
        }
    }
    return readyCount;
#elif defined(__APPLE__)
    std::vector<struct kevent> nativeEvents(static_cast<size_t>(maxEvents));
    timespec timeout{};
    timespec* timeoutPointer = nullptr;
    if (timeoutMs >= 0) {
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_nsec = (timeoutMs % 1000) * 1000000;
        timeoutPointer = &timeout;
    }

    const int readyCount =
        ::kevent(eventFd, nullptr, 0, nativeEvents.data(), maxEvents, timeoutPointer);
    if (readyCount < 0) {
        return readyCount;
    }

    for (int index = 0; index < readyCount; ++index) {
        const auto eventIndex = static_cast<size_t>(index);
        events[eventIndex].fd = static_cast<int>(nativeEvents[eventIndex].ident);
        events[eventIndex].events = None;
        if (nativeEvents[eventIndex].filter == EVFILT_READ)
            events[eventIndex].events |= Event::Read;
        if (nativeEvents[eventIndex].filter == EVFILT_WRITE)
            events[eventIndex].events |= Event::Write;
        if ((nativeEvents[eventIndex].flags & (EV_ERROR | EV_EOF)) != 0U) {
            events[eventIndex].events |= Event::Error;
        }
    }
    return readyCount;
#endif
}

int setNonBlocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

}  // namespace net
