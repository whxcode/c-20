/*
#include <boost/asio.hpp>
#include <cctype>
#include <iostream>

namespace asio = boost::asio;
using asio::ip::tcp;

int main() {
    try {
        asio::io_context io;

        // 创建监听 socket
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8081));

        std::cout << "监听 8081..." << std::endl;

        // 等待客户端连接
        tcp::socket sock(io);
        acceptor.accept(sock);
        std::cout << "客户端已连接" << std::endl;

        char buf[1024];
        for (;;) {
            boost::system::error_code ec;
            size_t n = sock.read_some(asio::buffer(buf), ec);

            if (ec == asio::error::eof) {
                std::cout << "客户端关闭连接" << std::endl;
                break;
            }
            if (ec) {
                std::cout << "读错误: " << ec.message() << std::endl;
                break;
            }

            std::cout << n << "::Recv:" << std::string(buf, n) << std::endl;

            // 转大写
            for (size_t i = 0; i < n; ++i) {
                buf[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(buf[i])));
            }

            // 写回
            asio::write(sock, asio::buffer(buf, n));
        }
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
*/
