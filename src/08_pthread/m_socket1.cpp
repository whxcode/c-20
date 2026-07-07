
#include <arpa/inet.h>
// #include <asm-generic/socket.h>
#include <netinet/in.h>
// #include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bitset>
#include <cerrno>
#include <coroutine>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

#include "include/08_pthread/m_pathred.h"
#include "include/09pthread_sockect/tools.h"
#include "include/SafeQueue.hpp"
#include "include/io.hpp"
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

                    // break;
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

static void test04() {
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
    fd_set tempReadFds{};

    FD_ZERO(&readfds);
    FD_ZERO(&tempReadFds);

    FD_SET(lfd, &readfds);

    int nready{0};
    int maxFd{lfd};
    char buf[1024]{0};
    size_t size{1024};

    while (1) {
        tempReadFds = readfds;
        // 返回值.
        // < 0 异常
        // = 0 阻塞超时，这儿 nullptr 这儿一定会一直阻塞。
        // > 0 可用的文件描述符总和。

        timeval timeouot{
            .tv_sec = 3,
            .tv_usec = 0,
        };
        nready = select(maxFd + 1, &tempReadFds, nullptr, nullptr, &timeouot);

        if (nready == 0) {
            std::cout << "超过最大时常" << std::endl;
            continue;
        }

        if (nready < 0) {
            if (errno == EINTR) {
                continue;
            }

            Wrap::PrintError("n ready < 0:");
        }

        // 有新客户端链接
        if (FD_ISSET(lfd, &tempReadFds)) {
            std::cout << "有新客户端请求连接" << std::endl;
            auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);

            if (cfd >= FD_SETSIZE) {
                std::cout << "客户端,已达到最大限制" << std::endl;
                close(lfd);
                continue;
            }

            maxFd = std::max(maxFd, cfd);
            FD_SET(cfd, &readfds);

            if (--nready == 0) {
                continue;
            }
        }

        for (size_t i = (size_t)lfd + 1; i <= maxFd; ++i) {
            if (!FD_ISSET(i, &tempReadFds)) {
                continue;
            }

            // 可用读取数据

            memset(buf, 0, size);
            auto readn = Wrap::Read(i, buf, size);

            if (readn <= 0) {
                close(i);
                FD_CLR(i, &readfds);

                // 清理不不要的,这儿使用 while 可用清理历史记录。
                if (i == maxFd) {
                    while (maxFd > lfd && !FD_ISSET(maxFd, &readfds)) {
                        --maxFd;
                    }
                }
            } else {
                for (size_t i = 0; i < readn; ++i) {
                    buf[i] = toupper(buf[i]);
                }

                Wrap::Write(i, buf, (size_t)readn);
            }

            if (--nready == 0) {
                break;
            }
        }

        std::cout << "本轮select通知已经处理完毕" << std::endl;

        // 判断客户端的发送的数据请求。
    }

    std::cout << "推出服务端" << std::endl;

    for (auto i = lfd; i <= maxFd; ++i) {
        close(i);
    }
}

