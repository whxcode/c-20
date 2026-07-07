#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "include/wrap/wrap.h"

template <typename T>
using s_ptr = std::shared_ptr<T>;

class IPServer : public std::enable_shared_from_this<IPServer> {
public:
    virtual void closeSession(const int fd) = 0;
};

using EpollHandle = std::function<void(const int fd)>;

using EpollEvent = struct {
    int fd{0};
    EpollHandle handle{nullptr};
};

class Epoll {
public:
    Epoll() {
        epfd = epoll_create(1);
    }
    // 处理析构函数
    ~Epoll() {
        for (auto& [fd, event] : events) {
            close(fd);
            delete event;
        }

        close(epfd);
    }

public:
    void attach(const int fd, EpollHandle&& handle) {
        epoll_event ev{};
        auto e = new EpollEvent;

        e->fd = fd;
        e->handle = std::move(handle);
        ev.events = EPOLLIN;
        ev.data.ptr = e;
        events[fd] = e;

        // 注册
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }

    void wait() {
        // 就绪列表
        epoll_event events[1024]{};
        auto nread = epoll_wait(epfd, events, 1024, -1);

        for (size_t i = 0; i < (size_t)nread; ++i) {
            EpollEvent* event = (EpollEvent*)events[i].data.ptr;
            auto fd = event->fd;

            if (fd == 0) {
                continue;
            }

            event->handle(fd);
        }

        _detach();
        // 关闭
    }

    void detach(const int fd) {
        closefds.insert(fd);
    }

private:
    void _detach() {
        for (auto fd : closefds) {
            auto it = events.find(fd);
            if (it == events.end()) {
                return;
            }

            auto second = it->second;

            close(fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            events.erase(fd);

            delete second;
        }
        closefds.clear();
    }

private:
    int epfd{0};
    std::set<int> closefds{};
    std::unordered_map<int, EpollEvent*> events{};
};

namespace tools {

static int MSocket(const int listenNum) {
    // 服务的

    auto sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt{1};
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (sfd == -1) {
        perror("socket: ");
        return -1;
    }

    sockaddr_in addr{.sin_family = AF_INET,
                     .sin_port = htons(8081),
                     .sin_addr =
                         {
                             .s_addr = htonl(INADDR_ANY),
                         }

    };

    size_t addrLen = sizeof(addr);

    if (bind(sfd, (sockaddr*)&addr, addrLen) < 0) {
        perror("bind: ");
        return -1;
    }

    if (listen(sfd, listenNum) < 0) {
        perror("listen: ");
        return -1;
    }

    return sfd;
}

static int Readn(int fd, char* buf, int n) {
    // 需要读取多少个字节
    int llfet{n};
    char* head{buf};

    // 继续读取
    while (llfet > 0) {
        auto readLen = recv(fd, (void*)head, (size_t)llfet, 0);

        if (readLen < 0) {
            if (errno == EINTR) {
                continue;
            }

            perror("Readn error,[readLen < 0]: ");
        } else if (readLen == 0) {
            // 对端关闭了连接,返回读取的字节说
            return (n - llfet);
        }

        head += readLen;
        llfet -= readLen;
    }

    if (llfet != 0) {
        perror("readn error lleft != 0: ");
    }

    return n;
}

static std::tuple<bool, std::vector<int>, std::string> splitAndParse(const std::string& input) {
    auto pos = input.find(',');

    if (pos == std::string::npos) {
        return {false, {}, ""};
    }

    // 解析前半部分 "1,2,3,4"
    std::string numPart = input.substr(0, pos);
    std::vector<int> nums;
    std::stringstream ss(numPart);
    std::string token;
    while (std::getline(ss, token, ',')) {
        nums.push_back(std::stoi(token));
    }

    // 解析后半部分 "faefaw"
    std::string strPart = (pos != std::string::npos) ? input.substr(pos + 1) : "";

    return {true, nums, strPart};
}

static int tcp(const size_t port) {
    sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(8081),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };

    auto lfd = Wrap::Socket(addr.sin_family, SOCK_STREAM, 0);
    int opt{1};

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Wrap::Bind(lfd, (sockaddr*)&addr, sizeof(addr));
    Wrap::Listen(lfd, 10);

    return lfd;
}

}  // namespace tools
