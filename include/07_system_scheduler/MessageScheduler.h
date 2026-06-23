#pragma once
#include <functional>
#include <future>
#include <mutex>

#include "include/07_system_scheduler/queue.hpp"

template <class ReturnValue>
class MessageScheduler {
public:
    using Take = std::function<void()>;
    using CallBack = std::function<void(ReturnValue)>;

private:
    // using Resutls = std::vector<ReturnValue>;
    // using FutureResutls = std::vector<std::future<ReturnValue>>;

public:
    MessageScheduler(CallBack&& c) : callback(c) {
        printf("MessageScheduler thread_max:[%zu]\n", workerCount);

        for (size_t i = 0; i < workerCount; ++i) {
            workers.emplace_back([this]() {
                workerLoop();
            });
        }
    }

    ~MessageScheduler() {
        takes.closed();
        printf("~MessageScheduler\n");

        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

public:
    template <typename F, typename... Args>
    void enqueue(F&& f, Args&&... args) {
        using return_type = typename std::invoke_result<F, Args...>::type;

        static_assert(std::is_same_v<return_type, ReturnValue>,
                      "all tasks must return ReturnValue.");

        auto task = std::make_shared<std::function<ReturnValue()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        takes.push([task, this]() {
            auto t = (*task)();
            results.push(std::move(t));
            printf("--------------\n");
        });
    }

    std::vector<ReturnValue> poll() {
        std::vector<ReturnValue> res{};

        while (auto take = results.tryPop()) {
            res.push_back(*take);
        }

        return res;
    }

private:
    void workerLoop() {
        while (auto take = takes.pop()) {
            (*take)();
        }
    }

private:
    std::vector<std::thread> workers{};
    SystemSafeQueue<Take> takes{};
    SystemSafeQueue<ReturnValue> results{};
    CallBack callback{nullptr};
    const size_t workerCount{std::thread::hardware_concurrency()};
};
