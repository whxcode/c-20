#pragma once
#include <coroutine>
#include <iostream>

class CoroTask {
public:
    struct promise_type {
    public:
        CoroTask get_return_object() {
            return CoroTask{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() {
            return {};
        }

        // 自动释放
        std::suspend_always final_suspend() noexcept {
            std::cout << "free---" << std::endl;
            return {};
        }
        /*
            void return_void() {
            }
        */
        void return_value(int c) {
            v = c;
            std::cout << "set --->:" << c << std::endl;
        }

        void unhandled_exception() {
        }

        int v{0};
    };

public:
    CoroTask() = default;
    CoroTask(std::coroutine_handle<promise_type> h) : handle(h) {
    }
    ~CoroTask() {
        if (handle) {
            // handle.destroy();
        }
    }

public:
    bool resume() {
        if (!handle.done()) {
            handle.resume();
        }

        std::cout << "done:" << handle.done() << std::endl;
        /*

          if (handle.promise().v == 1) {
              return false;
          }

          if (handle) {
              handle.resume();
          }
      */

        return true;
    }

private:
    std::coroutine_handle<promise_type> handle;
};
