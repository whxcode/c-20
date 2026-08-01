#pragma once
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class ReadBuffer {
public:
    [[nodiscard]] size_t size() const;

    [[nodiscard]] bool empty() const;

    ssize_t readfd(int fd, int* errnum);

    std::string peek(const size_t len);
    bool readUint32t(uint32_t& value);

    void retrieve(const size_t len);
    uint8_t* head() {
        return (uint8_t*)data.data();
    }

public:
    std::vector<char> data;
};
