
#include "include/07_system_scheduler/system_scheduler.h"

#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>

#include "include/07_system_scheduler/AsyncScheduler.h"
#include "include/07_system_scheduler/BatchScheduler.h"
#include "include/07_system_scheduler/MessageScheduler.h"
#include "include/07_system_scheduler/backend.h"
#include "include/07_system_scheduler/queue.hpp"

static void test01() {
    SystemSafeQueue<std::string> q{10};
    std::vector<std::thread> produceies{};
    std::vector<std::thread> customeies{};
    std::atomic<size_t> count{0};

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
        customeies.push_back(std::thread([&q, &count]() {
            while (auto v = q.pop()) {
                if (v) {
                    count.fetch_add(1);
                    printf(">>> %s\n", v->c_str());
                }
            }
        }));
    }

    for (auto& p : produceies) {
        p.join();
    }

    q.closed();

    for (auto& p : customeies) {
        p.join();
    }

    std::cout << "size:" << count << std::endl;
}

static void test02() {
    auto backend{std::make_shared<Backend>()};

    for (size_t i = 1; i <= 100; ++i) {
        auto s = i;

        backend->submitExportImageTask(std::move(s));
    }

    auto t = backend->downImageTask();

    for (auto& f : t) {
        for (auto& v : f.get()) {
            printf("[%d]\n", v);
        }
    }
}

static void test03() {
    BatchScheduler<int> schdulers;

    schdulers.enqueue([]() {
        printf("task 1\n");
        return 0;
    });

    schdulers.enqueue([]() {
        printf("task 2\n");
        return 0;
    });

    schdulers.enqueue([]() {
        printf("task 3\n");
        return 0;
    });

    schdulers.run();
}

static void test04() {
    BatchScheduler<std::string> schdulers;

    // 主线程提交任务
    for (size_t i = 0; i < 10; ++i) {
        schdulers.enqueue(
            [](size_t i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return std::string("whx1:") + std::to_string(i);
            },
            i);
    }

    // 启动新线程执行任务
    auto result = schdulers.run();

    // 主线程拿到结果
    for (auto& r : result) {
        std::cout << r << std::endl;
    }
}

static void test05() {
    auto schdulers = std::make_shared<AsyncScheduler<std::string>>();

    // 主线程提交任务
    for (size_t i = 0; i < 100; ++i) {
        schdulers->enqueue(
            [](size_t i) {
                std::this_thread::sleep_for(std::chrono::seconds(2));
                return std::string("whx1:") + std::to_string(i);
            },
            i);
    }

    // 启动新线程执行任务
    // auto result = schdulers.getAwaitResults();
    schdulers->getAsyncResults([](auto&& result) {
        // 主线程拿到结果
        // for (auto& r : result) {
        // std::cout << r << std::endl;
        //}
        std::cout << "后台任务已经执行完毕了:" << result.size() << std::endl;
    });

    // while (true) {
    //  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    printf("~~~ 阻塞线程中 ~~~\n");
    // }
}

static void test06() {
    auto thumbSchdulers = std::make_shared<MessageScheduler<std::string>>([](std::string t) {
        // printf("缩略图生成完成，已经获取 URL ，需要使用setState设置 [%s]\n", t.c_str());
    });

    thumbSchdulers->enqueue([]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::string a = std::string("whx1:") + std::to_string(10);
        return a;
    });

    thumbSchdulers->enqueue([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::string a = std::string("whx2:") + std::to_string(20);
        return a;
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        printf("~~~ UI 线程执行中,【更新DOM】，【接受UI事件】 ~~~\n");
        auto result = thumbSchdulers->poll();
        printf("result[%d]\n", result.size());
    }
}

void SystemScheduler() {
    test06();
    // test05();
    // test04();
    // test03();
    // test02();
    // test01();
};
