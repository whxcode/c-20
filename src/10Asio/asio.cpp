
#include <asm-generic/socket.h>
#include <event2/event.h>
#include <sys/socket.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#include "include/08_pthread/m_pathred.h"
#include "include/09pthread_sockect/tools.h"
#include "include/SafeQueue.hpp"
#include "include/io.hpp"
#include "include/patch.hpp"
#include "include/wrap/wrap.h"
/**
 * libevent c++ 网络库
 *
 * */

namespace Asio {
static void test01() {
    const char** p = event_get_supported_methods();
    while (true) {
        if (*p == nullptr) {
            break;
        }

        std::cout << *p << std::endl;
        p++;
    }

    //
    struct event_base* base = event_base_new();
    if (base == nullptr) {
        std::cout << "event_base_new failed" << std::endl;
    }

    const char* method = event_base_get_method(base);
    std::cout << "event_base_get_method: " << method << std::endl;
}
/**
 * 使用 libevent 的步骤
 * 1、创建根节点 event_base_new
 * 2、设置事件和数据可读可写的事件的回调函数。
 * 3、事件循环 -- event_base_dispatch
 *    相当于 while(1),在虚幻内部等待事件的发生，若有事情发生则触发事件对应的回调函数。
 * 4、释放根节点，
 *
 * 5、libevent 是事件驱动。
 *
 * */

/**
 * 编写一个基于 libevent 实现的 tcp 服务器
 * 1、创建 sockect -- sockect.
 * 2、设置端口复用 setsockopt(lfd,SOL_SOCKET,SO_RESUEADDR,&opt,sizeof(opt));
 * 3、绑定 ip port
 * 4、监听 listen
 *
 * 5、创建 地基
 * struct event_base *base = event_base_new() -- epoll_create(1)
 * 6、上树
 *
 *  void readcb(evutil_socket_t fd,short events,void *args) {
 *      n = read(fd,buf,sizeof(buf));
 *      if(n <= 0) {
 *          // 从base中删除事件
 *          close(fd);
 *          event_dele(ev);
 *
 *      }
 *  }
 *
 *
 *  创建 lfd 对应的事件
 *  void conncb(int fd, short events,void *args) {
 *    // 接受新的链接
 *    int cfd accept(fd,nullptr,nullptr);
 *     struct event_base *base = (struct event_base*)args;
 *
 *    if(cfd > 0) {
 *      // 再次执行上树
 *
 *      struct event *event event_new(base,cfd,EV_READ|EV_PERSIST,readcb,nullptr);
 *      event_add(ev,nullptr);
 *    }
 *
 *
 *  }
 *  struct event *event = event_new(base ,lfd,EV_READ|EV_PRESIST,conncb[回调],nullptr);
 *  event_add(event,nullptr);
 *
 * 8、进入事件循环
 * event_base_dispatch(base);
 *
 * 9、异常情况下；退出循环、释放资源
 * event_base_free(base);
 * event_free(event);
 *
 *
 *
 * */

static void test02() {
    int lfd{Wrap::Socket(AF_INET, SOCK_STREAM, 0)};
    int opt{1};

    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv{
        .sin_family = AF_INET, .sin_port = htons(8081), .sin_addr = {.s_addr = INADDR_ANY}};

    Wrap::Bind(lfd, (struct sockaddr*)&serv, sizeof(serv));

    Wrap::Listen(lfd, 10);

    struct event_base* base{event_base_new()};

    if (base == nullptr) {
        Wrap::PrintError("event_base_new failed");
    }

    struct Connect {
        struct event* ev;
    };

    // typedef void (*event_callback_fn)(evutil_socket_t, short, void*);
    struct event* ev{event_new(
        base, lfd, EV_READ | EV_PERSIST,
        [](const evutil_socket_t fd, short events, void* args) {
            std::cout << "有新客户端请求链接" << std::endl;
            auto base = (struct event_base*)args;

            auto cfd = Wrap::Accpet(fd, nullptr, nullptr);

            auto readcn = [](const evutil_socket_t fd, short events, void* args) {
                char buf[1024]{0};
                memset(buf, 0, sizeof(buf));
                int n{Wrap::Read(fd, buf, sizeof(buf))};

                if (n <= 0) {
                    std::cout << "客户端断开链接" << std::endl;
                    close(fd);
                    auto c = (struct Connect*)args;
                    event_del(c->ev);
                    event_free(c->ev);
                    delete c;
                } else {
                    for (size_t i = 0; i < n; ++i) {
                        buf[i] = toupper(buf[i]);
                    }

                    Wrap::Write(fd, buf, (size_t)n);
                }
            };

            auto c = new Connect{};
            c->ev = event_new(base, cfd, EV_READ | EV_PERSIST, readcn, c);

            event_add(c->ev, nullptr);
        },
        base)};

    if (ev == nullptr) {
        Wrap::PrintError("event_new");
    }

    if (event_add(ev, nullptr) < 0) {
        Wrap::PrintError("event_add");
    }

    event_base_dispatch(base);

    event_base_free(base);
    event_free(ev);
    close(lfd);
}
/**
 * 1、 bufferevent
 *     帮调用者监听 socket 的可读可写事件
 *     用户提供回调事件。
 *
 * 2、bufferevent 的读事件回调发时机
 *    当数据由内核缓冲区导 bufferevent 的读缓冲区的时候；会触发 bufferevent 的回调事件。
 *    读事件回调，需要注意的是；数据由内核导 buffferevent 不是由调用者实现；是 bufferevent
 * 自己内部实现。
 *
 * 3、写回调。当调用者将数据写道bufferevent的写缓冲区；他会自动将数据写入导内核的缓冲区（自动发送）
 *    就会触发写回调
 *
 *
 * */

static void test04() {
    auto fd = tools::tcp(8081);
    Epoll* epoll = new Epoll();

    epoll->attach(fd, [epoll](const int fd) {
        auto cfd = Wrap::Accpet(fd, nullptr, nullptr);
        if (cfd < 0) {
            Wrap::PrintError("accpet error:");
        }

        std::cout << "新客户端链接:" << cfd << std::endl;

        epoll->attach(cfd, [epoll](const int fd) {
            // std::cout << "读取数据" << std::endl;

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
        epoll->wait();
    }

    delete epoll;

    // AcceptAwaiter();
    // ReadAWaiter();
    // WriteAWaiter();
}

class CoEpoll {
public:
    struct promise_type {
    public:
        CoEpoll get_return_object() {
            return CoEpoll{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() {
            return {};
        }

        // 自动释放
        std::suspend_always final_suspend() noexcept {
            std::cout << "free---" << std::endl;
            return {};
        }
        /*
            void return_void() {
            }
        */
        void return_value(int c) {
            v = c;
            std::cout << "set --->:" << c << std::endl;
        }

        void unhandled_exception() {
        }

    public:
        int v{0};
        Epoll* epoll{nullptr};
    };
    using CoHandle = std::coroutine_handle<promise_type>;

public:
    CoEpoll(CoHandle h) : handle(h), epoll(h.promise().epoll) {};
    void setEpoll(Epoll* e) {
        epoll = e;
        handle.promise().epoll = e;
    }

public:
    CoHandle handle{};
    Epoll* epoll{nullptr};
};

struct AcceptAwaiter {
    int fd{0};

    bool await_ready() {
        return false;
    }

    void await_suspend(std::coroutine_handle<CoEpoll::promise_type> h) {
        // ★ 从 promise 拿到 epoll，注册监听 fd
        h.promise().epoll->attach(fd, [h](int) {
            h.resume();  // 事件来了恢复协程
        });
    }

    int await_resume() {
        // 接受新连接
        return Wrap::Accpet(fd, nullptr, nullptr);
    }
};

std::vector<CoEpoll> g_live_task;

Epoll epoll;

static void spawn(CoEpoll task) {
    task.setEpoll(&epoll);
    auto h = task.handle;
    g_live_task.push_back(std::move(task));
    h.resume();
}

struct ReaderAwaiter {
    ReaderAwaiter(int _fd) : fd(_fd) {
    }
    int fd{0};
    bool await_ready() {
        return false;
    }

    void await_suspend(std::coroutine_handle<CoEpoll::promise_type> h) {
        // ★ 从 promise 拿到 epoll，注册监听 fd
        h.promise().epoll->attach(fd, [h](int) {
            h.resume();  // 事件来了恢复协程
        });
    }

    int await_resume() {
        char buf[1024]{0};
        memset(buf, 0, sizeof(buf));
        int n{Wrap::Read(fd, buf, sizeof(buf))};

        if (n <= 0) {
            std::cout << "客户端断开链接" << std::endl;
            close(fd);
        } else {
            for (size_t i = 0; i < n; ++i) {
                buf[i] = toupper(buf[i]);
            }

            Wrap::Write(fd, buf, (size_t)n);
        }
        return 0;
    }
};

static CoEpoll reader_loop(int fd) {
    while (1) {
        auto cfd = co_await ReaderAwaiter{fd};
    }

    co_return 0;
}

static CoEpoll accept_loop() {
    auto fd = tools::tcp(8081);

    while (1) {
        auto cfd = co_await AcceptAwaiter{fd};
        spawn(reader_loop(cfd));
    }

    co_return 0;
}

static void test05() {
    spawn(accept_loop());

    while (1) {
        epoll.wait();
    }
}

// 结束
}  // namespace Asio

void TestAsio() {
    Asio::test05();
    // Asio::test01();
    // Asio::test10();
    // Asio::test11();
    // Asio::test12();
}
