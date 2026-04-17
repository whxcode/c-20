#include <deque>
#include <functional>
#include <future>
#include <vector>

#include "include/04Sync/sync.h"

static void test01() {
    auto worker = [](const size_t i) {
        printf("i[%d]\n", i);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 10;
    };

    std::future<int> fut1 = std::async(std::launch::async, worker, 1);

    std::cout << fut1.get() << std::endl;
    std::cout << "我在处理其它任务1 " << std::endl;

    // std::future<int> fut2 = std::async(std::launch::deferred, worker, 2);
    // fut2.wait();
    // std::cout << "我在处理其它任务2" << std::endl;
}

using ThreadID = size_t;
using Take = std::function<void()>;

class BackgroundWorker {
public:
    BackgroundWorker() {
        for (size_t i = 0; i < workerCount; ++i) {
            workers.emplace_back([i, this] {
                // 接受主进程的任务

                while (true) {
                    std::unique_lock m{mtx};
                    cv.wait(m, [this, i] {
                        auto result = !takes.empty() || stop;
                        return result;
                    });

                    if (takes.empty()) {
                        if (stop) {
                            break;
                        }

                        continue;
                    }

                    auto take = std::move(takes.front());
                    takes.pop_front();

                    m.unlock();

                    take();
                }
            });
        }
    }

    ~BackgroundWorker() {
        {
            std::lock_guard m{mtx};
            stop = true;
        }

        cv.notify_all();

        for (auto& w : workers) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

public:
    void pusTake(Take&& t) {
        std::unique_lock m{mtx};

        takes.push_back(std::move(t));
        cv.notify_one();
    }

    template <typename F, typename... Args>
    auto pusTakeAny(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        // 1. 依然使用 shared_ptr 解决 packaged_task 的拷贝限制
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();

        {
            std::unique_lock m{mtx};
            // 修正 3: Lambda 必须接受一个 ThreadID 参数，才能匹配 std::function<void(ThreadID)>
            takes.push_back([task]() {
                (*task)();
            });
        }

        cv.notify_one();
        return res;
    }

private:
    const size_t workerCount{std::thread::hardware_concurrency()};
    std::vector<std::thread> workers;
    std::mutex mtx{};
    std::condition_variable cv{};
    std::deque<Take> takes{};
    bool stop{false};
};

static void test02() {
    BackgroundWorker backgroundWorker;

    for (size_t i = 0; i < 10; ++i) {
        auto r = backgroundWorker.pusTakeAny(
            [](int a, int b) {
                return a + b;
            },
            10, 20);
        /*
            Take t{[i](const ThreadID id) {
                printf("worker[%d] is processing task[%d]\n", id, i);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }};

            auto future = t.get_future();

            backgroundWorker.pusTake(std::move(t));
            */

        // future.get();

        /*
          if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
              future.get();
              printf("task[%d] is ready\n", i);
          }
      */
    }
}

static void test03() {
    BackgroundWorker backgroundWorker;

    auto r1 = backgroundWorker.pusTakeAny(
        [](int a, int b) {
            return a + b;
        },
        10, 20);

    auto r2 = backgroundWorker.pusTakeAny([]() {
        return "I 'm Whx";
    });

    std::cout << r1.get() << std::endl;
    std::cout << r2.get() << std::endl;
}

void Future() {
    test03();
    // test01();
}
