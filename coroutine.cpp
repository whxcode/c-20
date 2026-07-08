#include <chrono>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <future>
#include <thread>

// Task：协程返回对象
// 作用：保存 coroutine_handle，负责最终销毁协程帧
class Task {
public:
    // promise_type：C++ 协程规定必须提供的类型
    // 作用：告诉编译器这个协程怎么创建、怎么暂停、怎么返回、怎么销毁
    struct promise_type {
        // get_return_object：
        // 作用：创建协程函数的返回对象，也就是 Task
        Task get_return_object() {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // initial_suspend：
        // 作用：协程刚创建时，是否立刻暂停
        // suspend_never = 不暂停，马上执行协程函数体
        std::suspend_never initial_suspend() {
            return {};
        }

        // final_suspend：
        // 作用：协程执行到最后时，是否暂停
        // suspend_always = 最后暂停，等待 Task 析构时 destroy
        std::suspend_always final_suspend() noexcept {
            return {};
        }

        /*
             return_void：
             作用：对应 co_return;
            void return_void(int v) {
                result = v;
            }
        */

        // return_void：
        // 作用：对应 co_return;
        void return_value(int v) {
            p.set_value(v);
            // result = v;
        }

        // unhandled_exception：
        // 作用：协程内部异常处理
        void unhandled_exception() {
            std::terminate();
        }

        std::promise<int> p;
    };

    using Handle = std::coroutine_handle<promise_type>;

    // Task 构造函数：
    // 作用：保存协程 handle
    Task(Handle h) : coro(h) {
    }

    int result() {
        return coro.promise().p.get_future().get();
    }

    // 禁止拷贝：
    // 因为一个协程 handle 只能被一个 Task 管理
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    // 支持移动：
    // 允许 Task t = f1(); 这种返回值移动
    Task(Task&& other) noexcept : coro(other.coro) {
        other.coro = nullptr;
    }

    // 析构函数：
    // 作用：销毁协程帧，释放协程保存的局部变量等资源
    ~Task() {
        if (coro) {
            coro.destroy();
        }
    }

private:
    // coroutine_handle：
    // 作用：协程的控制句柄，可以 resume / destroy / done
    Handle coro{};
};

// Awaiter：co_await 后面的对象
// 作用：决定这个 co_await 要不要暂停、暂停后谁来恢复、恢复后返回什么
struct FetchImage {
    // await_ready：
    // 作用：判断这个异步操作是否已经完成
    // true  = 不暂停，直接继续执行
    // false = 需要暂停，进入 await_suspend
    bool await_ready() {
        printf("await_ready\n");
        return false;
    }

    // await_suspend：
    // 作用：协程即将暂停时调用
    // 参数 h：当前协程的 handle
    // 后面可以保存 h，等异步任务完成后调用 h.resume()
    void await_suspend(std::coroutine_handle<> h) {
        printf("await_suspend: 协程暂停，丢给后台线程\n");

        std::thread([h] {
            std::this_thread::sleep_for(std::chrono::seconds(2));

            printf("thread: 后台任务完成，恢复协程\n");

            // 恢复协程
            h.resume();
        }).detach();
    }

    // await_resume：
    // 作用：协程恢复后调用
    // 返回值就是 co_await 表达式的结果
    int await_resume() {
        printf("await_resume\n");
        return 10;
    }
};

struct EncodeImage {
    EncodeImage(int id) : _id(id) {
    }
    // await_ready：
    // 作用：判断这个异步操作是否已经完成
    // true  = 不暂停，直接继续执行
    // false = 需要暂停，进入 await_suspend
    bool await_ready() {
        return false;
    }

    // await_suspend：
    // 作用：协程即将暂停时调用
    // 参数 h：当前协程的 handle
    // 后面可以保存 h，等异步任务完成后调用 h.resume()
    void await_suspend(std::coroutine_handle<> h) {
        std::thread([h] {
            std::this_thread::sleep_for(std::chrono::seconds(2));

            // 恢复协程
            h.resume();
        }).detach();
    }

    // await_resume：
    // 作用：协程恢复后调用
    // 返回值就是 co_await 表达式的结果
    std::string await_resume() {
        return std::string{"feawfawefkwae;ljwak;l[[[" + std::to_string(_id)};
    }
    int _id;
};

// f1：协程函数
// 只要函数内部出现 co_await / co_return / co_yield
// 它就是协程函数
Task f1() {
    printf("f1 start\n");

    // co_await Awaiter{}：
    // 1. 调用 await_ready()
    // 2. 如果返回 false，调用 await_suspend(handle)
    // 3. 协程暂停
    // 4. 以后 h.resume()
    // 5. 调用 await_resume()
    // 6. await_resume 返回值赋给 value
    int value = co_await FetchImage{};
    printf("FetchImage value = %d\n", value);

    auto path = co_await EncodeImage{value};
    printf("EncodeImage value = %s\n", path.c_str());

    co_return 10;
}

static void test01() {
    // 调用 f1() 会创建协程
    // 因为 initial_suspend 是 suspend_never，所以会马上执行 f1 函数体
    Task t = f1();

    printf("main thread continue...[%d]\n", t.result());

    // 这里必须等一下
    // 因为 Awaiter 里开了后台线程，2 秒后才 resume
    // 如果 t 太早析构，协程帧被 destroy，后台线程再 resume 就会崩
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

int main() {
    test01();
    return 0;
}
