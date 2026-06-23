#include "include/08_pthread/m_pathred.h"

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <optional>
#include <thread>
#include <vector>

namespace pthreadApi {
/**
 * 1、系统分配资源的基本单位是“进程”
 * 2、系统调度进程的最基本单位是 “线程”
 * 3、多个子线程和主线程共享一个地址空间，只有一个唯一的 PID
 * 4、通过线程ID来区分不同线程
 * 5、内核以 PCB 调度最小执行单位；每一个线程都有一个 PCB
 *
 *
 * 线程:
 *   内核区： PCB
 *   用户区:
 *    环境变量
 *    命令行参数
 *    动态加载区域
 *    堆
 *    栈
 *    .txt
 *    .bss
 *    .dat
 *
 *    受保护区域
 *
 *
 * 6、除开 "栈[是相对于子线程自身栈内的数据；如果栈的变量生命周期长则可以共享]"
 * 不能共享，其他数据都能在线程之间互相共享
 * 7、主线程和子线程的执行优先级由系统调度确定；默认无法人为干预
 *
 *
 * api:
 *  pthread_create() 创建线程
 *  pthread_exit() 线程退出
 *  pthread_join() 阻塞
 *  pthread_detach() 分离线程;不阻塞
 *  pthread_cancel() 取消线程 -> 设置取消点 pthread_testcancel()
 * */

static void test01() {
    pthread_t t;

    pthread_create(
        &t, nullptr,
        [](void* data) -> void* {
            printf("data_[%d]\n", (intptr_t)(data));
            printf("pthread pid_[%d],pthread_id[%d]\n", getpid(), pthread_self());
            return (void*)200;
        },
        (void*)10);

    long int data{0};

    printf("main pid_[%d],pthread_id[%d]\n", getpid(), pthread_self());
    pthread_join(t, (void**)&data);

    printf("data[%d]\n", data);
}

static void test02() {
    pthread_t t;
    struct Person {
        std::string name{"王恒星"};
        uint32_t age{18};
        void dump() {
            printf("name[%s],age[%d]\n", name.c_str(), age);
        }
    };

    Person p;
    pthread_create(
        &t, nullptr,
        [](void* data) -> void* {
            auto person = (Person*)data;
            person->dump();

            // printf("pthread pid_[%d],pthread_id[%d]\n", getpid(), pthread_self());
            return (void*)200;
        },
        &p);

    long int data{0};

    pthread_join(t, (void**)&data);
}

static void test03() {
    std::vector<pthread_t> pthreads{};
    pthreads.resize(5);

    for (size_t i = 0; i < 5; ++i) {
        pthread_create(
            &pthreads[i], nullptr,
            [](void* data) -> void* {
                printf("i'm is the [%d] pthread\n", (intptr_t)data);

                return nullptr;
            },
            (void*)i);
    }

    for (size_t i = 0; i < pthreads.size(); ++i) {
        pthread_join(pthreads[i], nullptr);
    }
}

static void test04() {
    int counter{0};
    std::vector<pthread_t> pthreads{};
    pthreads.resize(5);
    pthread_exit(nullptr);

    for (size_t i = 0; i < 5; ++i) {
        pthread_create(
            &pthreads[i], nullptr,
            [](void* data) -> void* {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                printf("i'm is the [%d] pthread\n", *(intptr_t*)data);
                // printf("i'm is the [%d] pthread\n", *(int*)data);

                return nullptr;
            },
            (void*)&counter);

        ++counter;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    for (size_t i = 0; i < pthreads.size(); ++i) {
        pthread_join(pthreads[i], nullptr);
    }
}

static void test05() {
    /*
     * double-free
    auto a = new int;
    delete a;
    delete a;
  */

    /* SEGV 空指针使用？
      auto f1 = []() -> int& {
          int a{10};
          return a;
      };

      f1() = 10;
    */

    /*
      auto f1 = []() -> int* {
          int a{10};
          return &a;
      };

      *f1() = 10;
    */
    auto a = new int;
}

static void test06() {
    pthread_t t;

    pthread_create(
        &t, nullptr,
        [](void* data) -> void* {
            pthread_exit(nullptr);
            // std::this_thread::sleep_for(std::chrono::seconds(100));
            return (void*)200;
        },
        (void*)10);

    long int data{0};
    std::this_thread::sleep_for(std::chrono::seconds(100));
    // pthread_exit(nullptr);
}

static void test07() {
    pthread_t t;

    pthread_create(
        &t, nullptr,
        [](void* data) -> void* {
            int* a{new int(10)};
            pthread_exit(a);
            return nullptr;
        },
        (void*)10);
    void* p{nullptr};

    pthread_join(t, &p);

    printf("p[%d]\n", *(int*)p);
    // delete (int*)p;
    delete (intptr_t*)p;
}

static void test08() {
    pthread_t t;

    pthread_create(
        &t, nullptr,
        [](void* data) -> void* {
            int* a{new int(10)};
            std::this_thread::sleep_for(std::chrono::seconds(2));
            printf("whxwhx");
            return nullptr;
        },
        (void*)10);

    // void* p{nullptr};
    // pthread_join(t, &p);
    // printf("p[%d]\n", *(int*)p);
    //  delete (int*)p;
    // delete (intptr_t*)p;
    pthread_detach(t);
    auto i = pthread_join(t, nullptr);
    if (i != 0) {
        std::cout << strerror(i) << std::endl;
    }
    printf("test08\n");
}

static void test09() {
    pthread_t t;

    pthread_create(
        &t, nullptr,
        [](void* data) -> void* {
            while (1) {
                int a{0};
                int b{0};
                printf("pthread test09 t2\n");
                // pthread_testcancel();
            }
            return nullptr;
        },
        (void*)10);

    pthread_cancel(t);
    pthread_join(t, nullptr);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_getstack(&attr, nullptr, nullptr);
}

static void test10() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    size_t size{0};
    void* addr{nullptr};
    pthread_attr_getstack(&attr, &addr, &size);
    pthread_attr_getstacksize(&attr, &size);
    std::cout << addr << ":" << size << std::endl;
}
}  // namespace pthreadApi
//
namespace PthreadSync {
/**
 * 线程 1；得到 CPU的时间片；在执行
 * ++ data;
 * 寄存器内是:
 *  read of memory data -> cpu -> l1_cache;
 *   l1_cache add data
 *  write li_cache data -> memory
 *
 *
 * 互斥锁：
 *  1. 创建一把锁 pthread_mutex_t mutex{}; // 可以认为等于 1
 *  2. 初始化锁： pthread_mutex_init(&mutex,NULL);
 *  3. 使用锁:
 *     在临界区中加锁/解锁
 *     pthread_mutex_lock(&mutex); // mutex --
 *        临界区
 *     pthread_mutex_unlock(&mutex); // mutex ++
 *
 *  4. 销毁锁:
 *     pthread_mutex_destory(&mutex) // 销毁资源？
 *
 *  NOTE: 注意就算加锁了；临界区依然某步依然会失去 cpu 时间片；（可能让其线程、程序执行）。
 *        然后下一次在继续执行。
 * 所以无论是原子操作；还是加锁的临界区不存在就要去CPU一个时间片内执行完毕。
 *
 * 死锁:
 *  1.  自己锁自己 RAII 机制
 *  2.
 * A线程占用A锁，又想去占用B锁；B线程占用B锁，又想去占用A锁；互相等待对方释放锁；造成死锁获取B锁，
 *      解决办法可以按照同一顺序加锁、解锁。
 *      或使用 trylock 来避免死锁；如果获取锁失败则不等待直接返回；或者使用 std::lock
 * 来同时加锁多个锁；保证不会死锁。
 *  3.  加锁完毕忘记解锁
 *
 *  读写锁:
 *    1.  线程A加写锁成功，线程B请求读锁
 *       e: 线程B会阻塞；等待线程A释放写锁，线程B继续执行。 A->B
 *    2.  线程A持有读锁；线程B请求写锁
 *       e: 线程B阻塞；等线程A释放；线程B继续执行。 A->B
 *    3. 线程A有用读锁；线程B请求读锁
 *       e: 线程A、B 互不影响继续执行。 [A,B]
 *    4. 线程A持有读锁；然后线程B请求写锁；线程C请求读锁。
 *       1. 是线程A、B继续执行。然后释放之后；线程B继续执行。 [A,B]-C
 *       2. 是线程A释放之后；线程B执行->线程C继续执行？    A->B->C
 *    5. 线程A持有写锁；线程B请求读锁；线程C请求写锁
 *       A->C->B
 *
 *    其核心问题：读锁是共享的？意味着我线程A持有读锁；正在执行；然后线程N也请求读锁（期间没有请求写锁的线程）
 *    是不是线程A..线程N都不会发生任何阻塞？
 *    如果期间有一个线程W请求写锁？还没来的执行（后面启动的读线程）将会一直阻塞；等待线程W执行完毕之后；才会继续执行？
 *    还是线程W。依旧执行
 *
 *    总结一下:
 *      加读锁: 不阻塞读锁。
 *      加写锁:
 * 阻塞后期所有读、写锁；直到释放。释放之后也是优先激活写锁线程；读锁线程调度优先级低（优先队列）?
 *
 * */
static void test01() {
    int num{0};
    pthread_t t1;
    pthread_t t2;

    pthread_create(
        &t1, nullptr,
        [](void* data) -> void* {
            int* n = static_cast<int*>(data);
            for (size_t i = 0; i < 10; ++i) {
                printf("[%d]\n", (*(int*)data));

                int old = *n;
                sched_yield();
                *n = old + 1;
            }
            return nullptr;
        },
        (void*)&num);

    pthread_create(
        &t2, nullptr,
        [](void* data) -> void* {
            int* n = static_cast<int*>(data);
            for (size_t i = 0; i < 10; ++i) {
                printf("[%d]\n", (*(int*)data));
                int old = *n;
                sched_yield();
                *n = old + 1;
            }
            return nullptr;
        },
        (void*)&num);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    std::cout << "num:" << num << std::endl;
}

static void test02() {
    int num{0};

    pthread_t t1;
    pthread_t t2;

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, nullptr, 2);

