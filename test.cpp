#include <unistd.h>

#include <future>
#include <iostream>
int main() {
    std::promise<int> value{};
    auto g = value.get_future();

    std::thread([&value]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        value.set_value(42);
    }).join();

    std::cout << g.get() << std::endl;

    std::async(std::launch::async, [&value]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }).wait();

    std::packaged_task<int()> task([]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return 42;
    });
    auto f = task.get_future();

    std::thread(std::move(task)).detach();
    std::cout << "f" << f.get() << std::endl;
    sleep(10);
}
