#include "include/03DataShared/datashard.h"

#include <condition_variable>
#include <cstddef>
#include <list>
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

template <typename T>
class List {
public:
    void push(T v) {
        // std::lock_guard m(mtx);

        data.push_back(v);
    }

    size_t size() {
        return data.size();
    }

private:
    std::vector<T> data;
    std::mutex mtx;
};

static void test03() {
    List<size_t> l;
    std::vector<std::thread> producers;
    size_t maxThreads = 1000;

    for (size_t i = 0; i < maxThreads; ++i) {
        producers.emplace_back(
            [&l](size_t i) {
                l.push(i);
            },
            i);
    }

    for (size_t i = 0; i < maxThreads; ++i) {
        producers[i].join();
    }

    printf("[%zu]\n", l.size());
}

class Account {
public:
    std::mutex mtx{};
    float balance{100.f};
};

void transfer(Account& from, Account& to, float amount) {
    // 线程 1 执行：A 给 B 转账 100
    // 线程 2 执行：B 给 A 转账 50
    // std::lock_guard<std::mutex> lock1(from.mtx);  // 线程 1 拿到了 A 的锁
    // 此时线程 2 拿到了 B 的锁
    // std::this_thread::sleep_for(std::chrono::milliseconds(1));  // 增加死锁概率

    // std::lock_guard<std::mutex> lock2(to.mtx);  // 线程 1 想要 B 的锁，但被线程 2 占着，它开始等
    // 线程 2 想要 A 的锁，但被线程 1 占着，它也在等

    std::scoped_lock m(from.mtx, to.mtx);  // 同时拿到 from 和 to 的锁，避免死锁,

    from.balance -= amount;
    to.balance += amount;
}

static void test04() {
    Account A, B;

    std::thread t1([&A, &B]() {
        transfer(A, B, 50.f);
    });

    std::thread t2([&A, &B]() {
        transfer(B, A, 50.f);
    });

    t1.join();
    t2.join();

    printf("A 的余额: %.2f\n", A.balance);
    printf("B 的余额: %.2f\n", B.balance);
}
static void test05() {
    Account A;
    A.balance = 1000.0f;  // 初始 1000 块
    std::vector<std::thread> threads;
    std::mutex mtx;

    // 开启 100 个线程，每个线程都尝试给 A 加 1 块钱
    for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&A, &mtx]() {
            for (int j = 0; j < 1000; ++j) {
                // 【核心竞争点】
                // balance += 1.0f 实际上是：
                // 1. 读取 balance 到寄存器
                // 2. 在寄存器里加 1
                // 3. 把结果写回 balance
                std::lock_guard m(mtx);
                A.balance += 1.0f;
            }
        });
    }

    for (auto& t : threads) t.join();

    // 理论上应该是 1000 + 100 * 1000 = 101000
    // 但实际上，你会发现结果远远小于这个数！
    printf("A 的最终余额: %.2f (预期: 101000.00)\n", A.balance);
}

void DataShared03() {
    test05();
    test04();
    // test03();
    // test02();
    // test01();
}