    auto worker = [](void* data) -> void* {
        auto* args = static_cast<std::pair<int*, pthread_barrier_t*>*>(data);

        int* num = args->first;
        pthread_barrier_t* barrier = args->second;

        int old = *num;  // 两个线程都先读到 0

        pthread_barrier_wait(barrier);  // 等两个线程都读完

        *num = old + 1;  // 两个线程都写 1

        return nullptr;
    };

    std::pair<int*, pthread_barrier_t*> args{&num, &barrier};

    pthread_create(&t1, nullptr, worker, &args);
    pthread_create(&t2, nullptr, worker, &args);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_barrier_destroy(&barrier);

    std::cout << "num:" << num << std::endl;
}

static void test03() {
    int num{0};

    pthread_t t1;
    pthread_t t2;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, nullptr);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, nullptr, 2);

    auto worker = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 3>*>(data);

        int* num = (int*)(*args)[0];
        // printf("num[%d]\n", num);

        pthread_barrier_t* barrier = (pthread_barrier_t*)((*args)[1]);

        pthread_mutex_t* mutex = (pthread_mutex_t*)((*args)[2]);

        pthread_mutex_lock(mutex);
        int old = *num;  // 两个线程都先读到 0

        printf("ready...\n");
        // pthread_barrier_wait(barrier);  // 等两个线程都读完
        printf("go...\n");

        *num = old + 1;  // 两个线程都写 1
        pthread_mutex_unlock(mutex);

        return nullptr;
    };

    // printf("num[%d]\n", &num);
    // std::pair<int*, pthread_barrier_t*> args{&num, &barrier};
    std::array<void*, 3> args{&num, &barrier, &mutex};

    pthread_create(&t1, nullptr, worker, &args);
    pthread_create(&t2, nullptr, worker, &args);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);

    std::cout << "num:" << num << std::endl;
}