static void WriteStringToFile(FILE* fp, const std::string& content, const int cfd) {
    if (fp == nullptr) {
        Wrap::PrintError("打开文件失败:");
    }

    auto c = "cfd:[%d]" + content;
    auto t = fprintf(fp, c.c_str(), cfd);
    fflush(fp);
}
// 优化代码。 数组
// 本质上还是单线程的模式
static void test05() {
    FILE* fp = fopen("/home/whx/webProject/c-20/text.txt", "a+");

    // 相当于还是轮询；
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
    fd_set tempReadFds{};

    FD_ZERO(&readfds);
    FD_ZERO(&tempReadFds);

    FD_SET(lfd, &readfds);

    int nready{0};
    int maxFd{lfd};
    char buf[1024]{0};
    size_t size{1024};
    std::vector<int> realReaderFds{};
    realReaderFds.resize(10);

    while (1) {
        tempReadFds = readfds;
        // 返回值.
        // < 0 异常
        // = 0 阻塞超时，这儿 nullptr 这儿一定会一直阻塞。
        // > 0 可用的文件描述符总和。

        timeval timeouot{
            .tv_sec = 3,
            .tv_usec = 0,
        };
        nready = select(maxFd + 1, &tempReadFds, nullptr, nullptr, &timeouot);

        if (nready == 0) {
            std::cout << "超过最大时常" << std::endl;
            continue;
        }

        if (nready < 0) {
            // 被信号中断
            if (errno == EINTR) {
                continue;
            }

            Wrap::PrintError("n ready < 0:");
        }

        // 有新客户端链接
        if (FD_ISSET(lfd, &tempReadFds)) {
            std::cout << "有新客户端请求连接" << std::endl;
            auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);

            if (cfd >= FD_SETSIZE) {
                std::cout << "客户端,已达到最大限制" << std::endl;
                close(lfd);
                continue;
            }

            maxFd = std::max(maxFd, cfd);
            FD_SET(cfd, &readfds);
            realReaderFds.push_back(cfd);

            if (--nready == 0) {
                continue;
            }
        }

        for (auto it = realReaderFds.begin(); it != realReaderFds.end();) {
            auto readerCfd = *it;
            if (!FD_ISSET(readerCfd, &tempReadFds)) {
                ++it;
                continue;
            }

            // 可用读取数据

            memset(buf, 0, size);
            auto readn = Wrap::Read(readerCfd, buf, size);

            if (readn <= 0) {
                close(readerCfd);
                FD_CLR(readerCfd, &readfds);

                // 清理不不要的,这儿使用 while 可用清理历史记录。
                if (readerCfd == maxFd) {
                    while (maxFd > lfd && !FD_ISSET(maxFd, &readfds)) {
                        --maxFd;
                    }
                }

                it = realReaderFds.erase(it);
            } else {
                for (size_t i = 0; i < readn; ++i) {
                    buf[i] = toupper(buf[i]);
                }

                WriteStringToFile(fp, buf, readerCfd);

                Wrap::Write(readerCfd, buf, (size_t)readn);

                ++it;
            }

            if (--nready == 0) {
                break;
            }
        }

        std::cout << "本轮select通知已经处理完毕" << std::endl;

        // 判断客户端的发送的数据请求。
    }

    std::cout << "推出服务端" << std::endl;

    for (auto i = lfd; i <= maxFd; ++i) {
        close(i);
    }

    fclose(fp);
}

/**
 * 多路 IO-POLL
 * int poll(struct pollfd* fds,nfds_t nfds,int timeout);
 *
 * poll 类似于 select，委托内核监控可读、异常事件;可用设置超时
 *
 * struct pollfd {
 *    int fd; 要监控的文件描述符
 *    short events; 输入参数，表示告诉内核要监控的事件，读事件、写事件、异常事件
 *    short revents; 输出参数，表示内核告诉调用者有哪些文件描述符有事发生。
 * }
 * events/revents:
 *    POLLIN: 可读事件：客户端链接、客户端发送数据 (被动)
 *    POLLOUT: 可写事件 (主动)
 *    POLLERR: 异常事件
 *
 * nfds 告诉内核监控的范围，具体时数组下标的最大值加+1
 *
 * timeout:
 *  0: 不阻塞
 *  -1: 一直阻塞；知道有事件发生
 *  > 0: 阻塞时长；在范围内若有事件发生立刻返回。
 *      超过时长也会返回。
 *
 *
 *  返回值:
 *    > 0 发生变化文件描述符的个数
 *    =0 没有文件描述符发生变化或者超时
 *    -1: 表示异常，并设置 errno
 *
 * */

