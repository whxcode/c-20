#include "include/server.h"

#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "include/consts/http_code.h"
#include "include/tools/file.h"
#include "include/tools/net.h"
#include "include/tools/tcp.h"

Server::~Server() {
    stop();
}

void Server::run() {
    for (size_t i = 0; i < cEpWorkerCount; ++i) {
        cEpWorker.push_back(std::make_shared<EpWorker>(cRouter, cWorkers));
    }

    initializeEventLoop();

    std::vector<net::ReadyEvent> events(static_cast<size_t>(cMaxEvents));

    while (!cStopped) {
        const int readyCount = net::wait(cEventFd, events.data(), cMaxEvents, -1);

        if (readyCount < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (!cStopped) {
                std::perror("event wait");
            }

            break;
        }

        for (size_t index = 0; index < readyCount; ++index) {
            auto fd = events[index].fd;

            if (fd == cListenFd) {
                acceptClients();
            }
        }
    }
}

void Server::acceptClients() {
    while (true) {
        int clientFd = tcp::acceptClient(cListenFd);

        if (clientFd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            file::printError("Server::acceptClients");

            break;
        }
        cEpWorker[cEpWorkerIndex++ % cEpWorkerCount]->listen(clientFd);
    }
    //
}

void Server::stop() {
    if (cStopped) {
        return;
    }

    cStopped = true;
    cWorkers.close();

    if (cListenFd >= 0) {
        ::close(cListenFd);
        cListenFd = -1;
    }
    if (cEventFd >= 0) {
        ::close(cEventFd);
        cEventFd = -1;
    }
}

void Server::get(const HttpPath& path, const ResponseHttpHandle& handle) {
    cRouter.get(path, handle);
}

void Server::post(const HttpPath& path, const ResponseHttpHandle& handle) {
    cRouter.post(path, handle);
}

void Server::useNotFound(const HttpPath&, const ResponseHttpHandle& handle) {
    cRouter.setNotFound(handle);
}

void Server::useStaticServer(const HttpPath& path, const ResponseHttpHandle& handle) {
    cRouter.setStatic(path, handle);
}

void Server::initializeEventLoop() {
    cEventFd = net::create();
    cListenFd = tcp::createListener(8081, cMaxEvents);
    net::setNonBlocking(cListenFd);

    // net::ctl(cEventFd, net::Operation::Add, cWake.readFd(), net::Read | net::EdgeTriggered);
    // net::ctl(cEventFd, net::Operation::Add, cWake.writeFd(), net::Write | net::EdgeTriggered);
    net::ctl(cEventFd, net::Operation::Add, cListenFd, net::Read | net::EdgeTriggered);
}

Response Server::StaticHandle(const HttpRequest& request) {
    namespace stdfs = std::filesystem;

    std::string relativePath = request.path;
    if (!relativePath.empty() && relativePath.front() == '/') {
        relativePath.erase(0, 1);
    }

    const auto filePath = (stdfs::current_path() / ".." / relativePath).lexically_normal();
    return Response::MakeFile(filePath.string());
}
