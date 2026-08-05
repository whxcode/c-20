#pragma once
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>

#include "include/ctx.h"

template <class T>
class SafeQueue {
public:
    SafeQueue(const size_t cap) : cCap(cap) {
    }

    void push(T&& item) {
        std::unique_lock lg(cMtx);

        if (cClose) {
            return;
        }

        cCusumer.wait(lg, [this]() {
            return cQueue.size() < cCap || cClose;
        });

        if (cClose) {
            return;
        }

        cQueue.push(std::move(item));

        cProducer.notify_one();
    }

    bool tryPush(T&& item) {
        {
            std::unique_lock lock{cMtx, std::try_to_lock};
            if (!lock.owns_lock()) {
                return false;
            }

            if (cClose || cQueue.size() >= cCap) {
                return false;
            }

            cQueue.push(std::move(item));

            lock.unlock();
        }

        cProducer.notify_one();

        return true;
    }

    void pop(std::optional<T>& item) {
        std::unique_lock lg(cMtx);

        if (cClose && cQueue.empty()) {
            return;
        }

        cProducer.wait(lg, [this]() {
            return !cQueue.empty() || cClose;
        });

        if (cClose) {
            return;
        }

        item = std::move(cQueue.front());
        cQueue.pop();

        cCusumer.notify_one();
    }

    void close() {
        std::lock_guard lg(cMtx);
        cClose = true;
        cProducer.notify_all();
        cCusumer.notify_all();
    }

    bool isClosed() const {
        return cClose;
    }

    ~SafeQueue() {
        {
            std::lock_guard lg(cMtx);
            cProducer.notify_all();
            cCusumer.notify_all();
        }
    }

private:
    size_t cCap{10};
    bool cClose{false};

    std::condition_variable cProducer{};
    std::condition_variable cCusumer{};
    std::mutex cMtx{};
    std::queue<T> cQueue;
};

class Workers {
public:
    using Handle = std::function<void()>;

public:
    Workers() {
        std::cout << "Workers thread num: " << cThreadNum << std::endl;

        for (size_t i{0}; i < cThreadNum; ++i) {
            cThreads.emplace_back([this]() {
                while (true) {
                    std::optional<Handle> handle;
                    cQueue->pop(handle);

                    if (!handle.has_value()) {
                        break;
                    }

                    (*handle)();
                }
            });
        }
    }

    ~Workers() {
        close();
    }

public:
    void push(Handle&& handle) {
        cQueue->push(std::move(handle));
    }

    bool tryPush(Handle&& handle) {
        return cQueue->tryPush(std::move(handle));
    }

    void close() {
        if (cClose) {
            return;
        }

        cClose = true;
        cQueue->close();

        for (auto& thread : cThreads) {
            thread.join();
        }
    }

public:
    bool cClose{false};
    std::vector<std::thread> cThreads{};
    sp<SafeQueue<Handle>> cQueue{std::make_shared<SafeQueue<Handle>>(10)};
    size_t cThreadNum{std::thread::hardware_concurrency() * 2};
};