static void test04() {
    pthread_t t1;
    pthread_t t2;
    pthread_mutex_t mutexA;
    pthread_mutex_t mutexB;
    pthread_mutex_init(&mutexA, nullptr);
    pthread_mutex_init(&mutexB, nullptr);

    auto worker1 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[1]);

        pthread_mutex_lock(mutexA);
        sleep(1);
        pthread_mutex_lock(mutexB);

        printf("worker1 go...\n");

        pthread_mutex_unlock(mutexB);
        pthread_mutex_unlock(mutexA);
        return nullptr;
    };

    auto worker2 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[1]);

        pthread_mutex_lock(mutexA);
        sleep(1);
        pthread_mutex_lock(mutexB);

        printf("worker2 go...\n");

        pthread_mutex_unlock(mutexB);
        pthread_mutex_unlock(mutexA);
        return nullptr;
    };

    auto worker3 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[0]);

        pthread_mutex_lock(mutexA);
        pthread_mutex_unlock(mutexB);
        sleep(1);
        pthread_mutex_lock(mutexB);

        printf("worker2 go...\n");

        pthread_mutex_unlock(mutexB);
        pthread_mutex_unlock(mutexA);
        return nullptr;
    };

    // printf("num[%d]\n", &num);
    // std::pair<int*, pthread_barrier_t*> args{&num, &barrier};
    std::array<void*, 2> args{&mutexA, &mutexB};

    // pthread_create(&t1, nullptr, worker1, &args);
    //  pthread_create(&t2, nullptr, worker2, &args);
    pthread_create(&t1, nullptr, worker3, &args);

    pthread_join(t1, nullptr);
    // pthread_join(t2, nullptr);

    pthread_mutex_destroy(&mutexA);
    pthread_mutex_destroy(&mutexB);

    // std::cout << "num:" << num << std::endl;
}