static void test06() {
    struct sockaddr_in addr{
        .sin_family = AF_INET, .sin_port = htons(8081), .sin_addr = {.s_addr = htons(INADDR_ANY)}};
    auto lfd{Wrap::Socket(addr.sin_family, SOCK_STREAM, 0)};
    int opt{1};

    // 端口复用
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Wrap::Bind(lfd, (sockaddr*)&addr, sizeof(addr));
    Wrap::Listen(lfd, 10);

    pollfd pollfds[1024];
    size_t nfds{0};

    for (size_t i = 0; i < 1024; ++i) {
        pollfds[i].fd = -1;
    }

    auto getPollfdsIndex = [&pollfds](int fd) -> int {
        for (size_t i = 0; i < 1024; ++i) {
            if (pollfds[i].fd == fd) {
                return i;
            }
        }

        Wrap::PrintError("链接超过最大限制\n");

        return -1;
    };

    pollfds[0].fd = lfd;
    pollfds[0].events = POLLIN;
    char buf[1024]{0};
    size_t size{1024};
    FILE* fp = fopen("/home/whx/webProject/c-20/text.txt", "a+");

    WriteStringToFile(fp, "feaf", 0);

    while (1) {
        auto eventNumber = poll(pollfds, nfds + 1, -1);

        if (eventNumber < 0) {
            if (errno == EINTR) {
                continue;
            }
            Wrap::PrintError("poll error:");
            break;
        }

        if (pollfds[0].revents & POLLIN) {
            --eventNumber;
            auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);
            if (cfd < 1024) {
                auto index = getPollfdsIndex(-1);
                pollfds[index].fd = cfd;
                pollfds[index].events = POLLIN;
                ++nfds;
            } else {
                std::cout << "超过最大连接数" << std::endl;
            }
        }

        if (eventNumber == 0) {
            std::cout << "本轮 poll 事件已处理完毕" << std::endl;
            continue;
        }

        for (size_t i = 1; i <= nfds; ++i) {
            if (pollfds[i].fd == -1) {
                continue;
            }

            if (pollfds[i].revents & POLLIN) {
                auto readerCfd = pollfds[i].fd;
                // 有数据来了

                memset(buf, 0, size);
                auto readn = Wrap::Read(readerCfd, buf, size);

                // 客户端关闭
                if (readn <= 0) {
                    if (errno == EINTR) {
                    } else {
                        Wrap::PrintError("read 异常");
                        break;
                    }

                    close(readerCfd);
                    pollfds[i].fd = -1;

                    size_t j{i};
                    while (i == nfds && pollfds[j--].fd == -1) {
                        --nfds;
                    }

                } else {
                    std::string a;
                    a.assign(buf, (size_t)readn);
                    WriteStringToFile(fp, a, readerCfd);

                    for (size_t i = 0; i < readn; ++i) {
                        buf[i] = toupper(buf[i]);
                    }

                    Wrap::Write(readerCfd, buf, (size_t)readn);
                }

                if (--eventNumber == 0) {
                    std::cout << "本轮 poll 事件已处理完毕" << std::endl;
                    break;
                }
            }
        }
    }

    for (size_t i = 0; i <= nfds; ++i) {
        if (pollfds[i].fd == -1) {
            continue;
        }

        close(pollfds[i].fd);
    }
}

static void printBit() {
    char a{0};
    a = 1 | 2;

    // std::cout << "a:" << (int)a << std::endl;
    // std::cout << std::bitset<8>(a) << std::endl;
}

static void test07() {
    /*
      sockaddr_in addr{
          .sin_family = AF_INET, .sin_port = htons(8081), .sin_addr = {.s_addr =
      htonl(INADDR_ANY)}}; auto lfd = Wrap::Socket(addr.sin_family, SOCK_STREAM, 0);
      // 设置端口复用
      int opt{1};
      setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      // 绑定
      Wrap::Bind(lfd, (sockaddr*)&addr, sizeof(addr));
      // 监听
      Wrap::Listen(lfd, 10);

      // 将监听的 fd 加入 epoll
      epoll_event ev{};
      ev.events = EPOLLIN;
      ev.data.fd = lfd;

      epoll_event events[1024]{};

      int epfd = epoll_create(1);  // 参数在 Linux 2.6.8 后被忽略，但要 > 0
      epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

      char buf[1024]{0};
      size_t size{2};

      size_t triigerCount{0};
      while (1) {
          int nready = epoll_wait(epfd, events, size, -1);

          if (nready == -1) {
              if (errno == EINTR) {
                  Wrap::PrintError("epoll_wait:");
              }

              break;
          }

          printf("第 [%d] 触发\n", ++triigerCount);

          for (size_t i = 0; i < nready; ++i) {
              int fd = events[i].data.fd;

              // 有新客户端链接
              if (fd == lfd) {
                  auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);

                  ev.events = EPOLLIN;
                  ev.data.fd = cfd;

                  // 注册到内核
                  epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);

                  continue;
              }

              memset(buf, 0, size);
              auto n = Wrap::Read(fd, buf, size);
              if (n == 0) {
                  // 客户端关闭链接;
                  close(fd);
                  epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                  continue;
              }

              for (size_t i = 0; i < n; ++i) {
                  buf[i] = toupper(buf[i]);
              }

              Wrap::Write(fd, buf, (size_t)n);

              // 处理客户端数据
          }

          printf("------\n");
      }
    */
}

