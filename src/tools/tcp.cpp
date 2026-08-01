#include "include/tools/tcp.h"

#include <arpa/inet.h>   // inet_addr, inet_ntoa
#include <netinet/in.h>  // sockaddr_in, htons, htonl
#include <sys/socket.h>  // socket, bind, listen, accept
#include <unistd.h>      // close

#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "include/tools/net.h"
//
int tcp::createListener(int port, int backlog) {
    sockaddr_in server{
        .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = {.s_addr = htonl(INADDR_ANY)}};

    int lfd{0};

    if ((lfd = socket(server.sin_family, SOCK_STREAM, 0)) < 0) {
        perror("sockect() error:");
        exit(-1);
    }

    int opt{1};

    // 设置端口复用
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() error:");
        exit(-1);
    }

    if (bind(lfd, (sockaddr*)&server, sizeof(server)) < 0) {
        perror("setsockopt() error:");
        exit(-1);
    }

    if (listen(lfd, backlog) < 0) {
        perror("listen() error:");
        exit(-1);
    }

    return lfd;
}

int tcp::acceptClient(int listenerFd) {
    int cfd = ::accept(listenerFd, nullptr, nullptr);

    if (cfd >= 0) {
        net::setNonBlocking(cfd);
    }

    return cfd;
}
