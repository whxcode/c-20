
#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>

#include "include/consts/http_code.h"
#include "include/ctx.h"
#include "include/handle.h"
#include "include/protocol/http.h"
#include "include/queue/queue.h"
#include "include/readerbuffer.h"
#include "include/tools/file.h"
#include "include/tools/net.h"
#include "include/wake.h"

struct RequestReadState {
    ReadBuffer buffer{};
    bool headerParsed{false};
    size_t bodyBytesPending{0};
    HttpProtocol protocol{};
    TimeLine::TimePoint cStartedAt{TimeLine::Clock::now()};
};

struct ITask {
    sp<Ctx> cCtx{};
    Response cResponse{};
};

using CallBack = std::function<void(Response&& response)>;

class IDispatch {
public:
    virtual void enqueueRequest(sp<Ctx> context, const CallBack& callback) = 0;
    virtual void dispatchRequest(sp<Ctx>& context, const CallBack& callback) = 0;
};

// 接受 fd,并监听
class EpWorker {
public:
    EpWorker(IDispatch* dispatch) : cDispatch(dispatch) {
        cEpfd = net::create();

        net::ctl(cEpfd, net::Operation::Add, cWake.readFd(), net::Read | net::EdgeTriggered);

        if (cEpfd < 0) {
            file::printError("IO Worker create epoll error");
        }

        std::thread([this]() {
            run();
        }).detach();
    }

public:
    void listen(const int fd) {
        net::ctl(cEpfd, net::Operation::Add, fd, net::Read | net::EdgeTriggered);
    }

    void run() {
        std::vector<net::ReadyEvent> events(static_cast<size_t>(cMaxEvents));

        while (!cStopped) {
            const int readyCount = net::wait(cEpfd, events.data(), cMaxEvents, -1);
            std::cout << "pid:" << std::this_thread::get_id() << std::endl;

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
                dispatchEvent(events[index]);
            }

            closePendingSessions();
        }
    }

    void handleWakeup() {
        cWake.consume();

        std::queue<ITask> readyResponses;
        {
            std::lock_guard lock(cCompletionMutex);
            readyResponses.swap(cCompletions);
        }

        while (!readyResponses.empty()) {
            auto completion = std::move(readyResponses.front());
            readyResponses.pop();

            if (completion.cCtx->isCancelled()) {
                continue;
            }

            completion.cCtx->setResponse(std::move(completion.cResponse));
            completion.cCtx->send();
        }
    }

    void dispatchEvent(const net::ReadyEvent& event) {
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

    void handleWritable(int fd) {
        const auto context = cContexts.find(fd);
        if (context != cContexts.end()) {
            context->second->write();
        }
    }

    void handleReadable(int fd) {
        auto context = readRequest(fd);
        if (!context) {
            return;
        }

        cContexts[fd] = context;
        cDispatch->dispatchRequest(context, [this, context](Response&& response) {
            ITask completion{context, std::move(response)};
            {
                std::lock_guard lock(cCompletionMutex);
                cCompletions.push(std::move(completion));
            }

            cWake.notify();
        });
    }

    void detach(int fd) {
        cPendingClose.insert(fd);
    }

    void closePendingSessions() {
        for (const int fd : cPendingClose) {
            if (const auto context = cContexts.find(fd); context != cContexts.end()) {
                auto path = context->second->request.path;
                std::cout << "客户端关闭:" << path << "::::" << fd << std::endl;

                context->second->cancel();
                cContexts.erase(context);
            }

            cReadStates.erase(fd);
            net::ctl(cEpfd, net::Operation::Delete, fd, net::None);
            ::close(fd);
        }

        cPendingClose.clear();
    }

    sp<Ctx> readRequest(int clientFd) {
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
            Ctx::Make(clientFd, cEpfd, std::move(readState->protocol), readState->cStartedAt);
        context->setErrorHandle([this](int fd, int) {
            detach(fd);
        });
        context->setCloseHandle([this](int fd, int) {
            detach(fd);
        });
        cReadStates.erase(clientFd);
        return context;
    }

private:
    int cEpfd{-1};
    int cMaxEvents{1024};
    bool cStopped{false};
    Wake cWake{};

    std::queue<ITask> cCompletions{};
    std::mutex cCompletionMutex{};

    std::set<int> cPendingClose{};
    std::unordered_map<int, sp<RequestReadState>> cReadStates{};
    std::unordered_map<int, sp<Ctx>> cContexts{};
    IDispatch* cDispatch{nullptr};
};