static void test08() {
    sockaddr_in addr{
        .sin_family = AF_INET, .sin_port = htons(8081), .sin_addr = {.s_addr = htonl(INADDR_ANY)}};
    auto lfd = Wrap::Socket(addr.sin_family, SOCK_STREAM, 0);
    // 设置端口复用
    int opt{1};
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // 绑定
    Wrap::Bind(lfd, (sockaddr*)&addr, sizeof(addr));
    // 监听
    Wrap::Listen(lfd, 10);

    // 将监听的 fd 加入 epoll
    TCP tcp;

    tcp.attach(lfd);

    char buf[1024]{0};
    size_t size{2};

    while (1) {
        auto epolls = tcp.await();
        printf("---[%d]\n", epolls.size());

        for (auto fd : epolls) {
            // 有新客户端链接
            if (fd == lfd) {
                std::cout << "新客户链接" << std::endl;

                auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);
                tcp.attach(cfd);
                continue;
            }

            memset(buf, 0, size);
            auto n = Wrap::Read(fd, buf, size);
            if (n == 0) {
                std::cout << "客户端断开链接" << std::endl;
                tcp.detach(fd);
                continue;
            }

            for (size_t i = 0; i < n; ++i) {
                buf[i] = toupper(buf[i]);
            }

            Wrap::Write(fd, buf, (size_t)n);

            // 处理客户端数据
        }
    }
}

/**
 * 使用epoll模型开发服务器流程
 * 1、创建文件描述符
 * 2、设置端口复用。
 * 3、绑定ip+port
 * 4、监听
 *
 * 5、创建 epoll 数
 * int epfd = epoll_create(1);
 *
 * 5、将文件描述符上树
 * struct epoll_event ev;
 * ev.events = EPOLLIN;// 监听可读事件。
 * epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
 * struct epoll_event events[1024];
 * wihile(1) {
 *    nready = epoll_wait(epfd,evnets,1024,-1); // 等待事件反应
 *
 *    if(nready < 0) {
 *      if(errno == EINTR) {
 *        contiune;
 *      }
 *      // 异常情况
 *      break;
 *    }
 *
 *    for(size_t i =0;i < nready; ++i)  {
 *      // 分为2中情况；
 *      1、有客户端请求链接
 *      2、有客户端发送数据
 *
 *      sockfd = events[i].data.fd;
 *
 *      if(sockfd == lfd) {
 *         // 存在客户端链接
 *         再次上树
 *
 *         cfd = accpet(lfd,nullptr,nullptr);
 *         event.data.fd = cfd;
 *         epoll_ctrl(epfd,EPOLL_CTL_ADD,cfd,&event);
 *
 *
 *
 *        contiune;
 *      }
 *
 *      处理客户端发送数据请求。
 *
 *
 *      n = read(sockfd,buf,sizeof(buf));
 *      if(n == 0) {
 *          客户端关闭链接
 *          close(sockfd);
 *          epoll_ctl(epf,EPOLL_CTL_DEL,sockfd,nullptr);
 *          contiune;
 *      }
 *
 *      // 处理后续流程
 *
 *    }
 *
 * }
 *
 *
 *
 * */

static void test09() {
    sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(8081),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };

    auto lfd = Wrap::Socket(addr.sin_family, SOCK_STREAM, 0);
    int opt{1};

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Wrap::Bind(lfd, (sockaddr*)&addr, sizeof(addr));
    Wrap::Listen(lfd, 10);

    auto epfd = epoll_create(1);

    if (epfd < 0) {
        Wrap::PrintError("epoll_create error:");
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = lfd;

    // 注册
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    // 就绪列表
    epoll_event events[1024]{};

    while (1) {
        auto nread = epoll_wait(epfd, events, 1024, -1);
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            }

            Wrap::PrintError("epoll_wait error:");
        }

        for (size_t i = 0; i < (size_t)nread; ++i) {
            auto fd = events[i].data.fd;

            if (fd == lfd) {
                // 新客户端

                auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);
                if (cfd < 0) {
                    Wrap::PrintError("accpet error:");
                }

                ev.events = EPOLLIN;
                ev.data.fd = cfd;

                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
                continue;
            }

            char buf[1024]{0};
            auto n = Wrap::Read(fd, buf, sizeof(buf));
            if (n == 0) {
                // 关闭
                close(fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                continue;
            } else if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }

                Wrap::PrintError("read error:");
            }

            for (size_t i = 0; i < (size_t)n; ++i) {
                buf[i] = toupper(buf[i]);
            }

            if (Wrap::Write(fd, buf, (size_t)n) < 0) {
                Wrap::PrintError("write error:");
            }
        }
    }
}

