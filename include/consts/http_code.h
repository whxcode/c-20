#pragma once

#include <cstdint>
#include <string>

enum class HttpStatusCode : uint32_t {
    cOk = 200,            // 请求成功
    cOkNoContent = 204,   // 请求成功，但没有内容返回
    cNoFound = 404,       // 请求的资源未找到
    cBadRequest = 400,    // 请求无效
    cNotAuthorized = 401, // 请求未授权
    cServiceUnavailable = 503  // 服务暂时不可用
};

static std::string GetStatusMessage(const HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::cOk:
            return "OK";
        case HttpStatusCode::cOkNoContent:
            return "No Content";
        case HttpStatusCode::cNoFound:
            return "Not Found";
        case HttpStatusCode::cBadRequest:
            return "Bad Request";
        case HttpStatusCode::cNotAuthorized:
            return "Not Authorized";
        case HttpStatusCode::cServiceUnavailable:
            return "Service Unavailable";
        default:
            return "Unknown Status";
    }
}
