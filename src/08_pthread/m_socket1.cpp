
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#include "include/08_pthread/m_pathred.h"
#include "include/SafeQueue.hpp"
#include "include/patch.hpp"
#include "include/wrap/wrap.h"

namespace MScokect1 {
/**
 * SYN_SEND 状态；客户端？
 * SYN_RECD 状态；服务端？
 * TIME_WAIT 主动关闭方
 * 在数据传输时；状态不会改变
 *
 * seq: 对方上一次的 ACK
 * ACK: 对方上一次的 SEQ+数据大小（注:SYN、FIN站一位）
 * 端口复用
 * setsockopt(int fd,SO_SOCKET,SO_BIND)
 * shutdown 可以实现半关闭, (用户行为；不影响内核行为)
 * shutdown 不考虑文件的引用计数关系；只要调用是直接彻底关闭
 * close 考虑文件引用计算；调用一次close只是将引用计数-1
 * 只有减小到0的时候才会真正关闭。
 *
 * */
static void test01() {
    auto sfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{
        .sin_family = AF_INET, .sin_port = htons(8081), .sin_addr = {.s_addr = htonl(INADDR_ANY)}

    };
    // 解决端口复用
    int opt{1};
    // setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Wrap::Bind(sfd, (sockaddr*)&addr, sizeof(addr));
    Wrap::Listen(sfd, 10);

    auto cfd = Wrap::Accpet(sfd, nullptr, nullptr);

    std::cout << "---:" << cfd << std::endl;

    // shutdown(cfd, SHUT_RD);
    shutdown(cfd, SHUT_WR);

    char buf[1024]{0};
    size_t size{1024};

    while (1) {
        int n = Wrap::Read(cfd, buf, size);
        if (n == 0) {
            close(cfd);
            std::cout << "关闭链接" << cfd << std::endl;
            break;
        }

        std::cout << n << "::" << "Recv:" << buf << std::endl;

        if (Wrap::Write(cfd, buf, (size_t)n, MSG_NOSIGNAL) < 0) {
            Wrap::PrintError("Wrap::Write Error:");
        }
    }

    sleep(1000);
}

/**
 * 1、什么是心跳包？
 *    主要检测对方网络链接是否正常?;主要用于长链接
 * 2、在什么情况下使用心跳包？
 *     a. int keepAive{0};
 *        setsockopt(lisenfd,SOL_SOCKET,SO_KEEPALIVE,(void*)&keepAive)
 *    .b 在应用程序中自定义心跳跑
 *
 * 3、如何使用心跳跑？
 *    发送心跳过程:
 *     空闲、或定时
 *     客户端发送->AAAA.
 *     服务端收到 AAAA，发送-> BBBB
 *     此时客户端收到 BBBB 之后；认为链接正常.
 *
 *     假如 客户端 发送了 （3-5） 次 AAAA
 *     依旧没有收到 BBBB 的回复；认为网络异常。
 *
 *     异常之后；A应该重建链接。
 *
 *     1. 先 close 原来的链接。
 *     2. 重新调用 connect 链接就行（前提是服务器正常运行）
 *
 *     如果让心跳和正常的业务数据不混淆?
 *     双方协议:
 *      a. 简单 添加 header 头。加一个位 TYPE： 0 业务数据，1 心跳数据
 *
 *     服务端这边需要拆包
 *     [header]
 *      type 0(业务),
 *
 * */
static void test02() {
    auto sfd = Wrap::Socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{
        .sin_family = AF_INET, .sin_port = htons(8081), .sin_addr = {.s_addr = htonl(INADDR_ANY)}

    };
    // 解决端口复用
    int opt{1};
    // setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Wrap::Bind(sfd, (sockaddr*)&addr, sizeof(addr));
    Wrap::Listen(sfd, 10);

    auto cfd = Wrap::Accpet(sfd, nullptr, nullptr);

    char buf[1024]{0};
    size_t size{1024};

    while (1) {
        int n = Wrap::Read(cfd, buf, size);
        if (n == 0) {
            close(cfd);
            std::cout << "关闭链接" << cfd << std::endl;
            break;
        }

        std::cout << n << "::" << "Recv:" << buf << std::endl;
        for (size_t i = 0; i < n; ++i) {
            buf[i] = toupper(buf[i]);
        }

        if (Wrap::Write(cfd, buf, (size_t)n, MSG_NOSIGNAL) < 0) {
            Wrap::PrintError("Wrap::Write Error:");
        }
    }

    sleep(1000);
}

/**
 * 使用 select 的开发服务流程。
 * 1. 创建 socket，得到监听文件描述符 lfd socket
 * 2. 设置端口复用 setsockopt
 * 3. 将 ldf 和ip port 绑定 bind
 * 4. 设置监听 listen
 * 5. fd_set readfds;
 *    FD_ZERO(&readfds);
 *    将 ldf 加入到 readfds 集合中。
 *    FD_SET(lfd,&readfds); 将 lfd 加入到 readfds 集合中。
 *
 *    size_t maxFd = lfd + 1;
 *    while (1) {
 *      auto tempFds = readfds;
 *      nready =  selet(maxFd,&tempFds,nullptr(写),nullptr(异常),nullptr(超时));
 *
 *      if(nready < 0) {
 *        if(errno == EINTR) { // 被信号中断，不是错误。
 *          contiune;
 *        }
 *
 *        break;
 *      }
 *
 *      有客户端请求到来。
 *      if(FD_ISSET(lfd,&tempFds)) {
 *          int cfd = accpet(fd,nullptr,nullptr);
 *          maxFd = std::max(maxFd,cfd) + 1;
 *          FD_SET(cfd,&readfds);
 *      }
 *
 *      if(nread == 1) {
 *        contine;
 *      }
 *
 *      for(i = ldf + 1;i < maxFd;++ i) {
 *          if(!FD_ISET(i,&tempFds)) {
 *            contine;
 *          }
 *
 *           while(1) {
 *
 *          // 处理客户的数据
 *          auto n = recv(i,buf,sizeof(buf),0);
 *           if( n <= 0) {
 *              close(i);
 *              FD_CLR(i,&readfs)
 *              maxFd = std::min(maxFd,i) + 1;
 *              contiune;
 *           }
 *
 *            // 回应客户端
 *            write(i,buf,n);
 *          }
 *
 *      }
 *
 *    }
 *
 *    close(ldf);
 *    return 0;
 *
 *
 *
 * */
static void test03() {
    sockaddr_in sAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(8081),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };

