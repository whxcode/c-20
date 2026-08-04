#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "include/consts/http_code.h"
#include "include/consts/http_method.h"
#include "include/kiwi/include/schema.h"
#include "include/protocol/model.h"

struct RawBody {
    std::string cData{};
};

struct FileBody {
    std::string cFile{};
};

// 客户端和服务器的通行协议

struct Headers {
    std::unordered_map<std::string, std::string> headers{};

    std::string toStirng() const {
        std::string result;

        for (const auto& [key, value] : headers) {
            result += key + ": " + value + "\r\n";
        }

        return result + "\r\n";
    }

    bool empty() const {
        return headers.empty();
    }

    std::string& operator[](const std::string& key) {
        return headers[key];
    }

    size_t count(const std::string& key) {
        return headers.count(key);
    }
};

struct Field {
    Field() = default;

    std::string contentType{};    // "image/png", "text/plain", ...
    std::string filename{};       // 文件上传时: "photo.jpg"
    std::vector<uint8_t> data{};  // 原始二进制内容
};

struct Body {
    std::string contentType{};                        // Content-Type 头
    std::vector<uint8_t> raw{};                       // 原始二进制
    std::unordered_map<std::string, Field> fields{};  // 按字段名解析后的数据

    void parse();  // 根据 contentType 解析 raw → fields
};

struct HttpRequest {
    std::string host{};
    std::string url{};

    HttpMethod method{};
    std::string path{};
    std::string version{};
    size_t contentLength{};
    Headers headers{};
    // std::unordered_map<std::string, std::string> headers{};
    Body body{};
};

class HttpProtocol : public Model {
public:
    Protocol getProtocol() override;
    void decode(kiwi::ByteBuffer& byte) override;
    void encode(kiwi::ByteBuffer& byte) override;
    void parser(uint8_t* ptr, size_t n);

public:
    void dump() const;

public:
    HttpRequest request{};
};

struct Response {
    static Response MakeRaw(std::string raw) {
        Response response;
        response.cStatus = HttpStatusCode::cOk;
        response.cHeaders["Content-Type"] = "text/plain; charset=utf-8";
        response.cBody = RawBody{std::move(raw)};

        return response;
    };

    static Response MakeFile(std::string filePath) {
        Response response;

        response.cStatus = HttpStatusCode::cOk;
        response.cHeaders["Content-Type"] = "application/octet-stream";
        response.cBody = FileBody{std::move(filePath)};

        return response;
    };

    HttpStatusCode cStatus{HttpStatusCode::cOk};
    Headers cHeaders{};
    std::variant<RawBody, FileBody> cBody;
};
