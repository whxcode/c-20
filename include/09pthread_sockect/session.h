#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>

#include "include/09pthread_sockect/session.h"
#include "include/09pthread_sockect/tools.h"
#include "include/common.h"

class PSession {
public:
    PSession(int sfd, int _fd, std::weak_ptr<IPServer> server)
        : sfd(sfd), fd(_fd), pServer(server) {
        // 启动读写线程
        std::thread readThread{[this]() {
            char c{0};
            while (true) {
                auto readN = recv(fd, &c, 1, 0);
                if (readN == 0) {
                    // 客户端推出
                    pServer.lock()->closeSession(fd);
                    return;
                }
            }
        }};

        std::thread writeThread{[]() {

        }};

        readThread.detach();
        writeThread.detach();
    }
    ~PSession() {
        std::cout << "~PSession" << std::endl;
    }

public:
    void sendMessage(const std::string& msg) {
        if (send(fd, msg.c_str(), msg.size(), 0) < 0) {
            perror("send error:");
        }

        // std::cout << "写入完毕:" << fd << std::endl;
    }

private:
    int sfd{0};  // 服务器的
    int fd{0};
    std::weak_ptr<IPServer> pServer{};
};
