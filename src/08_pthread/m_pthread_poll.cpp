

#include <arpa/inet.h>
#include <sys/socket.h>
// #include <asm-generic/socket.h>
#include <netinet/in.h>
// #include <sys/epoll.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
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

namespace PthreadPoll {
/**
 * 线程池:
 *  1、是一个抽象的概念，若干个线程组合在一起；形成线程池
 *  常见的模型；
 *  epoll 用于并发；pthared_poll 用于完成业务处理;
 *  生产者(N)->消费者模型(M)
 *
 *  1->N
 *  线程池和任务池:
 *    任务池：是一个抽象的概念，若干个任务组合在一起；形成任务池
 *
 *
 *
 * */

struct PoolTask {
    int tasknum{0};
    void* arg{nullptr};
    void (*taskFunc)(void* arg){nullptr};
};

struct ThreadPoll {
    int maxJobNum{0};  // 最大线程格式
    int jobNum{0};     // 初始化任务
    PoolTask* tasks{nullptr};
    int jobPush{0};
    int jobPop{0};

    int threadNum{0};
    pthread_t* threads{nullptr};  // 线程池内的个数
    bool shutdown{false};         // 是否关闭线程池

    pthread_mutex_t poolLock{};
    pthread_cond_t fullTask{};   // 任务队列满的条件
    pthread_cond_t emptyTask{};  // 任务队列步为空的条件
};

// 子线程woker;

void* threadRun(void* data) {
    ThreadPoll* pool = (ThreadPoll*)data;

    return nullptr;
}

// 创建线程池
ThreadPoll* createThreadPoll(const int threadNum, const int maxTaskNum) {
    auto threadPool = new ThreadPoll();

    threadPool->tasks = new PoolTask[(size_t)maxTaskNum];

    threadPool->threads = new pthread_t[(size_t)threadNum];

    pthread_mutex_init(&threadPool->poolLock, nullptr);
    pthread_cond_init(&threadPool->fullTask, nullptr);
    pthread_cond_init(&threadPool->emptyTask, nullptr);

    for (size_t i{0}; i < threadNum; ++i) {
        pthread_create(&threadPool->threads[i], nullptr, threadRun, (void*)threadPool);
    }

    return threadPool;
}

void destroyThreadPoll(ThreadPoll* pool) {
    pthread_mutex_destroy(&pool->poolLock);
    pthread_cond_destroy(&pool->emptyTask);
    pthread_cond_destroy(&pool->fullTask);

    delete pool->tasks;
    delete pool->threads;
    delete pool;
}

void runTask(void* args);  // 运行任务

// 任务回调函数;
// 添加任务
void addTask(ThreadPoll* pool) {
    static int beginnum = 0;
    pthread_mutex_lock(&pool->poolLock);
    // 任务队列满
    //
    while (pool->maxJobNum <= pool->jobNum) {
        pthread_cond_wait(&pool->fullTask, &pool->poolLock);
    }

    int taskPos = (pool->jobNum++) % pool->maxJobNum;

    pool->tasks[taskPos].tasknum = beginnum++;
    pool->tasks[taskPos].arg = (void*)&pool->tasks[taskPos];
    void (*taskRun)(void*);
    pool->tasks[taskPos].taskFunc = taskRun;
    pool->jobNum++;

    pthread_mutex_unlock(&pool->poolLock);

    // 通知存在任务
    pthread_cond_signal(&pool->emptyTask);
}

static void test01() {
    auto pool = createThreadPoll(10, 20);
    addTask(pool);
}

static void test02() {
    int cfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (cfd < 0) {
        Wrap::PrintError("socket error:");
    }

    sockaddr_in server{
        .sin_family = AF_INET,
        .sin_port = htons(8081),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };

    sockaddr_in client{};

    bind(cfd, (sockaddr*)&server, sizeof(server));

    char buf[1024]{0};
    size_t size{1024};

    while (1) {
        // 阻塞等待
        memset(buf, 0, sizeof(buf));

        recvfrom(cfd, buf, sizeof(buf), 0, (sockaddr*)&client, (socklen_t*)sizeof(client));
    }
}

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        /*
         *  手动调用
            std::suspend_always initial_suspend() {
                return {};
            }
      */

        // 立即执行。
        std::suspend_never initial_suspend() noexcept {
            return {};
        }

        std::suspend_always final_suspend() noexcept {
            return {};
        }

        int return_void() {
            return 10;
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

    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "main end\n";

    return 0;
}

/**
 * 本地 socket 通信;通过sockect来实现进程通信
 * int sockect(int domain,int type, int protocol);
 * domain: AF_UNIX or AF_LOCAL /NOTE: 网络通信是 AF_INT （ip_v4）
 * type: SOCK_STREAM or SOCK_DGRAM
 * proctocol: 0;默认协议
 *
 * int bind(int sockfd,const struct sockaddr *addr, socklen_t addrlen);
 *    struct sockaddr_un {
 *        so_family_t sun_family; AF_UNIX
 *        char sun_path[108];  // pathname 文件地址。

 *
 *
 *
 * */

static void test03() {
    sockaddr_un local = {
        .sun_family = AF_UNIX,
        .sun_path = "/home/whx/webProject/c-20/t.s",
    };

    unlink(local.sun_path);  // 删除可能残留的 socket 文件

    auto lfd = Wrap::Socket(local.sun_family, SOCK_STREAM, 0);

    Wrap::Bind(lfd, (sockaddr*)&local, sizeof(local));
    Wrap::Listen(lfd, 10);

    sockaddr_un client{};
    socklen_t len = sizeof(client);

    auto cfd = Wrap::Accpet(lfd, (sockaddr*)&client, &len);

    std::cout << "client:" << client.sun_path << std::endl;
    std::cout << "client:" << client.sun_family << std::endl;

    while (1) {
        char buf[1024]{0};
        auto n = Wrap::Read(cfd, buf, sizeof(buf));
        if (n == 0) {
            std::cout << "client closed" << std::endl;
            close(cfd);
            close(lfd);
            break;
        }

        // std::cout << "recv data: " << buf << std::endl;

        for (size_t i = 0; i < n; ++i) {
            buf[i] = std::toupper(buf[i]);
        }

        Wrap::Write(cfd, buf, (size_t)n);
    }
}

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
}

}  // namespace PthreadPoll
void MScokect() {
    PthreadPoll::test04();
    // PthreadPoll::test03();
    // PthreadPoll::main();
    // PthreadPoll::test01();
}
