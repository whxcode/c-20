#include "include/protocol/http.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>

#include "include/kiwi/include/schema.h"

// ─── URL 解码（x-www-form-urlencoded 用） ───
static std::string urlDecode(const std::string& src) {
    std::string ret;
    for (size_t i = 0; i < src.size(); ++i) {
        if (src[i] == '%' && i + 2 < src.size()) {
            char hex[3] = {src[i + 1], src[i + 2], 0};
            ret += (char)strtol(hex, nullptr, 16);
            i += 2;
        } else if (src[i] == '+') {
            ret += ' ';
        } else {
            ret += src[i];
        }
    }
    return ret;
}

// ─── Body::parse ───
void Body::parse() {
    fields.clear();
    if (raw.empty()) return;

    if (contentType.find("application/x-www-form-urlencoded") != std::string::npos) {
        std::string rawStr(raw.begin(), raw.end());
        size_t pos = 0;
        while (pos < rawStr.size()) {
            auto amp = rawStr.find('&', pos);
            if (amp == std::string::npos) amp = rawStr.size();
            auto eq = rawStr.find('=', pos);
            if (eq < amp) {
                auto key = urlDecode(rawStr.substr(pos, eq - pos));
                auto val = urlDecode(rawStr.substr(eq + 1, amp - eq - 1));
                auto& f = fields[key];
                f.data.assign(val.begin(), val.end());
            }
            pos = amp + 1;
        }
    } else if (contentType.find("multipart/form-data") != std::string::npos) {
        // 提取 boundary
        auto boundaryPos = contentType.find("boundary=");
        if (boundaryPos == std::string::npos) {
            // 无法解析，全量数据存为 raw
            return;
        }
        std::string boundary = contentType.substr(boundaryPos + 9);
        // 去掉可能的引号
        if (boundary.front() == '"' && boundary.back() == '"') {
            boundary = boundary.substr(1, boundary.size() - 2);
        }
        std::string delim = "--" + boundary;
        std::string endDelim = delim + "--";

        std::string rawStr(raw.begin(), raw.end());
        size_t pos = rawStr.find(delim);
        if (pos == std::string::npos) return;
        pos += delim.size();

        while (pos < rawStr.size()) {
            // 跳过 \r\n
            if (rawStr[pos] == '\r') pos++;
            if (rawStr[pos] == '\n') pos++;
            if (rawStr.substr(pos, endDelim.size()) == endDelim) break;
            if (rawStr.substr(pos, delim.size()) == delim) break;

            // 解析 part 头部
            auto headerEnd = rawStr.find("\r\n\r\n", pos);
            if (headerEnd == std::string::npos) break;

            std::string partHeaders = rawStr.substr(pos, headerEnd - pos);
            pos = headerEnd + 4;

            // 取 Content-Disposition
            auto cdPos = partHeaders.find("Content-Disposition:");
            std::string name, filename, partContentType;
            if (cdPos != std::string::npos) {
                auto cdEnd = partHeaders.find("\r\n", cdPos);
                if (cdEnd == std::string::npos) cdEnd = partHeaders.size();
                std::string cd = partHeaders.substr(cdPos, cdEnd - cdPos);
                // name="xxx"
                auto nPos = cd.find("name=\"");
                if (nPos != std::string::npos) {
                    nPos += 6;
                    name = cd.substr(nPos, cd.find('"', nPos) - nPos);
                }
                // filename="xxx"
                auto fPos = cd.find("filename=\"");
                if (fPos != std::string::npos) {
                    fPos += 10;
                    filename = cd.substr(fPos, cd.find('"', fPos) - fPos);
                }
            }
            // Content-Type
            auto ctPos = partHeaders.find("Content-Type:");
            if (ctPos != std::string::npos) {
                auto ctEnd = partHeaders.find("\r\n", ctPos);
                if (ctEnd == std::string::npos) ctEnd = partHeaders.size();
                partContentType = partHeaders.substr(ctPos + 13, ctEnd - ctPos - 13);
                // trim
                if (!partContentType.empty() && partContentType.front() == ' ')
                    partContentType = partContentType.substr(1);
            }

            // 取 part 数据
            auto nextDelim = rawStr.find("\r\n" + delim, pos);
            if (nextDelim == std::string::npos) nextDelim = rawStr.size();

            std::vector<uint8_t> partData(rawStr.begin() + pos, rawStr.begin() + nextDelim);
            // 去掉尾部 \r\n
            if (partData.size() >= 2 && partData[partData.size() - 2] == '\r')
                partData.resize(partData.size() - 2);

            auto& f = fields[name];
            f.contentType = partContentType;
            f.filename = filename;
            f.data = std::move(partData);

            pos = nextDelim + delim.size();
        }
    }
    // application/json, image/*, text/plain 等：存为默认字段
}

