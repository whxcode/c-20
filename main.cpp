#include <cstdio>
#include <format>
#include <thread>

#include "include/queue/queue.h"
#include "include/server.h"
#include "include/session.h"

using Handle = std::function<void()>;

int main(int argc, char* argv[]) {
    /*
      auto workers = new Workers();

      for (size_t i{0}; i < 6; ++i) {
          workers->post([]() {
              std::this_thread::sleep_for(std::chrono::seconds(10));
              std::printf("Handle %d\n", std::this_thread::get_id());
          });
      }
    */

    Server ser;

    // 静态文件
    ser.useStaticServer("/editor", Server::StaticHandle);

    ser.get("/list", [](sp<Ctx> ctx) -> ResponseData {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return {.context = nullptr, .body = "----Hello:" + ctx->request.path};
    });

    ser.get("/get", [](sp<Ctx> ctx) -> ResponseData {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return {.context = nullptr, .body = "----Hello:" + ctx->request.path};
    });

    ser.run();
    return 0;
}
