#include "include/01Exmale/02.accumulat.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <numeric>
#include <thread>
#include <vector>

template <typename Iteraotr, typename T>
struct AccumulateBock {
    void operator()(Iteraotr first, Iteraotr last, T& result) {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iteraotr, typename T>
T ParallelAccumulate(Iteraotr first, Iteraotr last, T init) {
    size_t length = (size_t)std::distance(first, last);

    if (!length) {
        return init;
    }

    size_t minPerThread = 25;
    size_t maxThreads = (length + minPerThread - 1) / minPerThread;
    size_t hardwarThreads = std::thread::hardware_concurrency();
    auto numThreads = std::min(hardwarThreads != 0 ? hardwarThreads : 2, maxThreads);
    auto blockSize = length / numThreads;
    std::vector<T> result(numThreads);
    std::vector<std::thread> threads(numThreads - 1);

    Iteraotr bockStart = first;
    for (size_t i = 0; i < (numThreads - 1); ++i) {
        Iteraotr blockEnd = bockStart;
        std::advance(blockEnd, blockSize);
        threads[i] =
            std::thread(AccumulateBock<Iteraotr, T>(), bockStart, blockEnd, std::ref(result[i]));
        bockStart = blockEnd;
    }

    AccumulateBock<Iteraotr, T>()(bockStart, last, result[numThreads - 1]);
    std::ranges::for_each(threads, [&](std::thread& n) {
        n.join();
    });

    return std::accumulate(result.begin(), result.end(), init);
}

static void test01() {
    std::vector<float> f1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::cout << "):" << ParallelAccumulate(f1.begin(), f1.end(), 0.0f) << std::endl;
    std::cout << "):" << ParallelAccumulate(f1.begin(), f1.begin(), 0.0f) << std::endl;
}

static void test02() {
    // std::function<void()> f1 = [&f1]() {
    //     std::thread t1{f1};
    //     t1.join();
    // };
    //
    // // f1();
}

static void test03() {
    std::thread t1{[]() {
    }};

    auto t2 = std::move(t1);
    t1 = std::thread{[]() {
    }};
    printf("t1 = after\n");
    t2.join();
}

static void test04() {
    std::vector<int> dogs = {1, 2, 3, 4};
    auto& f1 = dogs[1];  // -> int *f1 = dogs + 1;

    for (size_t i = 0; i < 100000; ++i) {
        dogs.push_back(i * 10);
    }

    f1 = 100;  // -> *f1 = 100

    printf("f1[%d],dogs_1[%d]\n", f1, dogs[1]);
    printf("%p\n", &f1);
    printf("%p\n", &dogs[1]);
}

void Accumlat02() {
    test04();
    // test03();
    // test02();
}
