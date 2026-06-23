
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "include/08_pthread/m_pathred.h"

namespace Base {
union {
    short s{0};
    char c[sizeof(short)];
} un2;

union {
    int s{0};
    char c[sizeof(short)];
} un4;

static void test01() {
    printf("[%d],[%d],[%d]\n", sizeof(short), sizeof(int), sizeof(long int));
    un2.s = 0x0102;  // 0x102 = ? 16*16+2

    printf("%d,%d,%d\n", un2.c[0], un2.c[1], un2.s);

    un4.s = 0x1020304;

    printf("%d,%d,%d,%d,%d\n", un4.c[0], un4.c[1], un4.c[2], un4.c[3], un4.s);
}

static void test02() {
    std::uint32_t x = 0x12345678;  // 从左到右: 高位->地低
                                   // 0x12 高
                                   // 0x34  |
                                   // 0x56  |
                                   // 0x78 低
    unsigned char* p = reinterpret_cast<unsigned char*>(&x);

    if (p[0] == 0x78) {
        std::cout << "little-endian\n";
    } else if (p[0] == 0x12) {
        std::cout << "big-endian\n";
    }
}

static void test03() {
    std::uint32_t x = 0x12345678;
    std::uint32_t y = htonl(x);  // host to network long
    std::uint32_t z = ntohl(y);  // host to network long

    printf("x: 0x%x, y: 0x%x\n", x, y);
    printf("%d,%d\n", x, z);
}

static void test04() {
    size_t i;
    inet_pton(AF_INET, "127.0.0.1", &i);
    printf("i[%x]\n", (char*)i);
}

static void test05() {
    // 创建文件描述符
    auto sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        perror("socket");
        return;
    }

    // 将 sfd 与 ip+port 绑定
    sockaddr_in addr;
    addr.sin_port = htons(8081);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;

    auto b = bind(sfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (b == -1) {
        perror("bind");
        return;
    }

    // 监听 ip + port
    auto l = listen(sfd, 10);
    if (l == -1) {
        perror("listen");
        return;
    }

    // 等待客户端链接
    sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);
    printf("等待链接!\n");
    auto cfd = accept(sfd, reinterpret_cast<struct sockaddr*>(&caddr), &caddr_len);
    if (cfd == -1) {
        perror("accept");
        return;
    }

    printf("链接成功!\n");
    char buf[1024];
    inet_ntop(AF_INET, &caddr.sin_addr.s_addr, buf, sizeof(buf));

    std::cout << "id:" << buf << std::endl;
    std::cout << "prot:" << ntohs(caddr.sin_port) << std::endl;

    while (1) {
        memset(buf, 0, sizeof(buf));
        recv(cfd, buf, sizeof(buf), 0);
        printf("收到数据: %s\n", buf);
        send(cfd, "hello", 5, 0);
    }
}

static void test06() {
    /**
     * 1.创建 socket，返回一个 文件描述符 (sfd) -- socket()
     *    sfd 用于监听客户端链接
     * 2.将 sfd 和 ip+port 绑定 -- bind()
     * 3.将 sfd 将主动变为被动 -- listen()
     * 4.监听是否由客户端链接 -- accept() 返回通信文件描述符 cfd (用于与客户端通信)
     * 5. 使用 read或recv -- 读取数据
     * 6. 使用 write或send -- 写入数据
     * 7. 使用 close(sfd)，close(cfd) 关闭文件描述符
     * */
    auto sfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sAddr{.sin_family = AF_INET,
                      .sin_port = htons(8080),
                      .sin_addr = {
                          .s_addr = htonl(INADDR_ANY),
                      }};

    if (bind(sfd, (sockaddr*)&sAddr, sizeof(sAddr)) == -1) {
        perror("bind");
    }

    if (listen(sfd, 10) == -1) {
        perror("listen");
    }

    sockaddr_in cAddr;
    socklen_t cAddrLen = sizeof(cAddr);

    auto cfd = accept(sfd, (sockaddr*)&cAddr, &cAddrLen);

    if (cfd == -1) {
        perror("accept");
    }

    send(cfd, "hello\n", 6, 0);
}

static void test07() {
    // tcp 程序
    // 1. 创建 socket
    auto lfd = socket(AF_INET, SOCK_STREAM, 0);

    if (lfd < 0) {
        perror("socket");
        return;
    }

    // 2.绑定
    sockaddr_in serv;
    auto servLen = sizeof(serv);

    bzero(&serv, servLen);

    serv.sin_family = AF_INET;
    serv.sin_port = htons(8080);               // short 型
    serv.sin_addr.s_addr = htonl(INADDR_ANY);  // int 型

    if (bind(lfd, (sockaddr*)&serv, servLen) == -1) {
        perror("bind");
    }

    if (listen(lfd, 10) == -1) {
        perror("listen");
    }

    auto cfd = accept(lfd, nullptr, nullptr);

    printf("lfd[%d], cfd[%d]\n", lfd, cfd);

    int i{0};
    int n{0};
    char buf[1024]{0};

    while (1) {
        // 接受数据
        memset(buf, 0, sizeof(buf));
        n = recv(cfd, buf, sizeof(buf), 0);
        printf("n == [%d],buf =[%s]\n", n, buf);

        for (i = 0; i < n; ++i) {
            buf[i] = toupper(buf[i]);
        }

        // 写回数据
        send(cfd, buf, (size_t)n, 0);
    }
}

}  // namespace Base

void MScokect() {
    Base::test07();
    // Base::test06();
    // Base::test05();
    // Base::test04();
    // Base::test03();
    // Base::test02();
    // Base::test01();
}