static void test05() {
    pthread_t t1;
    pthread_t t2;
    pthread_mutex_t mutexA;
    pthread_mutex_t mutexB;
    pthread_mutex_init(&mutexA, nullptr);
    pthread_mutex_init(&mutexB, nullptr);

    auto worker1 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[1]);

        pthread_mutex_lock(mutexA);  // 锁 A 0
        sleep(1);
        pthread_mutex_lock(mutexB);

        printf("worker1 go...\n");

        pthread_mutex_unlock(mutexB);
        pthread_mutex_unlock(mutexA);
        return nullptr;
    };

    auto worker2 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[1]);

        pthread_mutex_lock(mutexB);
        sleep(1);
        pthread_mutex_lock(mutexA);  //

        printf("worker2 go...\n");

        pthread_mutex_unlock(mutexB);
        pthread_mutex_unlock(mutexA);
        return nullptr;
    };

    std::array<void*, 2> args{&mutexA, &mutexB};

    pthread_create(&t1, nullptr, worker1, &args);
    pthread_create(&t2, nullptr, worker2, &args);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_mutex_destroy(&mutexA);
    pthread_mutex_destroy(&mutexB);

    // std::cout << "num:" << num << std::endl;
}

static void test06() {
    pthread_t t1;
    pthread_t t2;
    pthread_mutex_t mutexA;
    pthread_mutex_t mutexB;
    pthread_mutex_init(&mutexA, nullptr);
    pthread_mutex_init(&mutexB, nullptr);

    auto worker1 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[1]);

        sleep(1);

        printf("worker1 go...\n");

        return nullptr;
    };

    auto worker2 = [](void* data) -> void* {
        auto* args = static_cast<std::array<void*, 2>*>(data);
        auto* mutexA = (pthread_mutex_t*)((*args)[0]);
        auto* mutexB = (pthread_mutex_t*)((*args)[1]);

        sleep(1);

        printf("worker2 go...\n");

        return nullptr;
    };

    std::array<void*, 2> args{&mutexA, &mutexB};

    pthread_create(&t1, nullptr, worker1, &args);
    pthread_create(&t2, nullptr, worker2, &args);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_mutex_destroy(&mutexA);
    pthread_mutex_destroy(&mutexB);

    // std::cout << "num:" << num << std::endl;
}

/**
 * defined: pthread_rwlock_t rwlock;
 * init:  pthread_rwlock_init(&rwlock,nullptr)
 * rdlock:
 *  pthread_rwlock_rdlock(&rwlock)
 *  pthread_rwlock_wrlock(&rwlock)
 *  ... 临界区
 *  unlock:
 *  pthread_rwloack_unlock(&rwlock)
 *  destory:
 *  pthread_rwlock_destroy(&rwlock)
 *
 * */
int number = 0;
pthread_rwlock_t rwlock;

