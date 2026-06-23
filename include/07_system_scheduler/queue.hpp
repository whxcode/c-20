#pragma once
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <class T>
class SystemSafeQueue {
public:
    SystemSafeQueue() : cap(10) {
    }

    SystemSafeQueue(const size_t _cap) : cap(_cap) {
    }

public:
    void push(T&& value) {
        {
            std::lock_guard lm{mtx};

            queue.push(std::move(value));
        }

        cv.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock um{mtx};

        cv.wait(um, [this]() {
            // 队列为空，阻塞中...
            // if (queue.empty()) {
            // return false;
            //}

            return !queue.empty() || true;
            // return close;
        });

        if (queue.empty()) {
            return std::nullopt;
        }

        auto result = std::make_optional<T>(std::move(queue.front()));
        queue.pop();

        return result;
    }

    std::optional<T> tryPop() {
        std::lock_guard um{mtx};
        if (queue.empty()) {
            return std::nullopt;
        }

        auto result = std::make_optional<T>(std::move(queue.front()));
        queue.pop();

        return result;
    }

    void closed() {
        {
            std::lock_guard lm{mtx};
            close = true;
        }

        cv.notify_all();  // 通知所有等待的线程，避免死锁
    }

    size_t size() {
        std::lock_guard lm{mtx};
        return queue.size();
    }

private:
    std::queue<T> queue{};
    std::mutex mtx{};
    std::condition_variable cv{};
    size_t cap{0};
    bool close{false};
};
