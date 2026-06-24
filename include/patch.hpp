#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class Patch {
private:
    uint32_t mime;      // 0=文本, 1=文件地址 (固定 4 字节)
    uint32_t data_len;  // 记录数据长度 (固定 4 字节)
    std::string data;   // 实际的数据内容 (变长)

public:
    // 构造函数
    Patch() : mime(0), data_len(0) {
    }
    Patch(uint32_t m, const std::string& d) : mime(m), data_len(d.size()), data(d) {
    }

    // Getters 方便测试
    uint32_t getMime() const {
        return mime;
    }
    std::string getData() const {
        return data;
    }

    // 核心 1：序列化 (把类里的属性打碎成一段连续的 byte 数组)
    std::vector<uint8_t> serialize() const {
        // 总长度 = mime(4) + data_len(4) + 字符串实际长度
        size_t total_size = sizeof(mime) + sizeof(data_len) + data.size();
        std::vector<uint8_t> buffer(total_size);

        // 像排队一样，把数据一个个 copy 进 buffer
        uint8_t* ptr = buffer.data();

        // 1. 塞入 mime
        std::memcpy(ptr, &mime, sizeof(mime));
        ptr += sizeof(mime);

        // 2. 塞入 data_len
        std::memcpy(ptr, &data_len, sizeof(data_len));
        ptr += sizeof(data_len);

        // 3. 塞入实际的字符串数据
        if (data_len > 0) {
            std::memcpy(ptr, data.data(), data_len);
        }
        std::cout << "mime:" << mime << std::endl;
        std::cout << "data_len:" << data_len << std::endl;

        return buffer;
    }

    // 核心 2：反序列化 (传入二进制 byte 数组，解析还原类的属性)
    bool deserialize(const std::vector<uint8_t>& buffer) {
        // 安全检查：如果连前 8 个字节的基本信息都不够，直接报废
        if (buffer.size() < sizeof(mime) + sizeof(data_len)) return false;

        const uint8_t* ptr = buffer.data();

        // 1. 抠出 mime
        std::memcpy(&mime, ptr, sizeof(mime));
        ptr += sizeof(mime);

        // 2. 抠出 data_len
        std::memcpy(&data_len, ptr, sizeof(data_len));
        ptr += sizeof(data_len);

        // 安全检查：防止 buffer 数据不完整导致内存越界
        if (buffer.size() < sizeof(mime) + sizeof(data_len) + data_len) return false;

        // 3. 抠出数据并还原为 string
        data.assign(reinterpret_cast<const char*>(ptr), data_len);

        return true;
    }
};
