#include "include/01Exmale/01.example.h"

#include <algorithm>
#include <cstdio>
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

/*
void a() {
}
*/

struct Func {
    Func(int& i_) : i(i_) {
    }

public:
    int& i;
    void operator()() {
        printf("hello[%d]\n", i);
    }
};

void test02() {
    int i = 10;
    Func f(i);
    std::thread t1(f);
    t1.detach();
}

struct Thread {
public:
    ~Thread() {
        // if (t.joinable()) t.join();
    }

    explicit Thread() {
    }

    Thread(Thread const&) = delete;  // 3
    Thread& operator=(Thread const&) = delete;

public:
    std::thread t{[]() {
        printf("Thread\n");
    }};
};

void test03() {
    /*
      std::thread t1([]() {
          printf("test03\n");
      });
    */
    Thread t1{};
}

void test04() {
    auto f1 = [](int&& i) {
        i = 10;
    };

    int i = 0;

    // std::thread t1{f1, std::ref(i)};
    std::thread t1{f1, i};
    t1.join();

    printf("[%d]\n", i);
}

void test05() {
    auto f1 = [](int* i) {
        *i = 10;
    };

    int i = 0;

    std::thread t1{f1, &i};
    // std::thread t1{f1, i};
    t1.join();

    printf("[%d]\n", i);
}

struct Func1 {
public:
    void test(int i) {
        printf("test[%d]\n", i);
    }
};

std::vector<int>& a() {
    std::vector<int> r;
    // xxx
    // xx
    return r;
}

void test06() {
    auto b = a();
    // xxx
}

void Example01() {
    test06();
    // test05();
    // test04();
    // test03();
    // test02();
    // test01();
    getchar();
}
