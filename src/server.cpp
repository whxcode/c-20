#include "include/server.h"

#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>

#include "include/consts/http_code.h"
#include "include/tools/net.h"
#include "include/tools/tcp.h"

Server::~Server() {
    stop();
}

void Server::run() {
    initializeEventLoop();
    std::vector<net::ReadyEvent> events(static_cast<size_t>(cMaxEvents));

    while (!cStopped) {
        const int readyCount = net::wait(cEventFd, events.data(), cMaxEvents, -1);
        std::cout << "readyCount:" << readyCount << std::endl;

        if (readyCount < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (!cStopped) {
                std::perror("event wait");
            }
            break;
        }

        for (int index = 0; index < readyCount; ++index) {
            dispatchEvent(events[index]);
        }

        closePendingSessions();

        std::cout << "本轮处理完毕" << std::endl;
    }
}

void Server::stop() {
    if (cStopped) {
        return;
    }

    cStopped = true;
    cWake.notify();
    cWorkers.close();

    for (const auto& [fd, context] : cContexts) {
        context->cancel();
        net::ctl(cEventFd, net::Operation::Delete, fd, net::None);
        ::close(fd);
    }
    cContexts.clear();
    cReadStates.clear();
    cPendingClose.clear();

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

void Server::handle(sp<Ctx>&) {
    acceptClients();
}

void Server::detach(int fd) {
    cPendingClose.insert(fd);
}

void Server::initializeEventLoop() {
    cEventFd = net::create();
    cListenFd = tcp::createListener(8081, cMaxEvents);
    net::setNonBlocking(cListenFd);

    net::ctl(cEventFd, net::Operation::Add, cWake.readFd(), net::Read | net::EdgeTriggered);
    // net::ctl(cEventFd, net::Operation::Add, cWake.writeFd(), net::Write | net::EdgeTriggered);
    net::ctl(cEventFd, net::Operation::Add, cListenFd, net::Read | net::EdgeTriggered);
}

void Server::dispatchEvent(const net::ReadyEvent& event) {
    if (event.fd == cWake.readFd()) {
        handleWakeup();
        return;
    }

    if (event.events & net::Error) {
        detach(event.fd);
        return;
    }

    if (event.events & net::Read) {
        handleReadable(event.fd);
    }
    if (event.events & net::Write) {
        handleWritable(event.fd);
    }
}

void Server::handleWakeup() {
    cWake.consume();

    std::queue<ResponseData> readyResponses;
    {
        std::lock_guard lock(cCompletionMutex);
        readyResponses.swap(cCompletions);
    }

    while (!readyResponses.empty()) {
        auto completion = std::move(readyResponses.front());
        readyResponses.pop();

        if (completion.context->isCancelled()) {
            continue;
        }

        completion.context->setRaw(completion.body);
        completion.context->send();
    }
}

void Server::handleReadable(int fd) {
    if (fd == cListenFd) {
        acceptClients();
        return;
    }

    auto context = readRequest(fd);
    if (!context) {
        return;
    }

    std::cout << "context:" << context->request.path << std::endl;

    cContexts[fd] = context;
    dispatchRequest(context);
}

void Server::handleWritable(int fd) {
    const auto context = cContexts.find(fd);
    if (context != cContexts.end()) {
        context->second->write();
    }
}

void Server::acceptClients() {
    for (;;) {
        const int clientFd = tcp::acceptClient(cListenFd);
        if (clientFd >= 0) {
            net::ctl(cEventFd, net::Operation::Add, clientFd, net::Read | net::EdgeTriggered);
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::perror("accept");
        }
        return;
    }
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

void Server::enqueueRequest(sp<Ctx> context) {
    const std::string requestPath = context->request.path;
    auto handle = getResponseHttpHandle(context);

    std::cout << "客户端开始处理器请求" << std::endl;

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

    cWorkers.post([this, context, handle] {
        auto rest = (*handle)(context);

        if (context->isCancelled()) {
            return;
        }

        ResponseData completion{context, rest.body};
        {
            std::lock_guard lock(cCompletionMutex);
            cCompletions.push(std::move(completion));
        }

        cWake.notify();
    });
}

void Server::dispatchRequest(sp<Ctx>& context) {
    const auto& request = context->request;

    if (request.method == HttpMethod::cGet) {
        if (request.path.starts_with(cStaticPath) && cStaticHandler) {
            cStaticHandler(context);
            return;
        }
    }

    enqueueRequest(context);
}

sp<Ctx> Server::readRequest(int clientFd) {
    auto& readState = cReadStates[clientFd];
    if (!readState) {
        readState = std::make_shared<RequestReadState>();
    }

    const ssize_t readSize = readState->buffer.readfd(clientFd, nullptr);
    if (readSize < 0) {
        detach(clientFd);
        return nullptr;
    }
    if (readSize == 0 && readState->buffer.empty()) {
        detach(clientFd);
        return nullptr;
    }

    if (!readState->headerParsed) {
        const std::string requestData(reinterpret_cast<char*>(readState->buffer.head()),
                                      readState->buffer.size());
        constexpr std::string_view headerEnd{"\r\n\r\n"};
        const auto headerEndOffset = requestData.find(headerEnd);
        if (headerEndOffset != std::string::npos) {
            const size_t headerSize = headerEndOffset + headerEnd.size();
            readState->protocol.parser(readState->buffer.head(), headerSize);
            readState->buffer.retrieve(headerSize);

            auto& headers = readState->protocol.request.headers;
            readState->bodyBytesPending = headers.count("Content-Length")
                                              ? std::stoul(headers.headers.at("Content-Length"))
                                              : 0;
            readState->headerParsed = true;
        }
    }

    if (readState->headerParsed && readState->bodyBytesPending > 0 &&
        readState->buffer.size() >= readState->bodyBytesPending) {
        auto rawBody = readState->buffer.peek(readState->bodyBytesPending);
        auto& request = readState->protocol.request;
        request.body.raw.assign(rawBody.begin(), rawBody.end());
        request.body.contentType = request.headers["Content-Type"];
        request.body.parse();
        readState->buffer.retrieve(readState->bodyBytesPending);
        readState->bodyBytesPending = 0;
    }

    if (!readState->headerParsed || readState->bodyBytesPending != 0) {
        if (readSize == 0) {
            detach(clientFd);
        }
        return nullptr;
    }

    auto context =
        Ctx::Make(clientFd, cEventFd, std::move(readState->protocol), readState->cStartedAt);
    context->setErrorHandle([this](int fd, int) {
        detach(fd);
    });
    context->setCloseHandle([this](int fd, int) {
        detach(fd);
    });
    cReadStates.erase(clientFd);
    return context;
}

void Server::closePendingSessions() {
    for (const int fd : cPendingClose) {
        if (const auto context = cContexts.find(fd); context != cContexts.end()) {
            context->second->cancel();
            cContexts.erase(context);
        }
        cReadStates.erase(fd);
        net::ctl(cEventFd, net::Operation::Delete, fd, net::None);
        ::close(fd);
    }
    cPendingClose.clear();
}

void Server::StaticHandle(sp<Ctx>& context) {
    namespace stdfs = std::filesystem;

    std::string relativePath = context->request.path;
    if (!relativePath.empty() && relativePath.front() == '/') {
        relativePath.erase(0, 1);
    }

    const auto filePath = (stdfs::current_path() / ".." / relativePath).lexically_normal();
    context->setFile(filePath);
    context->send();
}
