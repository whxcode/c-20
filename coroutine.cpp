#include <unistd.h>

#include <chrono>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <future>
#include <iostream>
#include <thread>

#include "CoroTask.hpp"

void coro1(int max) {
    std::cout << "CORO " << max << " start\n";
    for (int val = 1; val <= max; ++val) {
        std::cout << "CORO " << val << "/" << max << "\n";
    }
    std::cout << "CORO " << max << " end\n";
}

CoroTask coro2(int max) {
    std::cout << "  CORO " << max << " start\n";

    for (int val = 1; val <= max; ++val) {
        // 打印下一个值：
        std::cout << "  CORO   " << val << "/" << max << "\n";
        co_await std::suspend_always{};  // 暂停
    }

    std::cout << " CORO " << max << " end\n";
    co_return 1;
}

void static test01() {
    // 启动协程：
    auto coroTask = coro2(3);
    std::cout << "coro() started\n";

    coroTask.resume();
    coroTask.resume();
    coroTask.resume();
    coroTask.resume();
    coroTask.resume();

    coroTask.resume();
    coroTask.resume();
    // coroTask.resume();

    std::cout << "coro() done\n";
    sleep(2);
    // coro1(3);
}

int main() {
    test01();
    return 0;
}
