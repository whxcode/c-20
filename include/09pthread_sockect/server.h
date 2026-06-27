#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/_pthread/_pthread_mutex_t.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#include "include/09pthread_sockect/session.h"
#include "include/09pthread_sockect/tools.h"
#include "include/common.h"

class PServers {
public:
    void listen() {
        this->sfd = tools::MSocket(10);

        while (1) {
            auto cfd = accept(sfd, nullptr, nullptr);

            if (cfd == -1) {
                perror("accept:");
                continue;
            }

            attach(cfd);
        }
    }

private:
    void attach(const int cfd) {
        std::cout << cfd << ": 加入" << std::endl;
        auto session = std::make_shared<PSession>(sfd, cfd);
        sessions.insert({cfd, session});
        // 1、通知所有人有人加入了

        for (auto& [fd, session] : sessions) {
            if (fd != cfd) {
                session->sendMessage("欢迎: " + std::to_string(cfd) + ":加入");
            }
        }

        // 给 加入进来的人发送所有在线的人的信息,只发送文件描述分: eg: 1,2,3,4

        std::string msg{"当前在线好友:"};

        for (auto& [fd, _] : sessions) {
            if (fd != cfd) {
                msg += std::to_string(fd) + ",";
            }
        }

        // 发送
        session->sendMessage(msg);
        std::cout << cfd << ": 加入:done" << std::endl;
    }

    void detach(const int cfd) {
        sessions.erase(cfd);

        std::cout << "欢迎: " << cfd << ":离开" << std::endl;
        std::cout << "现在聊天人数:" << sessions.size() << std::endl;
    }

    s_ptr<PSession> getSession(const int cfd) {
        auto it = sessions.find(cfd);

        if (it != sessions.end()) {
            return it->second;
        }

        return nullptr;
    }

private:
    std::unordered_map<int, s_ptr<PSession>> sessions{};
    int sfd{0};
};
