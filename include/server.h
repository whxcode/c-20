#pragma once

#include <string>

#include "include/io-worker.h"
#include "include/queue/queue.h"
#include "include/router.h"

class Server {
public:
    using ResponseHttpHandle = Router::Handler;
    using HttpPath = std::string;

    ~Server();

    static Response StaticHandle(const HttpRequest& request);

    void run();
    void stop();

    void get(const HttpPath& path, const ResponseHttpHandle& handle);
    void post(const HttpPath& path, const ResponseHttpHandle& handle);
    void useNotFound(const HttpPath& path, const ResponseHttpHandle& handle);
    void useStaticServer(const HttpPath& path, const ResponseHttpHandle& handle);
    void acceptClients();

private:
    void initializeEventLoop();

private:
    int cListenFd{-1};
    int cEventFd{-1};
    int cMaxEvents{1024};
    bool cStopped{false};

    Router cRouter{};
    Workers cWorkers{};
    std::vector<sp<EpWorker>> cEpWorker{};
    size_t cEpWorkerCount{10};
    size_t cEpWorkerIndex{0};
};
