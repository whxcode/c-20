#include <cstddef>
#pragma one
#include <condition_variable>
#include <mutex>
#include <queue>
template <typename T>

class SafeQueue {
public:
    SafeQueue(const size_t c) : cap(c) {
    }

public:
    void close() {
        std::lock_guard m{mtx};
        closed = true;

        // 唤醒
        consumerCv.notify_all();
        producerCv.notify_all();
    }

    bool isClosed() {
        return closed;
    }

    void push(T&& v) {
        std::unique_lock m{mtx};

        if (isClosed()) {
            printf(">>> 队列已关闭，无法继续 push 数据\n");
            return;
        }

        // 小于实际容量，可以继续 push;否则阻塞
        producerCv.wait(m, [this] {
            return data.size() < cap;
        });

        data.push(std::move(v));

        m.unlock();
        consumerCv.notify_one();
    }

    void push(T& v) {
        T c = v;
        push(std::move(c));
    }

    T pop() {
        std::unique_lock m{mtx};

        consumerCv.wait(m, [this] {
            return !data.empty() || isClosed();  // 队列不为空时；可以继续消费
        });

        if (isClosed() && data.empty()) {
            return T{};
        }

        T v = std::move(data.front());
        data.pop();

        m.unlock();
        producerCv.notify_one();

        return v;
    }

    size_t size() {
        std::lock_guard m{mtx};
        return data.size();
    }

private:
    const size_t cap{0};
    bool closed{false};

    std::mutex mtx{};
    std::queue<T> data{};
    std::condition_variable consumerCv{};
    std::condition_variable producerCv{};
};