static void test07() {
    pthread_t pthreads[8];
    int arr[8];

    pthread_rwlockattr_t attr;
    pthread_rwlockattr_init(&attr);

    pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);

    pthread_rwlock_init(&rwlock, &attr);

    for (size_t i = 0; i < 3; ++i) {
        arr[i] = i;
        pthread_create(
            &pthreads[i], nullptr,
            [](void* data) -> void* {
                int i = *(int*)data;
                int cur{0};

                while (true) {
                    pthread_rwlock_wrlock(&rwlock);
                    cur = number;
                    cur++;
                    usleep(500);
                    number = cur;

                    printf("----w_p[%d]-[%d]\n", i, cur);
                    pthread_rwlock_unlock(&rwlock);
                }

                return nullptr;
            },
            arr + i);
    }

    for (size_t i = 3; i < 8; ++i) {
        arr[i] = i;

        pthread_create(
            &pthreads[i], nullptr,
            [](void* data) -> void* {
                int i = *(int*)data;

                int cur{0};

                while (true) {
                    cur = number;
                    printf("r_p[%d]-[%d]\n", i, cur);
                    usleep(400);
                }

                return nullptr;
            },
            arr + i);
    }

    for (size_t i = 0; i < 8; ++i) {
        pthread_join(pthreads[i], nullptr);
    }

    pthread_rwlock_destroy(&rwlock);
}
};  // namespace PthreadSync
//
namespace WR {
/**
 * 条件变量:
 * 不满足: 解锁、并阻塞
 * 满 足:  解除阻塞，加锁 (如果存在多个就很有可能竞争)
 * pthread_cond_boadcast(&cond) // 解除所有阻塞的线程
 * pthread_cond_signal(&cond) // 唤醒至少一个阻塞的线程
 *
 * */
struct Node {
    int data{0};
    Node* next{nullptr};
};

pthread_mutex_t mutex{};
pthread_cond_t cond{};

pthread_cond_t producerCond{};
pthread_cond_t consumerCond{};
size_t maxFoodSize{5};
size_t foodSize{0};

static void test01() {
    Node* head{nullptr};
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&producerCond, nullptr);
    pthread_cond_init(&consumerCond, nullptr);

    auto producer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            pthread_mutex_lock(&mutex);

            if (foodSize > maxFoodSize) {
                printf("full ops\n");
                pthread_cond_wait(&producerCond, &mutex);
                // continue;
            }

            ++foodSize;

            auto newData = new Node;

            if (newData == nullptr) {
                perror("new error.\n");
                exit(-1);
            }

            newData->data = rand() % 1000;

            newData->next = *head;
            *head = newData;

            printf("producer produce...[%d]\n", newData->data);
            pthread_mutex_unlock(&mutex);
            pthread_cond_signal(&consumerCond);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            pthread_mutex_lock(&mutex);

            if (*head == nullptr || foodSize <= 0) {
                printf("empty ops\n");
                // 阻塞等待;并解锁 (等待另外一个线程调用 pthread_cond_signal 函数进行来通知)
                // 解除阻塞并加锁
                pthread_cond_wait(&consumerCond, &mutex);

                // 如果这儿直接解锁；那么后面的代码访问将会出现数据竞争的问题
                // pthread_mutex_unlock(&mutex);
            }

            auto nextHead = (*head)->next;

            printf("consumer consume...[%d]\n", (*head)->data);

            delete (*head);
            *head = nextHead;
            --foodSize;

            pthread_cond_signal(&producerCond);

            pthread_mutex_unlock(&mutex);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return nullptr;
    }};

    constexpr size_t kThreadNum{2};
    constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < middle; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &head);
    }

    for (size_t i = middle; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &head);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }
}  // namespace WR
//
static void test02() {
    Node* head{nullptr};
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    auto producer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            auto newData = new Node;
            if (newData == nullptr) {
                perror("new error.\n");
                exit(-1);
            }

            newData->data = rand() % 1000;

            pthread_mutex_lock(&mutex);
            newData->next = *head;
            *head = newData;

            pthread_mutex_unlock(&mutex);
            // pthread_cond_signal(&cond);
            pthread_cond_broadcast(&cond);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        };
        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            pthread_mutex_lock(&mutex);
            while (*head == nullptr) {
                printf("empty ops\n");
                // 阻塞等待;并解锁
                // 解除阻塞并加锁
                pthread_cond_wait(&cond, &mutex);
                // 如果这儿直接解锁；那么后面的代码访问将会出现数据竞争的问题
                // pthread_mutex_unlock(&mutex);
            }

            auto nextHead = (*head)->next;
            printf("consumer consume...[%d]\n", (*head)->data);

            delete (*head);
            *head = nextHead;

            pthread_mutex_unlock(&mutex);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return nullptr;
    }};

    constexpr size_t kThreadNum{10};
    constexpr size_t blances{2};

    // constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < blances; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &head);
    }

    for (size_t i = blances; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &head);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }
}  // namespace WR
}  // namespace WR
//
// 信号量
namespace Signal {

struct Node {
    int data{0};
    Node* next{nullptr};
};

pthread_mutex_t mutex{};
pthread_cond_t cond{};

pthread_cond_t producerCond{};
pthread_cond_t consumerCond{};
size_t maxFoodSize{5};
size_t foodSize{0};
/**
 *  信号量:
 *   sem_t sem; 类型
 *   pshared = 0;同步进程
 *   value 可并行多少个执行
 *   init sem_init(sem_t*t,int pshard,usigned int value) 初始化函数
 *   int sem_wait() // sem--,当sem=0时；阻塞
 *   int sem_post() // sem++,
 *   int sem_trywait(sem_t *t) 尝试 wait() 不阻塞
 *   int sem_destory() 销毁
 *
 * **/

static void test01() {
    Node* head{nullptr};
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);

