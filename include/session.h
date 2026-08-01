#pragma once
#include <string>

#include "include/handle.h"
#include "include/protocol/http.h"
#include "include/readerbuffer.h"

class Session : public IHandle {
public:
    static Session* Make(const int fd, ISessionManager* m);

public:
    Session(const int fd, ISessionManager* m);
    ~Session() = default;

public:
    void handle(sp<Ctx>& ctx) override;

private:
    void processRequest();

private:
    int cfd{0};
    char buf[1024]{0};
    ISessionManager* manager{nullptr};
    HttpProtocol http;
    size_t headerSize{sizeof(uint32_t)};
    ReadBuffer buffer{};
    bool headerParsed{false};
    size_t bodyNeed{0};
};
