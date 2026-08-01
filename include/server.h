#pragma once

#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>

#include "include/ctx.h"
#include "include/handle.h"
#include "include/queue/queue.h"
#include "include/readerbuffer.h"
#include "include/tools/net.h"
#include "include/wake.h"

struct RequestReadState {
    ReadBuffer buffer{};
    bool headerParsed{false};
    size_t bodyBytesPending{0};
    HttpProtocol protocol{};
    TimeLine::TimePoint cStartedAt{TimeLine::Clock::now()};
};

struct ResponseData {
    sp<Ctx> context{};
    std::string body{};
};

class Server : public IHandle, ISessionManager {
public:
    using HttpHandle = std::function<void(sp<Ctx>& context)>;
    using ResponseHttpHandle = std::function<ResponseData(sp<Ctx> context)>;
    using HttpPath = std::string;

    ~Server();

    static void StaticHandle(sp<Ctx>& context);

    void run();
    void stop();

    void get(const HttpPath& path, const ResponseHttpHandle& handle);
    void post(const HttpPath& path, const ResponseHttpHandle& handle);
    void useNotFound(const HttpPath& path, const HttpHandle& handle);
    void useStaticServer(const HttpPath& path, const HttpHandle& handle);

    void handle(sp<Ctx>& context) override;
    void detach(int fd) override;

private:
    void initializeEventLoop();
    void dispatchEvent(const net::ReadyEvent& event);
    void handleWakeup();
    void handleReadable(int fd);
    void handleWritable(int fd);
    void acceptClients();
    void enqueueRequest(sp<Ctx> context);
    void dispatchRequest(sp<Ctx>& context);
    sp<Ctx> readRequest(int clientFd);
    void closePendingSessions();

    ResponseHttpHandle* getResponseHttpHandle(sp<Ctx>& context);

private:
    int cListenFd{-1};
    int cEventFd{-1};
    int cMaxEvents{1024};
    bool cStopped{false};

    std::set<int> cPendingClose{};
    std::unordered_map<int, sp<RequestReadState>> cReadStates{};
    std::unordered_map<int, sp<Ctx>> cContexts{};

    std::unordered_map<HttpPath, ResponseHttpHandle> cGetHandlers{};
    std::unordered_map<HttpPath, ResponseHttpHandle> cPostHandlers{};
    HttpHandle cNotFoundHandler{};
    HttpPath cStaticPath{};
    HttpHandle cStaticHandler{};

    std::mutex cCompletionMutex{};
    std::queue<ResponseData> cCompletions{};

    Wake cWake{};
    Workers cWorkers{};
};
