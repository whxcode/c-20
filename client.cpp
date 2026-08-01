#include <netinet/in.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <ostream>

#include "include/protocol/http.h"

int main(int argc, char* argv[]) {
    HttpProtocol http{};
    http.host = "192.168.0.1:8081";
    http.url = "/userlist";
    http.contentLength = 100;

    kiwi::ByteBuffer bb;
    http.encode(bb);

    std::cout << "hello" << std::endl;
    sockaddr_in server{.sin_family = AF_INET,
                       .sin_port = htons(8081),
                       .sin_addr = {.s_addr = htonl(INADDR_ANY)

                       }};

    auto cfd = socket(server.sin_family, SOCK_STREAM, 0);
    auto lfd = connect(cfd, (sockaddr*)&server, sizeof(server));

    if (lfd < 0) {
        perror("connect error");
        return -1;
    }

    size_t n{0};
    size_t total{0};
    uint32_t len = htonl(bb.size());
    uint32_t dataLen = sizeof(uint32_t);

    while (n < dataLen) {
        auto sendn = send(cfd, (const char*)&len + n, dataLen - n, 0);

        if (sendn < 0) {
            perror("send error\n");
            break;
        }

        if (sendn == 0) {
            std::cout << "send 0 bytes" << std::endl;
            break;
        }

        n += (size_t)sendn;
    }

    total += n;
    n = 0;

    while (n < bb.size()) {
        auto sendn = send(cfd, bb.data() + n, bb.size() - n, 0);

        if (sendn < 0) {
            perror("send error\n");
            break;
        }

        if (sendn == 0) {
            std::cout << "send 0 bytes" << std::endl;
            break;
        }

        n += (size_t)sendn;
    }

    total += n;

    std::cout << "总共发送 bytes:" << total << std::endl;
    std::cout << "数据包含:" << bb.size() << std::endl;

    return 0;
}
