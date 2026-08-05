#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "include/ctx.h"
#include "include/io-worker.h"
#include "include/protocol/http.h"
#include "include/queue/queue.h"

class Server : public IDispatch {
public:
    using HttpHandle = std::function<void(sp<Ctx>& context)>;
    using ResponseHttpHandle = std::function<Response(const HttpRequest& request)>;
    using HttpPath = std::string;

    ~Server();

    static void StaticHandle(sp<Ctx>& context);

    void run();
    void stop();

    void get(const HttpPath& path, const ResponseHttpHandle& handle);
    void post(const HttpPath& path, const ResponseHttpHandle& handle);
    void useNotFound(const HttpPath& path, const HttpHandle& handle);
    void useStaticServer(const HttpPath& path, const HttpHandle& handle);

    void enqueueRequest(sp<Ctx> context, const CallBack& callback) override;
    void dispatchRequest(sp<Ctx>& context, const CallBack& callback) override;
    void acceptClients();

private:
    void initializeEventLoop();

    ResponseHttpHandle* getResponseHttpHandle(sp<Ctx>& context);

private:
    int cListenFd{-1};
    int cEventFd{-1};
    int cMaxEvents{1024};
    bool cStopped{false};

    std::unordered_map<HttpPath, ResponseHttpHandle> cGetHandlers{};
    std::unordered_map<HttpPath, ResponseHttpHandle> cPostHandlers{};
    HttpHandle cNotFoundHandler{};
    HttpPath cStaticPath{};
    HttpHandle cStaticHandler{};

    Workers cWorkers{};
    std::vector<sp<EpWorker>> cEpWorker{};
    size_t cEpWorkerCount{10};
    size_t cEpWorkerIndex{0};
};