    auto lfd = Wrap::Socket(sAddr.sin_family, SOCK_STREAM, 0);
    int opt{1};

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    Wrap::Bind(lfd, (sockaddr*)&sAddr, sizeof(sAddr));
    Wrap::Listen(lfd, 10);

    fd_set readfds{};
    size_t maxFd = (size_t)lfd;

    char buf[1024]{0};
    size_t size{1024};

    FD_ZERO(&readfds);
    FD_SET(lfd, &readfds);

    std::cout << "进入while" << std::endl;
    while (1) {
        auto fdsetp = readfds;
        auto nready = select(maxFd + 1, &fdsetp, nullptr, nullptr, nullptr);

        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }

            break;
        }

        // 有新客户端请求链接。
        if (FD_ISSET(lfd, &fdsetp)) {
            std::cout << "有新客户端请求链接" << std::endl;
            auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);
            if (cfd >= FD_SETSIZE) {
                close(cfd);
                std::cout << "达到最大链接限制" << std::endl;
                continue;
            }

            FD_SET(cfd, &readfds);
            maxFd = std::max(maxFd, (size_t)cfd);

            // 没有后续处理事项了。
            if (--nready == 0) {
                continue;
            }
        }

        for (size_t i = (size_t)lfd + 1; i <= maxFd; ++i) {
            if (!FD_ISSET(i, &fdsetp)) {
                continue;
            }

            while (1) {
                auto n = recv(i, buf, size, MSG_DONTWAIT);
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 没有数据
                        break;
                    }
                    Wrap::PrintError("recv error:");
                }

                if (n == 0) {
                    // 客户端关闭链接
                    std::cout << "客户端关闭链接" << std::endl;
                    FD_CLR(i, &readfds);
                    close(i);

                    if (i == maxFd) {
                        while (maxFd > lfd && !FD_ISSET(maxFd, &readfds)) {
                            --maxFd;
                        }
                    }

                    break;
                }

                for (size_t i = 0; i < n; ++i) {
                    buf[i] = toupper(buf[i]);
                }

                Wrap::Write(i, buf, (size_t)n);
            }

            // 已经处理完毕了。
            if (--nready == 0) {
                break;
            }
        }

        std::cout << "本轮处理完毕；进行下一轮处理" << std::endl;
    }

    close(lfd);
    for (size_t i = (size_t)lfd + 1; i <= maxFd; ++i) {
        close(i);
    }
}
};  // namespace MScokect1

void MScokect() {
    MScokect1::test03();
    // MScokect1::test02();
    //  MScokect1::test01();
}
