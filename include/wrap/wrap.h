#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>

namespace Wrap {

static void PrintError(const char* desc) {
    perror(desc);
    exit(EXIT_FAILURE);
}

// 创建文件描述符
static int Socket(const int domain, const int type, const int protocol) {
    int sockfd = socket(domain, type, protocol);

    if (sockfd < 0) {
        PrintError("socket error:");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// 读取指定字节数的函数
static int ReadN(int fd, void* buf, size_t count) {
    char* head{(char*)buf};
    int lleft{(int)count};

    while (lleft > 0) {
        int n = recv(fd, buf, count, 0);

        if (n < 0) {
            if (errno == EINTR) {
                continue;  // 被系统信号中断，属于正常现象，继续重试 recv
            }

            return -1;
        }

        lleft -= n;
        head += n;
    }

    if (lleft > 0) {
        PrintError("Read lleft > 0:\n");
    }

    return count;
}

static int Read(int fd, void* buf, size_t bytes) {
    ssize_t n{0};
again:
    if ((n = read(fd, buf, bytes)) < 0) {
        if (errno == EINTR) {
            goto again;
        }
        return -1;
    }

    return n;
}

static int Write(int fd, const void* ptr, size_t nbytes, int __flags = 0) {
    int n{0};
    size_t lleft{nbytes};
    const char* head{(const char*)ptr};

    while (lleft > 0) {
        if ((n = send(fd, head, (size_t)lleft, __flags)) < 0) {
            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return nbytes - lleft;
            }

            return -1;
        }

        lleft -= (size_t)n;
        head += n;
    }

    if (lleft > 0) {
        Wrap::PrintError("Write lleft > 0:");
    }

    return nbytes - lleft;
}

static int Writen(int fd, const void* vptr, size_t n) {
    size_t nleft{n};
    ssize_t nwritten{0};
    const char* ptr{(char*)vptr};

    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR) {
                nwritten = 0;
            } else {
                return -1;
            }
        }

        nleft -= (size_t)nwritten;
        ptr += nwritten;
    }

    if (nleft > 0) {
        PrintError("Writen lleft > 0:");
    }

    return n;
}

static ssize_t MyRead(int fd, char* ptr) {
    static int readCnt{0};
    static char* readPtr{nullptr};
    static char* readBuf[100]{0};

    if (readCnt <= 0) {
    again:
        readCnt = read(fd, readBuf, sizeof(readBuf));

        if (readCnt < 0) {
            if (errno == EINTR) {
                goto again;
            }

            return -1;
        } else if (readCnt == 0) {
            // fd 已关闭
            return 0;
        }

        readPtr = (char*)readBuf;
    }

    readCnt--;
    *ptr = *readPtr++;

    return 1;
}

static ssize_t ReadLine(int fd, void* vptr, size_t maxLen) {
    ssize_t n{0};
    ssize_t rc{0};
    char c{0};
    char* ptr{(char*)vptr};

    for (n = 1; n < maxLen; n++) {
        if ((rc = MyRead(fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            *ptr = 0;

            return n - 1;
        } else {
            return -1;
        }
    }

    *ptr = 0;

    return n;
}

static int Bind(const int fd, const struct sockaddr* sa, socklen_t salen) {
    int n{0};

again:
    if ((n = bind(fd, sa, salen)) < 0) {
        if ((errno == ECONNABORTED) || errno == EINTR) {
            goto again;
        }

        PrintError("Bind");
    }

    return n;
}

static int Accpet(const int fd, struct sockaddr* sa, socklen_t* salenptr) {
    int n{0};
again:
    if ((n = accept(fd, sa, salenptr)) < 0) {
        if ((errno == ECONNABORTED) || errno == EINTR) {
            goto again;
        }
        PrintError("Accpet");
    }

    return n;
}

static int Listen(const int fd, const int num) {
    int n{0};

    if ((n = listen(fd, num)) < 0) {
        PrintError("Listen:");
    }

    return n;
}

static int Connect(int fd, const struct sockaddr* addr, socklen_t len) {
    int n{0};

    if ((n = connect(fd, addr, len)) < 0) {
        PrintError("Connect:");
    }

    return n;
}

static int TcpBind(uint16_t port) {
    sockaddr_in sAddr{
        .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = {.s_addr = htonl(INADDR_ANY)}};

    auto sLen = sizeof(sAddr);

    auto sfd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;

    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        exit(1);
    }

    Bind(sfd, (sockaddr*)&sAddr, sLen);

    Listen(sfd, 10);

    return sfd;
}

static int EpollCreate() {
    int root{-1};
#ifdef __linux__
    root = epoll_create(1);
#elif __APPLE__
    root = kqueue();
#endif
    return root;
}

static void EpollCtl(int epfd, int fd) {
#ifdef __linux__
    epoll_event ev{
        .events = EPOLLIN,
    };
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

#elif __APPLE__
    struct kevent change;
    EV_SET(&change, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    kevent(epfd, &change, 1, nullptr, 0, nullptr);
#endif
}

static std::vector<int> EpollAwait(int epfd) {
    std::vector<int> result{};

    static constexpr int kMaxEvents = 1024;
#ifdef __linux__
    epoll_event events[1024]{0};
    int nready = epoll_wait(epfd, events, 1024, -1);

    for (size_t i = 0; i < nready; ++i) {
        int fd = events[i].data.fd;
        result.push_back(fd);
    }

#elif __APPLE__
    std::array<struct kevent, kMaxEvents> events{};
    int nready = kevent(epfd, nullptr, 0, events.data(), events.size(), nullptr);
    for (size_t i = 0; i < nready; i++) {
        int fd = (int)(intptr_t)events[i].ident;
        result.push_back(fd);
    }

#endif
    return result;
}

static void EpollDetach(int epfd, const int fd) {
#ifdef __linux__
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
#elif __APPLE__
    struct kevent change;
    EV_SET(&change, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    kevent(epfd, &change, 1, nullptr, 0, nullptr);
#endif
}

};  // namespace Wrap
