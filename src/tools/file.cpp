#include "include/tools/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>

#ifdef __linux__
#include <sys/sendfile.h>
#elif defined(__APPLE__)
#include <sys/socket.h>
#endif

namespace file {

std::string contentType(const std::string& filePath) {
    if (filePath.ends_with(".html") || filePath.ends_with(".htm")) return "text/html";
    if (filePath.ends_with(".json")) return "application/json";
    if (filePath.ends_with(".png")) return "image/png";
    if (filePath.ends_with(".jpg") || filePath.ends_with(".jpeg")) return "image/jpeg";
    if (filePath.ends_with(".js")) return "application/javascript";
    if (filePath.ends_with(".css")) return "text/css";
    if (filePath.ends_with(".pdf")) return "application/pdf";
    return "application/octet-stream";
}

int openReadOnly(const std::string& filePath) {
    return ::open(filePath.c_str(), O_RDONLY | O_CLOEXEC);
}

ssize_t size(int fd) {
    struct stat status{};
    return ::fstat(fd, &status) == 0 ? static_cast<ssize_t>(status.st_size) : -1;
}

ssize_t sendFile(int socketFd, int fileFd, off_t* offset, size_t count) {
#ifdef __linux__
    return ::sendfile(socketFd, fileFd, offset, count);
#elif defined(__APPLE__)
    off_t sentSize = static_cast<off_t>(count);
    const int result = ::sendfile(fileFd, socketFd, *offset, &sentSize, nullptr, 0);
    if (sentSize > 0) {
        *offset += sentSize;
        return sentSize;
    }
    return result == 0 ? 0 : -1;
#else
    errno = ENOSYS;
    return -1;
#endif
}

void printError(const std::string& err) {
    auto t = err + "{" + std::to_string(errno) + "}";
    std::perror(t.c_str());

    exit(-1);
}
}  // namespace file
