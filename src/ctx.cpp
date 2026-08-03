#include "include/ctx.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

#include "include/tools/file.h"
#include "include/tools/net.h"

TimeLine::TimeLine(TimePoint startedAt) : cStartedAt(startedAt) {
}

double TimeLine::elapsedSeconds() const {
    const auto elapsed = Clock::now() - cStartedAt;
    return std::chrono::duration<double>(elapsed).count();
}

sp<Ctx> Ctx::Make(int clientFd, int eventFd, HttpProtocol&& protocol,
                  TimeLine::TimePoint startedAt) {
    return std::make_shared<Ctx>(clientFd, eventFd, std::move(protocol), startedAt);
}

Ctx::Ctx(int clientFd, int eventFd, HttpProtocol&& protocol, TimeLine::TimePoint startedAt)
    : request(std::move(protocol.request)),
      cClientFd(clientFd),
      cEventFd(eventFd),
      cTimeline(startedAt) {
    cResponseHeaders["Content-Type"] = "text/plain";
    cResponseHeaders["Connection"] = "close";
}

Ctx::~Ctx() {
    closeBodyFile();
}

Headers& Ctx::responseHeaders() {
    return cResponseHeaders;
}

void Ctx::setRaw(const std::string& body) {
    cRawBody = body;
    cBodyType = BodyType::Raw;
}

void Ctx::setFile(const std::string& filePath) {
    cFilePath = filePath;
    cBodyType = BodyType::File;
}

void Ctx::setStatusCode(HttpStatusCode statusCode) {
    cStatusCode = statusCode;
}

HttpStatusCode Ctx::getStatusCode() const {
    return cStatusCode;
}

std::string Ctx::getStatusMessage() const {
    return GetStatusMessage(cStatusCode);
}

bool Ctx::isCancelled() const {
    return cCancelled.load();
}

void Ctx::cancel() {
    cCancelled.store(true);
}

void Ctx::setErrorHandle(ErrorHandle handle) {
    cErrorHandle = std::move(handle);
}

void Ctx::setCloseHandle(ErrorHandle handle) {
    cCloseHandle = std::move(handle);
}

void Ctx::send() {
    if (isCancelled()) {
        return;
    }

    prepareResponse();
    write();
}

void Ctx::prepareResponse() {
    cHeaderOffset = 0;
    cBodyOffset = 0;
    cSentSize = 0;
    closeBodyFile();

    if (cBodyType == BodyType::Raw) {
        cBodySize = static_cast<ssize_t>(cRawBody.size());
    } else {
        cFileFd = file::openReadOnly(cFilePath);
        cBodySize = file::size(cFileFd);
        cResponseHeaders["Content-Type"] = file::contentType(cFilePath);
    }

    cResponseHeaders["Content-Length"] = std::to_string(cBodySize);
    cResponseHeader = "HTTP/1.1 " + std::to_string(static_cast<int>(cStatusCode)) + " " +
                      getStatusMessage() + "\r\n" + cResponseHeaders.toStirng();
    cHeaderSize = static_cast<ssize_t>(cResponseHeader.size());
    cTotalSize = cHeaderSize + cBodySize;
}

void Ctx::write() {
    if (isCancelled()) {
        return;
    }

    while (cSentSize < cTotalSize) {
        ssize_t written = 0;

        if (cHeaderOffset < cHeaderSize) {
            written = ::send(cClientFd, cResponseHeader.data() + cHeaderOffset,
                             static_cast<size_t>(cHeaderSize - cHeaderOffset), MSG_NOSIGNAL);
            if (written > 0) {
                cHeaderOffset += written;
                cSentSize += written;
            }
        } else if (cBodyOffset < cBodySize) {
            if (cBodyType == BodyType::Raw) {
                written = ::send(cClientFd, cRawBody.data() + cBodyOffset,
                                 static_cast<size_t>(cBodySize - cBodyOffset), MSG_NOSIGNAL);
                if (written > 0) {
                    cBodyOffset += written;
                }
            } else {
                written = file::sendFile(cClientFd, cFileFd, &cBodyOffset,
                                         static_cast<size_t>(cBodySize - cBodyOffset));
            }

            if (written > 0) {
                cSentSize += written;
            }
        } else {
            break;
        }

        if (written > 0) {
            continue;
        }

        if (written == 0) {
            failResponse(EIO);
            return;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            enableWriteEvent();
            return;
        }

        failResponse(errno);
        return;
    }

    finishResponse();
}

void Ctx::enableWriteEvent() {
    if (cWriteEventEnabled) {
        return;
    }

    net::ctl(cEventFd, net::Operation::Modify, cClientFd,
             net::Event::Read | net::Event::EdgeTriggered | net::Event::Write);
    cWriteEventEnabled = true;
}

void Ctx::disableWriteEvent() {
    if (!cWriteEventEnabled) {
        return;
    }

    net::ctl(cEventFd, net::Operation::Modify, cClientFd,
             net::Event::Read | net::Event::EdgeTriggered);
    cWriteEventEnabled = false;
}

void Ctx::finishResponse() {
    disableWriteEvent();
    closeBodyFile();

    constexpr double bytesPerGiB = 1024.0 * 1024.0 * 1024.0;
    std::cout << "response completed path=" << request.path << " bytes=" << cSentSize
              << " GiB=" << static_cast<double>(cSentSize) / bytesPerGiB
              << " elapsed=" << cTimeline.elapsedSeconds() << "s" << std::endl;

    if (cCloseHandle) {
        cCloseHandle(cClientFd, 0);
    }
}

void Ctx::failResponse(int error) {
    disableWriteEvent();
    closeBodyFile();

    if (cErrorHandle) {
        cErrorHandle(cClientFd, error);
    }
}

void Ctx::closeBodyFile() {
    if (cFileFd >= 0) {
        ::close(cFileFd);
        cFileFd = -1;
    }
}
