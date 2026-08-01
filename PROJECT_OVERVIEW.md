# Cpp20Study - 单 Reactor 网络服务项目

## 项目定位
自己实现的单 Reactor + 非阻塞 IO 网络服务框架（教学/学习性质），底层封装了 epoll(Linux) / kqueue(macOS)。

## 目录结构

```
.
├── main.cpp          # 入口: Server ser; ser.run();
├── CMakeLists.txt    # C++20, 依赖 libevent (当前仅用了 Threads, libevent 是预留)
│
├── include/
│   ├── server.h      # Server 类定义 - 监听者 + Reactor 事件循环
│   ├── session.h     # Session 类定义 - 单客户端连接处理
│   ├── handle.h      # IHandle 纯虚接口: virtual void handle() = 0
│   └── tools/
│       ├── tcp.h     # socket 操作函数声明 (namespace tcp)
│       └── net.hpp   # epoll/kqueue 统一封装 (namespace net) - 头文件内联实现
│
└── src/
    ├── server.cpp    # Server 实现 - Reactor 事件循环
    ├── session.cpp   # Session 实现 - 读客户端数据
    └── tools/
        └── tcp.cpp   # createSockect / accept / read / write 实现
```

## 架构图

```
main()
  └── Server::run()
        ├── epfd = net::create()           // 创建 epoll 实例
        ├── lfd = tcp::createSockect(8081) // 监听 socket
        ├── net::ctl(ADD, lfd, this)       // 注册监听 fd
        │
        └── while (1)  ←── 这就是 Reactor 主循环
              ├── n = net::wait(epfd, es)  // 等事件
              └── for (每个事件)
                    └── handler->handle()  // 回调分发
                          ├── Server::handle()   → accept → new Session → 注册到 epoll
                          └── Session::handle()  → tcp::read → 处理数据
```

## 核心设计

### Reactor 模式（单线程单 Reactor）
- 一个 `epoll_wait` 循环 + 事件驱动回调
- `IHandle` 是所有事件处理器的虚基类
- `Server` 既是 Listener（监听 accept）又是 EventLoop（跑事件循环）

### IO 模型
- **边缘触发 (ET)** + 非阻塞 fd
- `accept4()` 直接设置 `SOCK_NONBLOCK`
- `tcp::read` 循环读直到 `EAGAIN`

### 跨平台
- `net.hpp` 用 `#ifdef __linux__` 封装 epoll，`__APPLE__` 封装 kqueue
- 统一接口: `net::create()`, `net::ctl()`, `net::wait()`

## 当前已知问题

### 1. 严重 Bug — 事件循环只处理了第一个事件
`src/server.cpp:32`:
```cpp
// 错误的: 永远只处理 es[0]
static_cast<IHandle*>(es->udata)->handle();
// 正确的:
static_cast<IHandle*>(es[i].udata)->handle();
```
后果：有 N 个事件就绪时，只处理第 1 个，剩下 N-1 个被丢弃。

### 2. ET 模式下 accept 可能丢连接
`src/tools/tcp.cpp:44` — `tcp::accept` 一次只 accpet 一个连接。ET 模式下多个连接同时到达，只 accept 一个就返回了，剩下的连接没人处理。
解决：需要 `while (1) { accept(); if (EAGAIN) break; }` 循环到 EAGAIN。

### 3. `tcp::read` 的边界问题与 ET 读取规则（2025-07-14 讨论中）

当前代码 (`src/tools/tcp.cpp:59`) 采用**内层 while 读到 EAGAIN** 的策略（方案 B）。目前有两个未解决的问题：

#### 3a) buf 满了但还没读到 EAGAIN → 丢数据
```
客户端发 2048 字节, buf 只有 1024
  tcp::read 内层 while:
    第 1 轮 ::read → 读 1024, n=1024
    第 2 轮 ::read(buf+1024, 0)  → size=0, Linux 返回 0
                                        ↓
                                被误判为"客户端关闭" !
```
或者在进入第 2 轮前加 `if (n >= bufSize) break;` —— 但这样 buf 满了就退出，内核还剩 1024 字节没读掉。
**ET 模式下 epoll 不会再通知**，数据烂在内核缓冲区。

#### 3b) 内层 while 每次 ::read 都往 buf 开头写
```cpp
::read(fd, buf, bufSize);  // 不是 buf+n
```
第二次读取会把前一次的数据覆盖掉。需要改成 `::read(fd, buf + n, bufSize - n)`。

#### 3c) EAGAIN 时 `return nread` 丢了累积的 n
当前 EAGAIN 时 `return nread;`（即 -1），调用方只知道"出错了"，不知道实际读了多少字节。
应该 `return n;` 让调用方知道已经读了多少数据。

### 4. `Session::handle` 中的错误（`src/session.cpp:25`）

```cpp
void Session::handle() {
    while (1) {                    // 外层 while + 内层 tcp::read
        memset(buf, 0, sizeof(buf));
        auto n = tcp::read(cfd, buf, sizeof(buf));

        if (n == 0) {
            close(cfd);            // 没 return，会继续往下走！
        }
        // n>0/n<0 的处理也是平铺 if，没用 if-else if
    }
    cout << buf;                   // 放循环外，只打印最后一次
}
```
问题点：
- `n==0` 后没有 `return`，逻辑穿透
- 三个互斥条件用平铺 `if` 没有 `else if`
- `cout` 在循环外，只打印最后一次 memset 之后的 buf
- `memset` 在循环内，前一次数据被清掉

## 构建与运行

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
./study_app
# 服务监听 0.0.0.0:8081
```

## 教学路线（待探讨）
- [ ] Reactor 核心原理
- [ ] ET vs LT 取舍
- [ ] TCP 粘包与缓冲区设计
- [ ] 优雅关闭（SIGTERM 处理）
- [ ] 多线程 Reactor / WorkQueue
- [ ] 超时管理
