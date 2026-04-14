#include "include/01Exmale/01.example.h"

#include <algorithm>
#include <functional>
#include <ranges>
#include <thread>
#include <vector>

void test01() {
    auto hello = [](size_t i) {
        printf("hello[%zu]\n", i);
    };

    // 1. 批量启动：这才是真并行
    std::vector<std::thread> threads;
    std::ranges::for_each(std::views::iota(0, 10), [&](int n) {
        threads.emplace_back(hello, n);
    });

    // 2. 批量收网
    std::ranges::for_each(threads, [](std::thread& t) {
        if (t.joinable()) t.join();
    });
}

void Example01() {
    test01();
}
