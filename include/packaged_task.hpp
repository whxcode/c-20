#include <cstddef>
#include <future>
#pragma once
#include <condition_variable>
#include <deque>
#include <mutex>
#include <queue>

using Task = std::packaged_task<void()>;

class PackagedTask {
public:
    void pushTake(std::packaged_task<void()>&& t) {
        std::lock_guard m{mtx};
        takes.push_back(std::move(t));
    }

    Task popTake() {
        std::lock_guard m{mtx};

        if (takes.empty()) {
            return {};
        }
        auto t = std::move(takes.front());

        takes.pop_front();

        return t;
    }

private:
    std::condition_variable cv{};
    std::mutex mtx{};
    std::deque<Task> takes{};
};
