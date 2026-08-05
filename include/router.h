#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "include/protocol/http.h"

class Router {
public:
    using Handler = std::function<Response(const HttpRequest& request)>;

    void get(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void setNotFound(Handler handler);
    void setStatic(std::string path, Handler handler);

    const Handler& resolve(const HttpRequest& request) const;

private:
    const Handler* findRoute(const HttpRequest& request) const;

private:
    std::unordered_map<std::string, Handler> cGetHandlers{};
    std::unordered_map<std::string, Handler> cPostHandlers{};
    std::string cStaticPath{};
    Handler cStaticHandler{};
    Handler cNotFoundHandler{[](const HttpRequest&) {
        auto response = Response::MakeRaw("404 Not Found");
        response.cStatus = HttpStatusCode::cNoFound;
        return response;
    }};
};
