#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

#include "include/06.nowmodel/constrcuor.h"

template <typename T>
class CondtionValueQueue {
public:
    void push(T&& value) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            queue_.push(std::move(value));
        }

        cv_.notify_one();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(mtx_);

        cv_.wait(lk, [&] {
            return closed_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return std::nullopt;  // closed 且没数据
        }

        T value = std::move(queue_.front());
        queue_.pop();

        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }

        cv_.notify_all();
    }

    size_t size() {
        std::lock_guard l(mtx_);
        return queue_.size();
    }

    ~CondtionValueQueue() {
        printf("~BlockingQueue\n");
        close();
    }

private:
    std::queue<T> queue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool closed_{false};
};

static void test01() {
    CondtionValueQueue<std::string> q;
    std::vector<std::thread> produceies{};
    std::vector<std::thread> customeies{};

    size_t p = 10;
    size_t c = 5;

    // produceies.resize(p);
    // customeies.resize(c);

    for (size_t i = 0; i < p; ++i) {
        produceies.push_back(std::thread(

            [&q](size_t id) {
                for (size_t j = 0; j < 10; ++j) {
                    q.push("我是第" + std::to_string(id) + "号生产者，生产了第" +
                           std::to_string(j) + "个数据");
                }
            },
            i)

        );
    }

    for (size_t i = 0; i < c; ++i) {
        customeies.push_back(std::thread([&q]() {
            while (auto v = q.pop()) {
                if (v) {
                    printf(">>> %s\n", v->c_str());
                }
            }
        }));
    }

    for (auto& p : produceies) {
        p.join();
    }

    q.close();

    for (auto& p : customeies) {
        p.join();
    }

    // std::cout << "size:" << q.size() << std::endl;
}

void Scheduler() {
    test01();
}
