1、先测试最基本的客户端发起->服务端写入->客户端收到。

2、在 Server 中接入线程池。
workers.post([],{})

1. 客户端断开后，工作线程仍会向已关闭或复用的 fd 写响应。
   工作任务强持有 ctx，src/server.cpp:99；客户端断开时主线程会关闭 fd 并删除 ctxs，src/server.cpp:187。任务 10 秒后仍进入完成队列，主线程
   在 src/server.cpp:68 对旧 Ctx 调用 send()。
   风险：写入 EBADF，更严重时 fd 已被 OS 分配给新客户，响应可能发错连接。
   处理：Completion 使用 weak_ptr<Ctx> 或保存连接 generation；主线程处理 Completion 时确认它仍等于 ctxs[fd] 的当前对象。

2. Server 析构成员顺序会造成 Wake-after-free。
   成员按声明的反序析构：include/server.h:149 会先于 include/server.h:148 析构。随后 Workers 析构会 join()，正在执行的任务仍会调用：

   wake->notify();

   此时 wake 已销毁。
   处理：先停止并 join Workers，再关闭 Wake；可在 Server 的显式 stop() 中按此顺序完成，或调整成员声明顺序。

3. 线程池关闭时可能永久卡住。
   include/queue/queue.h:25 的等待谓词没有 cClose；队列满时 close() 唤醒生产者后，它会再次等待。
   include/queue/queue.h:41 同样如此：工作线程已在等待空队列时，close() 后会被唤醒再继续等待。
   结果：include/queue/queue.h:104 的 join() 可能永不返回。

4. ET 监听 socket 只 accept 一次，会漏连接。
   src/server.cpp:26 每次事件只调用一次 accept()，而监听 fd 也没有设为 nonblocking。ET 下应循环 accept() 到 EAGAIN；否则同时到来的连接会
   停在 accept 队列中，之后未必再收到事件。
   此外，src/tools/tcp.cpp:49 在检查 cfd < 0 前就调用 set_nonblocking(-1)。

5. HTTP 已声明 Connection: close，但响应完成后没有关闭客户端 fd。
   include/ctx.h:62 是 Connection: close，但 include/ctx.h:218 只取消 EPOLLOUT，不会 detach(cfd)。
   结果：连接、ctxs 中的 shared_ptr 和 socket 都会一直保留到客户端主动断开。后续同 fd 请求还不会替换旧 Ctx，因为 src/server.cpp:92 仅在不
   存在时插入。

资源与错误处理

1. 文件 fd 没有可靠的 RAII 管理。
   cFileFd 初始值仍是 0，include/ctx.h:265；完成时仅关闭 > 0 的 fd，include/ctx.h:224。若 open() 返回 0 会泄漏；关闭后也没有设为 -1，以后
   可能二次关闭被复用的 fd。
   更重要的是，首次 sendfile() 发生错误且 cClear == false 时，错误分支不会关闭文件 fd。include/ctx.h:190
   处理：使用 RAII fd 类型，或至少初始 -1，所有退出路径统一关闭并置回 -1。

2. 网络错误可能终止整个服务器。
   include/tools/file.hpp:80 除少数 errno 外直接 std::terminate()。例如静态文件不存在、header send() 失败，都可能杀掉进程。
   网络层错误应关闭当前连接，不应终止整个服务。send() 还应考虑 SIGPIPE，Linux 可使用 MSG_NOSIGNAL。

3. n == 0 处理不完整。
   header 分支的处理在 include/ctx.h:143，但 body 的 send/sendfile 后没有对应检查。并且 cErrorHanlde == nullptr 时只是打印、不返回，仍可
   能循环。
   应保证每次实际发送调用后：

   n > 0：更新 offset
   n == -1 / EINTR：重试
   n == -1 / EAGAIN：注册 EPOLLOUT
   n == 0 或其他错误：清理资源并关闭连接

4. epfd、监听 fd 和 Wake fd 的初始化/关闭不完整。
   src/server.cpp:18 只关闭 lfd，不关闭 epfd；若 run() 前析构，lfd{0} 会关闭标准输入。Wake 创建失败后仍可能将 -1 注册进 epoll。所有 fd 建
   议初始为 -1，创建失败后立即停止启动流程。

线程与协议边界

1. Reactor 线程会因任务队列满而阻塞。
    src/server.cpp:99 最终调用有界队列的阻塞 push()。队列容量仅 10，include/queue/queue.h:123，业务慢时 Reactor 会卡在投递任务，无法处理其
    他连接。
    应提供 tryPost()，队列满时返回 503、断开或限流，而不是阻塞 Reactor。

2. 工作线程仍访问了 Ctx。
    src/server.cpp:104 在工作线程调用 ctx->getRequestPath()。当前字段碰巧没有被修改，但应在投递前由 Reactor 复制 path、method 和必要 body
    数据；工作线程只操作独立任务输入。

3. 请求体与静态路径缺少边界和安全检查。
    Content-Length 直接 stoul() 并可导致无限制缓存，src/server.cpp:246。静态路径只做 lexically_normal()，没有确认结果仍在静态根目录内，
    src/server.cpp:304，存在 .. 路径穿越风险。

建议修复顺序

1. 增加 Server::stop()：停止接入，停止投递，关闭队列，join workers，最后关闭 Wake/epoll/fd。
2. 修复“断连任务完成”校验与响应完成后的连接关闭。
3. 修复 ET accept 循环和队列满不阻塞 Reactor。
4. 引入 fd RAII，收敛 Ctx::write() 的所有成功/错误清理路径。
5. 最后加请求体上限、静态路径根目录校验、超时和取消任务机制。