    auto producer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            auto newData = new Node;
            if (newData == nullptr) {
                perror("new error.\n");
                exit(-1);
            }

            newData->data = rand() % 1000;

            pthread_mutex_lock(&mutex);
            newData->next = *head;
            *head = newData;

            pthread_mutex_unlock(&mutex);
            // pthread_cond_signal(&cond);
            pthread_cond_broadcast(&cond);
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        };
        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            pthread_mutex_lock(&mutex);
            if (*head == nullptr) {
                printf("empty ops\n");
                // 阻塞等待;并解锁
                // 解除阻塞并加锁
                pthread_cond_wait(&cond, &mutex);
                // 如果这儿直接解锁；那么后面的代码访问将会出现数据竞争的问题
                // pthread_mutex_unlock(&mutex);
            }

            auto nextHead = (*head)->next;
            printf("consumer consume...[%d]\n", (*head)->data);

            delete (*head);
            *head = nextHead;

            pthread_mutex_unlock(&mutex);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return nullptr;
    }};

    constexpr size_t kThreadNum{10};
    constexpr size_t blances{2};

    // constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < blances; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &head);
    }

    for (size_t i = blances; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &head);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }
}  // namespace WR
//
sem_t semProcuder{};
sem_t semConsumer{};

static void test02() {
    Node* head{nullptr};

    constexpr size_t kThreadNum{10};
    constexpr size_t producerNum{5};

    sem_init(&semProcuder, 0, 5);
    sem_init(&semConsumer, 0, 0);

    auto producer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            auto newData = new Node;

            if (newData == nullptr) {
                perror("new error.\n");
                exit(-1);
            }

            newData->data = rand() % 1000;

            sem_wait(&semProcuder);

            // pthread_mutex_lock(&mutex);
            newData->next = *head;
            *head = newData;
            // sem_consumer ++ ; 唤起阻塞的线程
            sem_post(&semConsumer);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        };
        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            // 等待生产者通知？
            sem_wait(&semConsumer);

            if (*head == nullptr) {
                printf("empty ops\n");
                // 阻塞等待;并解锁
                // 解除阻塞并加锁
                // pthread_cond_wait(&cond, &mutex);
                // 如果这儿直接解锁；那么后面的代码访问将会出现数据竞争的问题
                // pthread_mutex_unlock(&mutex);
            }

            auto nextHead = (*head)->next;
            printf("consumer consume...[%d]\n", (*head)->data);

            delete (*head);
            *head = nextHead;

            sem_post(&semProcuder);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return nullptr;
    }};

    // constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < producerNum; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &head);
    }

    for (size_t i = producerNum; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &head);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }

    sem_destroy(&semConsumer);
    sem_destroy(&semProcuder);
}  // namespace WR

template <class T>
class Chain {
public:
    Chain(size_t cap) {
        sem_init(&p, 0, cap);
        sem_init(&c, 0, 0);
    }

public:
    void operator<(const T& inVal) {
        if (isClose()) {
            printf("already close\n");
            return;
        }

        sem_wait(&p);  // 先 --

        {
            // push 数据
            std::lock_guard lock(mutex);
            data.push_back(inVal);
        }
        sem_post(&c);
    }

    std::optional<T> operator>(T& _outVal) {
        ++waitNum;

        sem_wait(&c);

        --waitNum;

        if (isEmpty()) {
            return std::nullopt;
        }

        std::optional<T> ret;

        std::lock_guard lock(mutex);
        // pop 数据
        auto outVal = data.back();
        ret = std::make_optional<T>(outVal);
        data.pop_back();

        sem_post(&p);
        return ret;
    }

    bool isClose() {
        std::lock_guard lock(mutex);
        return _close;
    }

    bool isEmpty() {
        std::lock_guard lock(mutex);
        return data.empty();
    }

