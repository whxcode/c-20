#include "include/router.h"

#include <utility>

void Router::get(const std::string& path, Handler handler) {
    cGetHandlers[path] = std::move(handler);
}

void Router::post(const std::string& path, Handler handler) {
    cPostHandlers[path] = std::move(handler);
}

void Router::setNotFound(Handler handler) {
    cNotFoundHandler = std::move(handler);
}

void Router::setStatic(std::string path, Handler handler) {
    cStaticPath = std::move(path);
    cStaticHandler = std::move(handler);
}

const Router::Handler& Router::resolve(const HttpRequest& request) const {
    if (request.method == HttpMethod::cGet && !cStaticPath.empty() && cStaticHandler &&
        request.path.starts_with(cStaticPath)) {
        return cStaticHandler;
    }

    if (const auto* handler = findRoute(request)) {
        return *handler;
    }

    return cNotFoundHandler;
}

const Router::Handler* Router::findRoute(const HttpRequest& request) const {
    if (request.method == HttpMethod::cGet) {
        const auto iterator = cGetHandlers.find(request.path);
        return iterator == cGetHandlers.end() ? nullptr : &iterator->second;
    }

    if (request.method == HttpMethod::cPost) {
        const auto iterator = cPostHandlers.find(request.path);
        return iterator == cPostHandlers.end() ? nullptr : &iterator->second;
    }

    return nullptr;
}
