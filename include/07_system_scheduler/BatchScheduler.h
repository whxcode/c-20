#pragma once
#include <functional>
#include <future>

#include "include/07_system_scheduler/queue.hpp"

template <class ReturnValue>
class BatchScheduler {
public:
    using Take = std::function<void()>;

private:
    using Resutls = std::vector<ReturnValue>;
    using FutureResutls = std::vector<std::future<ReturnValue>>;

public:
    BatchScheduler() {
        printf("BatchScheduler thread_max:[%zu]\n", workerCount);
    }

public:
    template <typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) {
        using return_type = typename std::invoke_result<F, Args...>::type;

        // static_assert(std::is_same_v<ReturnValue, ReturnValue>, "all tasks must return
        // ReturnValue.\n");

        static_assert(std::is_same_v<return_type, ReturnValue>,
                      "all tasks must return ReturnValue.");

        auto task = std::make_shared<std::packaged_task<ReturnValue()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        result.push_back(std::move(task->get_future()));

        takes.push([task]() {
            (*task)();
        });
    }

    Resutls run() {
        takes.closed();
        Resutls l;

        for (size_t i = 0; i < workerCount; ++i) {
            workers.emplace_back([this]() {
                while (auto take = takes.pop()) {
                    (*take)();
                }
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }

        for (auto& r : result) {
            l.push_back(r.get());
        }

        return l;
    }

private:
    const size_t workerCount{std::thread::hardware_concurrency()};
    std::vector<std::thread> workers{};
    FutureResutls result{};
    SystemSafeQueue<Take> takes{};
};
