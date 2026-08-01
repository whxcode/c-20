#include "include/readerbuffer.h"

#include <netinet/in.h>

#include <cstdint>
#include <iostream>

size_t ReadBuffer::size() const {
    return data.size();
}
[[nodiscard]] bool ReadBuffer::empty() const {
    return data.empty();
}
ssize_t ReadBuffer::readfd(int fd, int* errnum) {
    char temp[1024]{0};
    ssize_t readTotal{0};

    while (true) {
        ssize_t n = ::read(fd, temp, sizeof(temp));
        if (n > 0) {
            data.insert(data.end(), temp, temp + n);
            readTotal += n;
        } else if (n == 0) {
            return 0;
        } else if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            if (errno == EINTR) {
                continue;
            }

            if (errnum != nullptr) {
                *errnum = errno;
            }

            return -1;
        }
    }

    return readTotal;
}

std::string ReadBuffer::peek(const size_t len) {
    auto l{len};

    if (l > data.size()) {
        l = data.size();
    }

    return {data.begin(), data.begin() + (long)l};
}

bool ReadBuffer::readUint32t(uint32_t& value) {
    if (data.size() < sizeof(uint32_t)) {
        return false;
    }

    value = ntohl(*(uint32_t*)data.data());

    return true;
}

void ReadBuffer::retrieve(const size_t len) {
    if (len >= data.size()) {
        data.clear();
    } else {
        data.erase(data.begin(), data.begin() + (long)len);
    }
}
