#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#include "include/09pthread_sockect/session.h"
#include "include/common.h"

class PSession {
public:
    PSession(int sfd, int fd) : sfd(sfd), fd(fd) {
        // 启动读写线程
        std::thread readThread{[]() {

        }};

        std::thread writeThread{[]() {

        }};

        readThread.detach();
        writeThread.detach();
    }

public:
    void sendMessage(const std::string& msg) {
        if (send(fd, msg.c_str(), msg.size(), 0) < 0) {
            perror("send error:");
        }
        std::cout << "写入完毕:" << fd << std::endl;
    }

private:
private:
    int sfd{0};  // 服务器的
    int fd{0};
};
