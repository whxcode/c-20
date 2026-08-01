#pragma once

#include "include/ctx.h"
class IHandle {
public:
    virtual void handle(sp<Ctx>& ctx) = 0;
};

class ISessionManager {
public:
    virtual void detach(const int fd) = 0;
};
