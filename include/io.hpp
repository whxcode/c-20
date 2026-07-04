#pragma once

#include <sys/socket.h>

#include <array>
#include <vector>

#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#endif
#include "include/wrap/wrap.h"

class TCP {
public:
    TCP() {
#ifdef __linux__
        root = epoll_create(1);
#elif __APPLE__
        root = kqueue();
#endif
    }
    void attach(const int fd) {
#ifdef __linux__
        epoll ev{
            .events = EPOLLIN,
        };
        ev.data.fd = fd;
        epoll_ctl(root, EPOLL_CTL_ADD, fd, &ev);

#elif __APPLE__
        struct kevent change;
        EV_SET(&change, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        kevent(root, &change, 1, nullptr, 0, nullptr);
#endif
    }

    std::vector<int> await() {
        std::vector<int> result{};

        static constexpr int kMaxEvents = 1024;
#ifdef __linux__
        epoll_event events[1024]{0};
        int nready = epoll_wait(root, events, 1024, nullptr);

        for (size_t i = 0; i < nready; ++i) {
            int fd = events[i].data.fd;
            result.push_back(fd);
        }

#elif __APPLE__
        std::array<struct kevent, kMaxEvents> events{};
        int nready = kevent(root, nullptr, 0, events.data(), events.size(), nullptr);
        for (size_t i = 0; i < nready; i++) {
            int fd = (int)(intptr_t)events[i].ident;
            result.push_back(fd);
        }

#endif
        return result;
    }

    void detach(const int fd) {
#ifdef __linux__
        epoll_ctl(root, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
#elif __APPLE__
        struct kevent change;
        EV_SET(&change, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        kevent(root, &change, 1, nullptr, 0, nullptr);
        close(fd);
#endif
    }

private:
    int root{0};
};
