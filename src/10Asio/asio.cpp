
#include "include/10Asio/asio.h"

#include <asio.hpp>
#include <asio/use_awaitable.hpp>
#include <cstdio>
#include <iostream>

namespace Asio {
static void test10() {
    std::cout << "=== Asio Hello World ===" << std::endl;

    // io_context = 事件循环（等价于 epoll 实例）
    asio::io_context io;

    // steady_timer = 定时器，绑定到 io_context
    asio::steady_timer timer(io, std::chrono::seconds(1));

    // async_wait = 异步等待 1 秒
    timer.async_wait([](std::error_code ec) {
        if (!ec) {
            std::cout << "1 秒到了，定时器触发！" << std::endl;
        }
    });

    std::cout << "等待 1 秒..." << std::endl;

    // run = 启动事件循环（等价于 while(1) epoll_wait）
    io.run();

    std::cout << "=== 结束 ===" << std::endl;
}

// ============================================================
// 多链接 TCP echo server
// ============================================================
static void test11() {
    // io_context = 事件循环（对应 epoll 实例）
    asio::io_context io;

    // acceptor = 监听器（对应 listen fd）
    asio::ip::tcp::acceptor acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 8081));
    std::cout << "服务器启动在 8888..." << std::endl;

    // 每个客户端有自己的 socket 和缓冲区
    struct Client {
        asio::ip::tcp::socket socket;
        std::array<char, 1024> buf{};

        Client(asio::io_context& io) : socket(io) {
        }
    };

    // 递归 lambda：accept 一个连接 → 处理 → 再 accept 下一个
    std::function<void()> do_accept = [&]() {
        auto client = std::make_shared<Client>(io);

        acceptor.async_accept(client->socket, [&, client](std::error_code ec) {
            if (ec) {
                std::cout << "accept 失败: " << ec.message() << std::endl;
                return;
            }

            std::cout << "新客户端连接" << std::endl;

            // 读取这个客户端的数据
            auto do_read = [client](auto&& self) -> void {
                client->socket.async_read_some(
                    asio::buffer(client->buf), [client, self](std::error_code ec, size_t n) {
                        if (ec == asio::error::eof || ec == asio::error::connection_reset) {
                            std::cout << "客户端断开" << std::endl;
                            return;  // 关闭，不再继续读
                        }
                        if (ec) {
                            std::cout << "read 失败: " << ec.message() << std::endl;
                            return;
                        }

                        // 转大写
                        for (size_t i = 0; i < n; i++) {
                            client->buf[i] = std::toupper(client->buf[i]);
                        }

                        // 写回去
                        async_write(client->socket, asio::buffer(client->buf, n),
                                    [client, self](std::error_code ec, size_t) {
                                        if (ec) {
                                            std::cout << "write 失败: " << ec.message()
                                                      << std::endl;
                                            return;
                                        }
                                        // 继续读
                                        self(self);
                                    });
                    });
            };

            do_read(do_read);

            // 继续 accept 下一个连接
            do_accept();
        });
    };

    do_accept();

    // 启动事件循环
    io.run();
}

// ============================================================
// 多链接 TCP echo server — 协程版
// ============================================================
static asio::awaitable<void> handle_client(asio::ip::tcp::socket socket) {
    std::cout << "新客户端连接" << std::endl;

    char buf[1024];
    for (;;) {
        // co_await 代替回调！从上往下读，没有嵌套
        auto [ec, n] =
            co_await socket.async_read_some(asio::buffer(buf), asio::as_tuple(asio::use_awaitable));

        if (ec) {
            std::cout << "客户端断开" << std::endl;
            break;
        }

        // 转大写
        for (size_t i = 0; i < n; i++) buf[i] = std::toupper(buf[i]);

        // 写回去
        co_await async_write(socket, asio::buffer(buf, n), asio::use_awaitable);
    }
}

static asio::awaitable<void> server() {
    auto executor = co_await asio::this_coro::executor;
    asio::ip::tcp::acceptor acceptor(executor, {asio::ip::tcp::v4(), 8081});
    std::cout << "服务器启动在 8888..." << std::endl;

    for (;;) {
        // accept 也是 co_await，顺序写
        auto socket = co_await acceptor.async_accept(asio::use_awaitable);

        // 每个新连接启动一个协程，互不干扰
        co_spawn(executor, handle_client(std::move(socket)), asio::detached);
    }
}

static void test12() {
    asio::io_context io;

    // co_spawn 启动 server 协程
    co_spawn(io, server(), asio::detached);

    // 事件循环
    io.run();
}

}  // namespace Asio

void TestAsio() {
    // Asio::test10();
    // Asio::test11();
    Asio::test12();
}
