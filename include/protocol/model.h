#pragma once
#include "include/kiwi/include/schema.h"

// 客户端和服务器的通行协议
#include <cstddef>

enum class Protocol : uint8_t {
    Http = 0,
    Sockect = 1,
};

class Model {
public:
    virtual Protocol getProtocol() = 0;
    virtual void decode(kiwi::ByteBuffer& byte) = 0;
    virtual void encode(kiwi::ByteBuffer& byte) = 0;
};