/**
 *
 * epoll 进阶
 *
 * epoll 的两种工作模式,ET 和 LT 模式
 * 水平触发:
 *  只要缓冲区有数据；就一直通知
 *
 *  边缘触发:
 *    缓冲区中有数据就会一直通知；之后有数据
 *
 *
 * */

static void test10() {
    sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(8081),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };

    auto lfd = Wrap::Socket(addr.sin_family, SOCK_STREAM, 0);
    int opt{1};

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    Wrap::Bind(lfd, (sockaddr*)&addr, sizeof(addr));
    Wrap::Listen(lfd, 10);

    auto epfd = epoll_create(1);

    if (epfd < 0) {
        Wrap::PrintError("epoll_create error:");
    }

    epoll_event ev{};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = lfd;

    // 注册
    epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

    // 就绪列表
    epoll_event events[1024]{};

    char buf[1024]{0};
    size_t size{2};
    size_t count{0};

    while (1) {
        auto nread = epoll_wait(epfd, events, 1024, -1);
        printf("第 [%d] 次触发\n", ++count);
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            }

            Wrap::PrintError("epoll_wait error:");
        }

        for (size_t i = 0; i < (size_t)nread; ++i) {
            auto fd = events[i].data.fd;

            if (fd == lfd) {
                // 新客户端

                auto cfd = accept4(lfd, nullptr, nullptr, SOCK_NONBLOCK);
                // auto cfd = Wrap::Accpet(lfd, nullptr, nullptr);
                if (cfd < 0) {
                    Wrap::PrintError("accpet error:");
                }

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = cfd;

                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
                continue;
            }

            int n{0};
            int readCount{0};
            while (1) {
                n = Wrap::Read(fd, buf + readCount, size);
                printf("nn->[%d]\n", n);
                if (n < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // 没有数据了
                        break;
                    }

                    Wrap::PrintError("read error:");
                } else if (n == 0) {
                    std::cout << "客户端关闭" << std::endl;
                    // 关闭
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
                    break;
                }

                readCount += n;
            }

            if (readCount == 0) {
                continue;
            }

            printf("readCount[%d]\n", readCount);
            for (size_t i = 0; i < (size_t)readCount; ++i) {
                buf[i] = toupper(buf[i]);
            }

            if (Wrap::Write(fd, buf, (size_t)readCount) < 0) {
                Wrap::PrintError("write error:");
            }
        }
    }
}

static void test11() {
    auto fd = tools::tcp(8081);
    Epoll* epoll = new Epoll();

    epoll->attach(fd, [epoll](const int fd) {
        auto cfd = Wrap::Accpet(fd, nullptr, nullptr);
        if (cfd < 0) {
            Wrap::PrintError("accpet error:");
        }

        std::cout << "新客户端链接:" << cfd << std::endl;

        epoll->attach(cfd, [epoll](const int fd) {
            std::cout << "读取数据" << std::endl;
            char buf[1024]{0};
            auto n = Wrap::Read(fd, buf, sizeof(buf));
            if (n == 0) {
                // 应该关闭
                epoll->detach(fd);
                return;
            } else if (n < 0) {
                if (errno == EINTR) {
                    return;
                }

                Wrap::PrintError("read error:");
            }

            for (size_t i = 0; i < (size_t)n; ++i) {
                buf[i] = toupper(buf[i]);
            }

            if (Wrap::Write(fd, buf, (size_t)n) < 0) {
                Wrap::PrintError("write error:");
            }
        });
    });

    while (1) {
        std::cout << "-" << std::endl;
        epoll->wait();
        std::cout << "----" << std::endl;
    }

    delete epoll;
}

