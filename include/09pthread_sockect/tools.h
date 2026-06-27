#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>

template <typename T>
using s_ptr = std::shared_ptr<T>;

namespace tools {

static int MSocket(const int listenNum) {
    // 服务的

    auto sfd = socket(AF_INET, SOCK_STREAM, 0);
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
}  // namespace tools
