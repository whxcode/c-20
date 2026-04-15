#include "include/03DataShared/datashard.h"

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>

template <typename T>
class Channel {
public:
    void send(T msg) {
        std::unique_lock<std::mutex> lock(mtx);

        // 阻塞
        producerCv.wait(lock, [this] {
            return queue.size() < cap;
        });

        queue.push(msg);

        // 唤醒消费者
        consumerCv.notify_one();
    }

    T receive() {
        std::unique_lock<std::mutex> lock(mtx);

        // 如果为空，就阻塞
        consumerCv.wait(lock, [this] {
            return !queue.empty();
        });

        // 唤醒一个生产者

        T msg = queue.front();
        queue.pop();

        producerCv.notify_one();

        return msg;
    }
    void operator<(T msg) {
        send(std::move(msg));
    }

    friend void operator>(T& val, Channel<T>& chain) {
        val = chain.receive();
    }

private:
    size_t cap{5};
    std::queue<T> queue{};
    std::mutex mtx{};
    std::condition_variable consumerCv{};
    std::condition_variable producerCv{};
};

static void test01() {
    Channel<int> chain{};

    std::thread producer{[&chain]() {
        for (size_t i = 0; i < 10; ++i) {
            // std::this_thread::sleep_for(std::chrono::milliseconds(500));
            chain < i * 10;
            // chain.send(i * 10);
        }
    }};

    std::thread consumer{[&chain]() {
        for (size_t i = 0; i < 10; ++i) {
            int data;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            data > chain;
            std::cout << data << std::endl;
        }
    }};

    producer.join();
    consumer.join();
}

static void test02() {
    Channel<int> chan{};  // 只有 5 个位置

    // 启动 3 个生产者
    std::vector<std::thread> producers;
    for (int i = 0; i < 3; ++i) {
        producers.emplace_back([&chan, i]() {
            for (int j = 0; j < 10; ++j) {
                chan < (i * 100 + j);  // 使用你喜欢的 < 运算符
                printf("生产者 %d 放入了数据 %d\n", i, i * 100 + j);
            }
        });
    }

    // 启动 2 个消费者
    std::vector<std::thread> consumers;
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&chan, i]() {
            for (int j = 0; j < 15; ++j) {
                int val;
                // val > chan;  // 使用你喜欢的 > 运算符
                // printf("--- 消费者 %d 取走了数据 %d\n", i, val);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 故意慢一点
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

void DataShared03() {
    test02();
    // test01();
}
