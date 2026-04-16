#include "include/04Sync/sync.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "include/SafeQueue.hpp"
#include "include/common.h"

bool flag{};
std::mutex m{};
size_t count{0};

static void wait_for_flag() {
    std::unique_lock<std::mutex> lk(m);
    while (!flag) {
        lk.unlock();                                                  // 1 解锁互斥量
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 2 休眠100ms
        lk.lock();                                                    // 3 再锁互斥量
    }
}

static void processData(size_t i) {
    std::unique_lock mk(m);
    printf("正在写入[%d]\n", i);

    count++;
    mk.unlock();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // 模拟耗时任务
}

static void test01() {
    std::vector<std::thread> threads;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(std::thread{processData, i});
    }

    for (auto& t : threads) t.join();
}

static void test02() {
    std::queue<int> dataQueue{};
    std::mutex mtx{};
    std::condition_variable consumerCv{};
    std::condition_variable producerCv{};

    auto writeDataThread = [&dataQueue, &mtx, &consumerCv, &producerCv]() {
        while (true) {
            // std::this_thread::sleep_for(std::chrono::milliseconds(1000));  // 模拟数据生成的时间
            //

            auto dataThunk = generateRandom();
            std::unique_lock m(mtx);

            producerCv.wait(m, [&dataQueue]() {
                return dataQueue.size() < 5;  // 等待队列有空位
            });

            dataQueue.push(dataThunk);

            consumerCv.notify_one();  // 通知等待的线程有新数据了
        }
    };

    auto readDataThread = [&dataQueue, &mtx, &consumerCv, &producerCv](size_t i) {
        while (true) {
            std::unique_lock m(mtx);

            consumerCv.wait(m, [&dataQueue]() {
                return dataQueue.size() > 0;
            });  // 等待数据队列非空

            auto f = dataQueue.front();
            dataQueue.pop();
            auto c = dataQueue.size();

            m.unlock();
            producerCv.notify_one();

            std::cout << i << "-->读取" << f << "剩余:" << c << std::endl;
        }
    };

    std::thread writer1(writeDataThread);
    std::thread writer2(writeDataThread);
    // std::thread writer3(writeDataThread);
    // std::thread writer4(writeDataThread);

    std::thread reader1{readDataThread, 1};
    std::thread reader2{readDataThread, 2};
    std::thread reader3{readDataThread, 3};
    std::thread reader4{readDataThread, 4};
    std::thread reader5{readDataThread, 5};
    std::thread reader6{readDataThread, 6};

    writer1.join();
    writer2.join();
    // writer3.join();
    // writer4.join();

    reader1.join();
    reader2.join();

    reader3.join();
    reader4.join();

    reader5.join();
    reader6.join();
}

static void test03() {
    SafeQueue<int> chan{5};                // 只有 5 个位置
    std::atomic<int> active_producers{3};  // 初始 3 个生产者                        //
    // 启动 3 个生产者
    std::vector<std::thread> producers;
    for (int i = 0; i < 3; ++i) {
        producers.emplace_back([&chan, i, &active_producers]() {
            for (int j = 0; j < 10; ++j) {
                chan.push(i * 100 + j);  // 使用你喜欢的 < 运算符
                printf("生产者 %d 放入了数据 %d\n", i, i * 100 + j);
            }

            if (--active_producers == 0) {
                printf(">>> 所有生产者已完成，正在关闭通道...\n");
                chan.close();
            }
        });
    }

    /*
      // 启动 2 个消费者
      std::vector<std::thread> consumers;
      for (int i = 0; i < 2; ++i) {
          consumers.emplace_back([&chan, i]() {
              for (int j = 0; j < 10; ++j) {  // 消费者胃口太小，
                  // for (int j = 0; j < 20; ++j) { // 生产者已经关闭了。一直卡死
                  printf("--- 消费者 %d 取走了数据 %d\n", i, chan.pop());
              }
          });
      }
    */

    std::vector<std::thread> consumers;
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&chan, i]() {
            int t{0};

            while (chan.pop(t)) {
                printf("--- 消费者 %d 取走了数据 %d\n", i, t);
            }

            printf(">>> 消费者 %d 发现通道已关闭且没有数据了，正在退出...\n", i);
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    printf("还遗留[%d]个数据\n", chan.size());
}

void Sync04() {
    test03();
    // test02();
    // test01();
}
