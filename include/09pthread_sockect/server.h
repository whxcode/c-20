#pragma once
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <unordered_map>

#include "include/09pthread_sockect/session.h"
#include "include/09pthread_sockect/tools.h"
#include "include/common.h"

class PServers : public IPServer {
public:
    void closeSession(const int _fd) override {
        if (sessions.find(_fd) == sessions.end()) {
            return;
        }

        sessions.erase(_fd);
        close(_fd);

        for (auto& [fd, session] : sessions) {
            session->sendMessage("欢迎: " + std::to_string(fd) + ":下次光临!\n");
        }
    }

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
        auto session = std::make_shared<PSession>(sfd, cfd, shared_from_this());
        sessions.insert({cfd, session});

        for (auto& [fd, session] : sessions) {
            if (fd != cfd) {
                session->sendMessage("欢迎: " + std::to_string(cfd) + ":加入\n");
            }
        }

        // 给 加入进来的人发送所有在线的人的信息,只发送文件描述分: eg: 1,2,3,4
        std::string msg{"当前在线好友:"};

        for (auto& [fd, _] : sessions) {
            if (fd != cfd) {
                msg += std::to_string(fd) + ",";
            }
        }

        msg += '\n';

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
