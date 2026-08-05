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
    // cEpWorker = std::make_shared<EpWorker>(this);

    for (size_t i = 0; i < cEpWorkerCount; ++i) {
        cEpWorker.push_back(std::make_shared<EpWorker>(this));
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
    cGetHandlers[path] = handle;
}

void Server::post(const HttpPath& path, const ResponseHttpHandle& handle) {
    cPostHandlers[path] = handle;
}

void Server::useNotFound(const HttpPath&, const HttpHandle& handle) {
    cNotFoundHandler = handle;
}

void Server::useStaticServer(const HttpPath& path, const HttpHandle& handle) {
    cStaticPath = path;
    cStaticHandler = handle;
}

void Server::initializeEventLoop() {
    cEventFd = net::create();
    cListenFd = tcp::createListener(8081, cMaxEvents);
    net::setNonBlocking(cListenFd);

    // net::ctl(cEventFd, net::Operation::Add, cWake.readFd(), net::Read | net::EdgeTriggered);
    // net::ctl(cEventFd, net::Operation::Add, cWake.writeFd(), net::Write | net::EdgeTriggered);
    net::ctl(cEventFd, net::Operation::Add, cListenFd, net::Read | net::EdgeTriggered);
}

Server::ResponseHttpHandle* Server::getResponseHttpHandle(sp<Ctx>& context) {
    const auto& request = context->request;
    if (request.method == HttpMethod::cGet) {
        const auto it = cGetHandlers.find(request.path);
        if (it != cGetHandlers.end()) {
            return &it->second;
        }
    } else if (request.method == HttpMethod::cPost) {
        const auto it = cPostHandlers.find(request.path);
        if (it != cPostHandlers.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

void Server::enqueueRequest(sp<Ctx> context, const CallBack& callback) {
    const std::string requestPath = context->request.path;
    auto handle = getResponseHttpHandle(context);

    if (handle == nullptr) {
        if (cNotFoundHandler) {
            cNotFoundHandler(context);
            return;
        }

        context->setStatusCode(HttpStatusCode::cNoFound);
        context->setRaw("404 Not Found");
        context->send();
        return;
    }

    cWorkers.post([this, context, handle, callback] {
        auto rest = (*handle)(context->request);

        if (context->isCancelled()) {
            return;
        }

        callback(std::move(rest));
    });
}

void Server::dispatchRequest(sp<Ctx>& context, const CallBack& callback) {
    const auto& request = context->request;

    if (request.method == HttpMethod::cGet) {
        if (request.path.starts_with(cStaticPath) && cStaticHandler) {
            cStaticHandler(context);
            return;
        }
    }

    enqueueRequest(context, callback);
}

void Server::StaticHandle(sp<Ctx>& context) {
    namespace stdfs = std::filesystem;

    std::string relativePath = context->request.path;
    if (!relativePath.empty() && relativePath.front() == '/') {
        relativePath.erase(0, 1);
    }

    const auto filePath = (stdfs::current_path() / ".." / relativePath).lexically_normal();
    context->setResponse(Response::MakeFile(filePath));
    context->send();
}