    void close() {
        std::lock_guard lock(mutex);

        _close = true;

        for (size_t i = 0; i < waitNum; ++i) {
            sem_post(&c);
        }
    }

private:
    std::atomic<size_t> waitNum{0};
    std::vector<T> data{};
    sem_t p{};
    sem_t c{};
    bool _close{false};
    std::mutex mutex{};
};

static void test03() {
    Node* head{nullptr};

    constexpr size_t kThreadNum{10};
    constexpr size_t producerNum{5};

    sem_init(&semProcuder, 0, producerNum);
    sem_init(&semConsumer, 0, 0);

    auto producer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            auto newData = new Node;

            if (newData == nullptr) {
                perror("new error.\n");
                exit(-1);
            }

            newData->data = rand() % 1000;

            sem_wait(&semProcuder);
            printf("---->\n");

            // pthread_mutex_lock(&mutex);
            newData->next = *head;
            *head = newData;
            // sem_consumer ++ ; 唤起阻塞的线程
            sem_post(&semConsumer);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        };
        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        auto head = (Node**)(data);

        while (true) {
            // 等待生产者通知？
            sem_wait(&semConsumer);
            printf("<----\n");

            continue;
            if (*head == nullptr) {
                printf("empty ops\n");
                // 阻塞等待;并解锁
                // 解除阻塞并加锁
                // pthread_cond_wait(&cond, &mutex);
                // 如果这儿直接解锁；那么后面的代码访问将会出现数据竞争的问题
                // pthread_mutex_unlock(&mutex);
            }

            auto nextHead = (*head)->next;
            printf("consumer consume...[%d]\n", (*head)->data);

            delete (*head);
            *head = nextHead;

            // sem_post(&semProcuder);

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }

        return nullptr;
    }};

    // constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < producerNum; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &head);
    }

    for (size_t i = producerNum; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &head);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }

    sem_destroy(&semConsumer);
    sem_destroy(&semProcuder);
}  // namespace WR
//
static void test04() {
    Node* head{nullptr};

    constexpr size_t kThreadNum{10};
    constexpr size_t producerNum{5};

    Chain<int> chain{5};

    auto producer{[](void* data) -> void* {
        auto& chain = *(Chain<int>*)(data);

        for (size_t i = 0; i < 10; ++i) {
            chain < (i + 1) * 10;
        }

        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        int val;
        auto& chain = *(Chain<int>*)(data);

        for (size_t i = 0; i < 10; ++i) {
            chain > val;
            printf("consumer consume...[%d]\n", val);
        }

        return nullptr;
    }};

    // constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < producerNum; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &chain);
    }

    for (size_t i = producerNum; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &chain);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }

}  // namespace WR
//
static void test05() {
    constexpr size_t kThreadNum{3};
    constexpr size_t producerNum{1};

    Chain<int> chain{5};

    auto producer{[](void* data) -> void* {
        auto& chain = *(Chain<int>*)(data);

        chain < 100;
        chain.close();

        return nullptr;
    }};

    auto consumer{[](void* data) -> void* {
        int val;
        auto& chain = *(Chain<int>*)(data);

        printf("准备获取数据\n");
        auto ret = chain > val;
        if (ret) {
            printf("consumer consume...[%d]\n", ret.value());
        } else {
            printf("empty -> data");
        }

        return nullptr;
    }};

    // constexpr size_t middle{kThreadNum / 2};

    pthread_t pthreads[kThreadNum];

    // 生产者
    for (size_t i = 0; i < producerNum; ++i) {
        pthread_create(&pthreads[i], nullptr, producer, &chain);
    }

    for (size_t i = producerNum; i < kThreadNum; ++i) {
        pthread_create(&pthreads[i], nullptr, consumer, &chain);
    }

    // join
    for (size_t i = 0; i < kThreadNum; ++i) {
        pthread_join(pthreads[i], nullptr);
    }

}  // namespace WR

};  // namespace Signal

void MPthread() {
    Signal::test05();
    // Signal::test04();
    // Signal::test03();
    // Signal::test02();

    // Signal::test01();
    // WR::test01();

    // PthreadSync::test07();
    // PthreadSync::test04();
    // PthreadSync::test05();
    // PthreadSync::test04();
    // PthreadSync::test02();
    // PthreadSync::test01();
    // pthreadApi::test10();
    // test08();
    // test07();
    // test06();
    // test05();
    // test04();
    // test03();
    // test02();
}
