#pragma once

#include <string>

#include <sys/types.h>

namespace file {

std::string contentType(const std::string& filePath);
int openReadOnly(const std::string& filePath);
ssize_t size(int fd);
ssize_t sendFile(int socketFd, int fileFd, off_t* offset, size_t count);

}  // namespace file
