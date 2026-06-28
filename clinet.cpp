#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iterator>

#include "include/SafeQueue.hpp"
#include "include/patch.hpp"

struct MPatch {
    char* buf{nullptr};
    int readN{0};
};

SystemSafeQueue<MPatch> queue{10};

static void test01() {
    // 1. 客户端：创建一个“文件地址”类型的 Patch
    // Patch client_patch(1, "/home/wang/bigfile.mp4");
    // 2. 客户端序列化：变成一串可以在网络上传输的 byte 流
    // std::vector<uint8_t> net_bytes = client_patch.serialize();
    // std::cout << "网络传输的字节数: " << net_bytes.size() << " 字节\n";

    auto cfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server{.sin_family = AF_INET,
                       .sin_port = htons(8081),
                       .sin_addr = {
                           .s_addr = htonl(INADDR_ANY),
                       }};
    if (connect(cfd, (sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect error:");
    }

    char buf[1024]{0};

    while (1) {
        read(STDIN_FILENO, buf, sizeof(buf));

        send(cfd, buf, strlen(buf), 0);

        memset(buf, 0, sizeof(buf));

        auto n = recv(cfd, buf, sizeof(buf), 0);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("read error: ");
        } else if (n == 0) {
            std::cout << "server close" << std::endl;
            break;
        }

        queue.push(MPatch{.buf = buf, .readN = (int)n});

        std::cout << "---" << buf << "---";
    }

    close(cfd);
}

static void test02() {
    auto cfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server{.sin_family = AF_INET,
                       .sin_port = htons(8081),
                       .sin_addr = {
                           .s_addr = htonl(INADDR_ANY),
                       }};
    if (connect(cfd, (sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect error:");
    }

    pthread_t reader;
    pthread_t writer;

    pthread_create(
        &reader, nullptr,
        [](void* arg) -> void* {
            int cfd = *(int*)arg;
            char buf[1024]{0};

            while (1) {
                auto n = recv(cfd, buf, sizeof(buf), 0);
                if (n < 0) {
                    if (errno == EINTR) {
                        continue;
                    }

                    perror("read error: ");
                } else if (n == 0) {
                    std::cout << "server close " << std::endl;
                    close(cfd);
                    exit(0);
                    break;
                }

                std::cout << "--" << buf << "--" << std::endl;
            }

            std::cout << "读取完毕" << std::endl;

            return nullptr;
        },
        &cfd);

    pthread_create(
        &writer, nullptr,
        [](void* arg) -> void* {
            int cfd = *(int*)arg;
            char buf[1024]{0};

            while (1) {
                read(STDIN_FILENO, buf, sizeof(buf));

                send(cfd, buf, strlen(buf), 0);
            }

            // 用来向客户端写入数据
        },
        &cfd);

    pthread_join(reader, nullptr);
    // pthread_join(writer, nullptr);

    std::cout << "客户端关闭" << std::endl;
}

static void test03() {
}

int main() {
    test02();
    // test01();
    /*
      Patch server_patch;
      if (server_patch.deserialize(net_bytes)) {
          std::cout << "【服务端解析成功】\n";
          std::cout << "Mime 类型: " << server_patch.getMime() << " (1代表文件地址)\n";
          std::cout << "收到数据: " << server_patch.getData() << "\n";
      } else {
          std::cout << "解析失败！\n";
      }
    */

    return 0;
}
