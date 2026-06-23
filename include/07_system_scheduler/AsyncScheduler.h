#pragma once
#include <functional>
#include <future>
#include <mutex>

#include "include/07_system_scheduler/queue.hpp"

template <class ReturnValue>
class AsyncScheduler {
public:
    using Take = std::function<void()>;

private:
    using Resutls = std::vector<ReturnValue>;
    using FutureResutls = std::vector<std::future<ReturnValue>>;

public:
    AsyncScheduler() {
        printf("AyncScheduler thread_max:[%zu]\n", workerCount);

        for (size_t i = 0; i < workerCount; ++i) {
            workers.emplace_back([this]() {
                workerLoop();
            });
        }
    }

    ~AsyncScheduler() {
        takes.closed();
        printf("~AsyncScheduler\n");

        if (resultThread_.joinable()) {
            resultThread_.join();
        }

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

        auto task = std::make_shared<std::packaged_task<ReturnValue()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        {
            std::lock_guard<std::mutex> lock(m);
            result.push_back(std::move(task->get_future()));
        }

        takes.push([task]() {
            (*task)();
        });
    }

    Resutls getAwaitResults() {
        Resutls l;

        for (auto& r : result) {
            l.push_back(r.get());
        }

        result.clear();

        return l;
    }

    void getAsyncResults(std::function<void(Resutls&&)>&& callback) {
        resultThread_ = std::thread([this, callback = std::move(callback)]() {
            Resutls l;

            for (auto& r : result) {
                l.push_back(r.get());
            }

            callback(std::move(l));

            result.clear();
        });

        resultThread_.detach();
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
    std::mutex m{};
    FutureResutls result{};
    std::thread resultThread_{};
    const size_t workerCount{std::thread::hardware_concurrency()};
};