Protocol HttpProtocol::getProtocol() {
    return Protocol::Http;
};

void HttpProtocol::decode(kiwi::ByteBuffer& byte) {
}

void HttpProtocol::encode(kiwi::ByteBuffer& bb) {
}

void HttpProtocol::parser(uint8_t* ptr, size_t n) {
    std::string raw(reinterpret_cast<char*>(ptr), n);

    // 找 \r\n\r\n 分割头部和正文
    auto headEnd = raw.find("\r\n\r\n");
    if (headEnd == std::string::npos) return;

    // 解析请求行
    auto firstLine = raw.find("\r\n");
    if (firstLine == std::string::npos) return;

    std::string requestLine = raw.substr(0, firstLine);
    auto p1 = requestLine.find(' ');
    auto p2 = requestLine.find(' ', p1 + 1);
    if (p1 == std::string::npos || p2 == std::string::npos) return;

    // 解析 请求方法

    auto methodStr = requestLine.substr(0, p1);

    if (methodStr == "GET") {
        request.method = HttpMethod::cGet;
    } else if (methodStr == "POST") {
        request.method = HttpMethod::cPost;
    } else if (methodStr == "PUT") {
        request.method = HttpMethod::cPut;
    } else if (methodStr == "DELETE") {
        request.method = HttpMethod::cDelete;
    } else if (methodStr == "HEAD") {
        request.method = HttpMethod::cHead;
    } else if (methodStr == "OPTIONS") {
        request.method = HttpMethod::cOptions;
    } else if (methodStr == "PATCH") {
        request.method = HttpMethod::cPatch;
    } else if (methodStr == "TRACE") {
        request.method = HttpMethod::cTrace;
    } else if (methodStr == "CONNECT") {
        request.method = HttpMethod::cConnect;
    }

    request.path = requestLine.substr(p1 + 1, p2 - p1 - 1);
    request.version = requestLine.substr(p2 + 1);

    // 解析头部
    size_t pos = firstLine;
    while (pos < headEnd) {
        auto next = raw.find("\r\n", pos + 2);
        if (next == std::string::npos || next > headEnd) break;

        std::string line = raw.substr(pos + 2, next - pos - 2);
        if (line.empty()) break;

        auto colon = line.find(": ");
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 2);
            request.headers[key] = value;

            if (key == "Host") request.host = value;
            if (key == "Content-Length") request.contentLength = std::stoul(value);
        }

        pos = next;
    }

    // 正文
    if (headEnd + 4 < raw.size()) {
        auto bodyBegin = raw.begin() + headEnd + 4;
        request.body.raw.assign(bodyBegin, raw.end());
        request.body.contentType =
            request.headers.count("Content-Type") ? request.headers["Content-Type"] : "";
        request.body.parse();
    }

    request.url = request.path;
}

void HttpProtocol::dump() const {
    std::cout << "  Method:   " << (ssize_t)request.method << std::endl;
    std::cout << "  Path:     " << request.path << std::endl;
    std::cout << "  Version:  " << request.version << std::endl;

    if (!request.headers.empty()) {
        std::cout << "  ───────────────────────────────── Headers ─────────────────────────────────"
                  << std::endl;

        std::cout << request.headers.toStirng() << std::endl;
        // for (auto& [k, v] : request.headers) {
        //  std::cout << "  " << k << ": " << v << std::endl;
        //}
    }

    if (!request.body.raw.empty()) {
        std::cout << "  ───────────────────────────────── Body ─────────────────────────────────"
                  << std::endl;
        if (request.body.fields.empty()) {
            std::cout << "  [raw " << request.body.raw.size() << " bytes]" << std::endl;
        } else {
            for (auto& [name, field] : request.body.fields) {
                std::cout << "  field: " << (name.empty() ? "(default)" : name) << std::endl;
                if (!field.filename.empty())
                    std::cout << "    filename: " << field.filename << std::endl;
                if (!field.contentType.empty())
                    std::cout << "    Content-Type: " << field.contentType << std::endl;
                std::cout << "    size: " << field.data.size() << " bytes" << std::endl;
                // 文本字段打印内容
                if (field.contentType.find("text") != std::string::npos ||
                    field.contentType.find("json") != std::string::npos ||
                    field.contentType.empty()) {
                    std::cout << "    value: " << std::string(field.data.begin(), field.data.end())
                              << std::endl;
                }
            }
        }
    }

    std::cout << std::endl;
};
