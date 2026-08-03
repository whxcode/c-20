#pragma once

#include <sys/types.h>

#include <string>

namespace file {

std::string contentType(const std::string& filePath);
int openReadOnly(const std::string& filePath);
ssize_t size(int fd);
ssize_t sendFile(int socketFd, int fileFd, off_t* offset, size_t count);
void printError(const std::string& err);

}  // namespace file
