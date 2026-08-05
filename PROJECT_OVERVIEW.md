## 调度模型

Server
= 唯一的 Accept 线程
= 管理者
= 只负责监听 listen fd、accept 新连接、将 client fd 分发给 Ep 线程
= 管理 Ep 线程池与 Workers 线程池

Ep 线程
= I/O 线程 / Event Loop 线程
= 每个线程独占一个 epoll 或 kqueue
= 只负责 reader、writer、close、连接状态
= 不执行 accept
= 不执行业务逻辑

Workers 线程
= 业务线程池
= 执行路由 handler、JSON、数据库、耗时计算等
= 不直接操作 client fd、Ctx::send()、epoll/kqueue

固定流程：

客户端
-> Server（Accept 线程）
-> 某个 Ep 线程
-> Workers 线程池
-> 原来的 Ep 线程
-> 客户端

具体职责边界：

Server
accept(fd)
-> epThread.postNewConnection(fd)

Ep 线程
读完整 HTTP 请求
-> workers.post(业务任务)

Workers
Response response = handler(request)
-> epThread.postResponse(connectionId, std::move(response))

Ep 线程
ctx.setResponse(std::move(response))
ctx.send()

因此：

- Server 只有一个。
- Ep 线程有 N 个。
- Workers 线程有 M 个，并被所有 Ep 线程共享。
- 一个 client fd 从分配开始到关闭，始终只属于一个固定的 Ep 线程。
- Ctx 也只应由所属 Ep 线程修改；业务线程只处理 HttpRequest -> Response。

## 问题 1

• 对，先只解决一个问题。

你当前“一个 client fd 固定归属一个 Ep 线程”这个原则，基本已经满足：

Server accept(clientFd)
-> 轮询选择一个 EpWorker
-> EpWorker::listen(clientFd)
-> 只有该 EpWorker 的 epoll/kqueue 监听这个 fd
-> 只有该 EpWorker 的 cContexts/cReadStates 保存它

其他 Ep 线程不会读写这个 client fd。唯一不够纯粹的地方是 Server 线程直接调用了
EpWorker::listen() 并操作其 epoll/kqueue，但这暂时不改变“连接由哪个 Ep 线程处理”的事实。

当前最严重的调度问题

是：Ep 线程会被 Workers 的任务队列阻塞。

路径是：

Ep 线程
-> dispatchRequest()
-> Server::enqueueRequest()
-> cWorkers.post()
-> SafeQueue::push()
-> 队列满时 condition_variable::wait()

include/queue/queue.h:18 的 push() 在队列满时等待：

cCusumer.wait(lg, [this]() {
return cQueue.size() < cCap || cClose;
});

你的业务队列容量是 10。例如：

10 个业务任务在排队
业务线程全忙
第 11 个请求到达某个 Ep 线程
-> Ep 线程在 cWorkers.post() 里等待
-> 该 Ep 线程停止 reader / writer / close
-> 它负责的全部连接一起受影响

这违反了 Ep 线程的核心纪律：

> Ep 线程可以拒绝、限流、暂缓请求，但绝不能等待业务线程池腾出位置。

第一个优化目标应是增加：

bool tryPost(Handle&& handle);

其行为：

队列未满 -> 入队并返回 true
队列已满 -> 立即返回 false
绝不等待

Server::enqueueRequest() 发现返回 false 后，可先采用最简单策略：

返回 HTTP 503 Service Unavailable

后续再学习更复杂的背压：暂停该连接的读事件，等业务队列有空间后再继续读。

先把这个问题解决后，再处理第二个问题：同一连接在业务执行期间仍可继续 Read，可能重复分发请求。

# 待解决

1. EpWorker 生命周期错误
   include/io-worker.h:47 的线程 detach()，但 src/server.cpp:71 停止时没有停止或 join Ep 线程。Server 析构后，Ep 线程
   仍可能访问 Router&、Workers&，属于悬空引用。

2. 连接在业务执行期间仍可继续读请求
   include/io-worker.h:130 每次 Read 都可能创建新的 Ctx 并覆盖 include/io-worker.h:136 的 cContexts[fd]。一个客户端在
   前一个 handler 未完成前继续发送数据，会出现同 fd 的多个请求、响应顺序混乱。后续需要连接状态 Reading / Processing /
   Writing / Closing。
