#include "include/session.h"

#ifdef __APPLE__
#include <sys/_types/_ssize_t.h>
#endif
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "include/readerbuffer.h"

Session* Session::Make(const int fd, ISessionManager* m) {
    return new Session(fd, m);
}

Session::Session(const int fd, ISessionManager* m) : cfd(fd), manager(m) {
}

void Session::handle(sp<Ctx>& ctx) {
    // ctx->http.dump();
    processRequest();
}

void Session::processRequest() {
    std::string body = "<html><body><h1>Hello, World!</h1></body></html>";
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;

    send(cfd, response.data(), response.size(), 0);
}
