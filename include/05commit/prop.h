#pragma once
#include <cassert>
#include <iostream>

#include "include/05commit/collector.h"

#define ASSERT(value, message)             \
    if (!(value)) {                        \
        std::cout << message << std::endl; \
        assert(value);                     \
    }

#define DEFINED_PROP(Type, Name)                                      \
private:                                                              \
    Type z##Name{};                                                   \
                                                                      \
public:                                                               \
    const Type& get##Name() const {                                   \
        return this->z##Name;                                         \
    }                                                                 \
                                                                      \
public:                                                               \
    void set##Name(Type&& value) {                                    \
        this->triggerChanged(PropKey::z##Name, this->z##Name, value); \
        z##Name = std::move(value);                                   \
    }                                                                 \
                                                                      \
    void set##Name(const Type& value) {                               \
        this->triggerChanged(PropKey::z##Name, this->z##Name, value); \
        z##Name = value;                                              \
    }
