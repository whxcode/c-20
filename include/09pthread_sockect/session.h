#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "include/09pthread_sockect/session.h"
#include "include/09pthread_sockect/tools.h"
#include "include/common.h"

class PSession {
public:
    PSession(int sfd, int _fd, std::weak_ptr<IPServer> server)
        : sfd(sfd), fd(_fd), pServer(server) {
        // 启动读写线程
        //

        std::thread readThread{[this]() {
            char buf[1024]{0};
            auto size = sizeof(buf);
            while (true) {
                memset(buf, 0, size);
                auto readN = recv(fd, buf, size, 0);

                if (readN == 0) {
                    // 客户端推出
                    pServer.lock()->closeSession(fd);
                    return;
                }

                std::cout << buf << std::endl;
                std::string str;
                str.assign(buf, (size_t)readN);
                auto [left, right] = tools::splitAndParse(str);

                {
                    std::unique_lock lg{mtx};
                    message = right;
                    cacheFds = left;

                    cv.notify_one();
                }
            }
        }};

        // 读线程;接受用户的数据,
        // 1、选择 好友，进入对话。不用二次确认；
        std::thread writeThread{[this]() {
            while (true) {
                // 等待消息到来
                std::unique_lock lg{mtx};
                cv.wait(lg, [this]() {
                    return cacheFds.size() != 0;
                });

                message.insert(0, "[" + std::to_string(fd) + "]说:");

                for (auto f : cacheFds) {
                    send(f, message.c_str(), message.size(), 0);
                }

                cacheFds.clear();
            }
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
    std::condition_variable cv{};
    std::mutex mtx{};
    std::string message{};  // 好友消息转发。
    std::vector<int> cacheFds{};
};
