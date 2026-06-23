#include "include/04Sync/sync.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
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
    SystemSafeQueue<int> chan{5};          // 只有 5 个位置
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

static void test04() {
    auto worker = [](std::function<void(const int value)>&& fn) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        fn(10);
        return 10;
    };

    std::future<int> answer = std::async(std::launch::async, worker, [](const int value) {
        std::cout << "worker执行完了，结果是:" << value << std::endl;
    });

    printf("执行async\n");
    auto v = answer.get();

    std::cout << "获取结果值" << v << std::endl;
}

static void test05() {
    std::mutex mutex{};
    std::condition_variable cv{};
    std::deque<std::packaged_task<int()>> tasks;

    auto loop = []() {

    };

    bool isShutdown = false;
    auto guiShutdownMessage = [&isShutdown]() {
        return isShutdown;
    };

    auto guiThread = [&]() {
        while (!guiShutdownMessage()) {  // 一直启动该软件
            loop();                      //  其他无关紧要的任务
            std::packaged_task<int()> task{};
            std::unique_lock lk(mutex);
            cv.wait(lk, [&] {
                return !tasks.empty() || guiShutdownMessage();
            });

            if (guiShutdownMessage() & tasks.empty()) {
                printf("整个程序关闭\n");
                break;
            }

            task = std::move(tasks.front());
            tasks.pop_front();
            task();
        }
    };

    auto postTaskForGuiThread = [&](auto f) {
        std::packaged_task<int()> task{std::move(f)};
        std::future<int> res = task.get_future();
        std::lock_guard m(mutex);
        tasks.push_back(std::move(task));

        cv.notify_one();

        return res;
    };

    std::thread t1{guiThread};
    std::vector<std::future<int>> futures{};

    while (true) {
        auto s = getchar();

        if (s == 'x') {
            std::lock_guard m{mutex};
            isShutdown = true;
            cv.notify_all();

            break;
        }

        auto f = postTaskForGuiThread([=]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            printf("执行任务[%c]\n", s);
            return 100;
        });

        futures.emplace_back(std::move(f));
    }

    printf("有[%d]个任务\n", futures.size());

    // 使用 ranges 遍历获取所有结果
    std::ranges::for_each(futures, [](auto& f) {
        // get() 会阻塞直到该任务完成
        // 如果任务抛出了异常，也会在这里抛出
        try {
            int result = f.get();
            std::cout << ">>> 最终确认结果: " << result << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "任务执行出错: " << e.what() << std::endl;
        }
    });

    if (t1.joinable()) {
        t1.join();
    }
}

static void test06() {
    auto netWorkThread = [](std::promise<std::string>& p) {
        std::cout << "正在链接\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
        p.set_value("M3_Template_v1.0final.");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "清理工作\n";
    };

    std::promise<std::string> promise{};
    std::future<std::string> future = promise.get_future();
    std::thread t{netWorkThread, std::ref(promise)};

    std::cout << "绘制图层\n";

    auto r = future.get();
    std::cout << "获取结果:" << r << std::endl;

    t.join();
}

static void test07() {
    std::future<int> f = std::async([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 10;
    });

    if (f.wait_for(std::chrono::milliseconds(35)) == std::future_status::ready) {
        auto r = f.get();
        printf("[%d]\n", r);
    } else {
        printf("任务超时\n");
    }
}

void Sync04() {
    test07();
    // test06();
    // test05();
    // test04();
    // test03();
    // test02();
    // test01();
}
