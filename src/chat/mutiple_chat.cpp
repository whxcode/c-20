#include "include/08chat/mutiple_chat.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

namespace {
int Accept(int serverFd) {
    // ... 你的服务器初始化和 listen 代码 ...

    // 1. 定义存储客户端地址信息的结构体
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);  // 必须初始化大小！

    // 2. 阻塞等待连接，同时获取客户端信息
    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);

    if (clientFd < 0) {
        perror("accept failed");
    } else {
        // 3. 解析客户端的 IP 和 端口
        char clientIp[INET_ADDRSTRLEN];  // 存储 IP 字符串的缓冲区

        // 将网络字节序的二进制 IP 转换为字符串
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);

        // 将网络字节序的端口转换为本地主机字节序的整数
        uint16_t clientPort = ntohs(clientAddr.sin_port);

        std::cout << "【新客户端连接】\n";
        std::cout << "IP 地址: " << clientIp << "\n";
        std::cout << "端口号 : " << clientPort << "\n";
        std::cout << "Socket FD: " << clientFd << "\n";
    }

    /*
    const char* h = "恭喜你，连接上了....";
    ssize_t bytes_written = write(clientFd, h, strlen(h));

    if (bytes_written < 0) {
        // 关键：打印 write 失败的具体原因
        std::cout << "Write 失败, 错误代码: " << errno << ", 错误原因: " << strerror(errno) << "\n";
    } else {
        std::cout << "成功写入 " << bytes_written << " 字节\n";
    }
  */

    return clientFd;
}

};  // namespace

void MutipleChat::server(int port) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverFd, (sockaddr*)&addr, sizeof(addr));
    listen(serverFd, 2);

    std::cout << "server listen port: " << port << "\n";

    //
    while (true) {
        int clientFd = Accept(serverFd);  // 阻塞，等待客户端连接

        std::thread([serverFd, clientFd]() {
            while (true) {
                char buffer[1024]{};
                auto bytelen = read(clientFd, buffer, sizeof(buffer));  // 阻塞等待客户端推送消息
                                                                        //
                if (bytelen > 0) {
                    std::string msg_v = "server: 已收到: [" + std::string(buffer) + "==> 共 " +
                                        std::to_string(bytelen) + "字节" + "]";
                    auto msg = msg_v.c_str();

                    write(clientFd, msg, strlen(msg));
                } else if (bytelen == 0) {
                    std::cout << "client closed 正常关闭,等待下一个幸运儿\n";
                    close(clientFd);
                    return;
                } else {
                    std::cerr << "read error 异常关闭 \n";
                    return;
                }
            }
        }).detach();
    }
}

void MutipleChat::client(int port) {
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(clientFd, (sockaddr*)&addr, sizeof(addr));  // 阻塞，等待客户端连接
                                                        //

    while (true) {
        std::string msg_s{};
        std::getline(std::cin, msg_s);  // 读取一整行，安全且自动扩容

        const char* msg = msg_s.c_str();
        write(clientFd, msg, strlen(msg));

        char buffer[1024]{};
        read(clientFd, buffer, sizeof(buffer));  // 阻塞，等待服务器推送消息

        std::cout << "server: " << buffer << "\n";
    }
}
