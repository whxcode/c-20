
#include <event2/event.h>

#include <iostream>

namespace Asio {
static void test01() {
    const char** p = event_get_supported_methods();
    while (true) {
        if (*p == nullptr) {
            break;
        }

        std::cout << *p << std::endl;
        p++;
    }

    //
    struct event_base* base = event_base_new();
    if (base == nullptr) {
        std::cout << "event_base_new failed" << std::endl;
    }

    const char* method = event_base_get_method(base);
    std::cout << "event_base_get_method: " << method << std::endl;
}

// 结束
}  // namespace Asio

void TestAsio() {
    Asio::test01();
    // Asio::test10();
    // Asio::test11();
    // Asio::test12();
}
