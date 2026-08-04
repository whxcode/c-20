#pragma once

#include <sys/types.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "include/consts/http_code.h"
#include "include/protocol/http.h"

template <typename T>
using sp = std::shared_ptr<T>;

using ErrorHandle = std::function<void(int fd, int error)>;

class TimeLine {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit TimeLine(TimePoint startedAt = Clock::now());

    double elapsedSeconds() const;

private:
    TimePoint cStartedAt;
};

class Ctx : public std::enable_shared_from_this<Ctx> {
public:
    static sp<Ctx> Make(int clientFd, int eventFd, HttpProtocol&& protocol,
                        TimeLine::TimePoint startedAt);

    Ctx(int clientFd, int eventFd, HttpProtocol&& protocol, TimeLine::TimePoint startedAt);
    ~Ctx();

    // Request data is immutable after parsing and is available to route handlers.
    HttpRequest request{};

    Headers& responseHeaders();
    void setRaw(const std::string& body);
    void setFile(const std::string& filePath);
    void setStatusCode(HttpStatusCode statusCode);
    HttpStatusCode getStatusCode() const;
    std::string getStatusMessage() const;

    void send();
    void write();

    void setErrorHandle(ErrorHandle handle);
    void setCloseHandle(ErrorHandle handle);

    bool isCancelled() const;
    void cancel();

public:
    void setResponse(Response&& response);

private:
    enum class BodyType {
        Raw,
        File,
    };

    void prepareResponse();
    void enableWriteEvent();
    void disableWriteEvent();
    void finishResponse();
    void failResponse(int error);
    void closeBodyFile();

private:
    // Connection ownership
    int cClientFd{-1};
    int cEventFd{-1};
    std::atomic_bool cCancelled{false};
    TimeLine cTimeline;

    // Response metadata
    HttpStatusCode cStatusCode{HttpStatusCode::cOk};
    Headers cResponseHeaders{};
    std::string cResponseHeader{};

    // Response body source
    BodyType cBodyType{BodyType::Raw};
    std::string cRawBody{};
    std::string cFilePath{};
    int cFileFd{-1};

    // Incremental write state
    ssize_t cHeaderOffset{0};
    ssize_t cHeaderSize{0};
    off_t cBodyOffset{0};
    ssize_t cBodySize{0};
    ssize_t cTotalSize{0};
    ssize_t cSentSize{0};
    bool cWriteEventEnabled{false};

    ErrorHandle cErrorHandle{};
    ErrorHandle cCloseHandle{};
};