class Connection {
public:
    Connection(Epoll* _epoll, int _fd) : epoll(_epoll), fd(_fd) {};

public:
    void start() {
        epoll->attach(fd, [this](const int fd) {
            this->handleRead(fd);
        });
    }

private:
    void handleRead(int fd) {
        char buf[1024]{};
        auto n = Wrap::Read(fd, buf, sizeof(buf));

        if (n <= 0) {
            epoll->detach(fd);
            return;
        }

        for (ssize_t i = 0; i < n; ++i) {
            buf[i] = std::toupper(static_cast<unsigned char>(buf[i]));
        }

        Wrap::Write(fd, buf, static_cast<size_t>(n));
    }

private:
    Epoll* epoll{nullptr};
    int fd{0};
};

class Server {
public:
    Server() {
        auto fd = tools::tcp(8081);
        epoll->attach(fd, [this](const int fd) {
            this->onAccept(fd);
        });
    }

public:
    void onAccept(int fd) {
        auto cfd = Wrap::Accpet(fd, nullptr, nullptr);
        if (cfd < 0) {
            Wrap::PrintError("accpet error:");
        }

        std::cout << "新客户端链接:" << cfd << std::endl;

        Connection* conn = new Connection(epoll, cfd);
        conn->start();
    }
    // 析构函数
    ~Server() {
        delete epoll;
    }

public:
    Epoll* epoll{new Epoll};
};

static void test12() {
    Server server;

    while (1) {
        server.epoll->wait();
    }
}

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() {
        }

        void unhandled_exception() {
            std::terminate();
        }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {
    }

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }

    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
};

Task test() {
    std::cout << "1\n";
    co_await std::suspend_always{};
    std::cout << "2\n";
    co_await std::suspend_always{};
    std::cout << "3\n";
}

static void test13() {
    Task t = test();

    std::cout << "start\n";

    t.resume();  // 输出 1，然后暂停
    t.resume();  // 输出 2，然后暂停
    t.resume();  // 输出 3，然后结束
}

};  // namespace MScokect1
//
//

namespace Co {

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void return_void() {
        }

        void unhandled_exception() {
            std::terminate();
        }
    };

    std::coroutine_handle<promise_type> handle{};

    explicit Task(std::coroutine_handle<promise_type> h) : handle(h) {
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    ~Task() {
        if (handle) {
            handle.destroy();
        }
    }

    void resume() {
        if (handle && !handle.done()) {
            handle.resume();
        }
    }
};

struct FetchDataAwaiter {
    std::string result;

    bool await_ready() {
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) {
        std::thread([this, h] {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            result = "success";

            h.resume();
        }).detach();
    }

    std::string await_resume() {
        return result;
    }
};

Task init() {
    std::cout << "start fetch\n";

    std::string data = co_await FetchDataAwaiter{};

    std::cout << "data: " << data << "\n";
}

int main() {
    std::cout << "考试执行" << std::endl;
    Task task = init();

    // task.resume();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "main end\n";
}
/**
 * 复习:
 * epoll 基本使用。 多路 IO 复用
 * 其内部是一颗红黑二叉树
 *
 * struct epoll_event {
 *    int events; // EPOLLIN、EPOLLET、EPLLOUT
 *    struct union data {
 *        void *ptr;
 *        int fd;
 *        uint32_t u32;
 *        uint64_t u64;
 *
 *    }
 * };
 *
 * int epfd = epll_create(1)
 * epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
 * int nready = epoll_wait(epfd,events,1024,-1);
 *
 * 调用 epoll_wait() 它可以把可用的描述符写入导 events 中，返回可用个数。
 * 业务层面可用直接拿到:
 * for(size_t i = 0;i<nready;++i) {
 *   // 业务处理
 *   int fd = events[i].data.fd;
 *
 * }
 *
 * 位运算
 * a | b 或；
 * a & b 与
 *
 * a |= b 或等
 *
 *
 *
 * */
}  // namespace Co
// void MScokect() {
//     char a{0};
//     a = 1 | 3;
//
//     unsigned char b = 0b10000000;
//     std::cout << "b:" << (uint32_t)b << std::endl;
//     std::cout << "a:" << (int)a << std::endl;
//     std::cout << std::bitset<8>(a) << std::endl;
//     // Co::main();
//     // MScokect1::test13();
//     // MScokect1::test12();
//     // MScokect1::test11();
//     // MScokect1::test08();
//     // MScokect1::printBit();
//     // MScokect1::test07();
//     //  MScokect1::test06();
//     //  MScokect1::test05();
//     //  MScokect1::test04();
//     //  MScokect1::test03();
//     //  MScokect1::test02();
//     //   MScokect1::test01();
// }
