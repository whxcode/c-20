#include <cstdio>
#include <format>
#include <random>
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

    ser.get("/list", [](const HttpRequest&) {
        static constexpr std::array<std::string_view, 5> names{
            "Alice", "Bob", "Chen", "Diana", "Eric",
        };

        std::mt19937 generator{std::random_device{}()};
        std::uniform_int_distribution<int> age(18, 65);

        std::string json = "[";

        for (size_t index = 0; index < names.size(); ++index) {
            if (index != 0) {
                json += ",";
            }

            json += std::format(R"({{"name":"{}","age":{}}})", names[index], age(generator));
        }

        json += "]";

        Response response;
        response.cHeaders["Content-Type"] = "application/json; charset=utf-8";
        response.cBody = RawBody{std::move(json)};
        return response;
    });

    /*
    ser.get("/get", [](sp<Ctx> ctx) -> ITask {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return {.cTx = nullptr, .body = "----Hello:" + ctx->request.path};
    });
  */

    ser.run();
    return 0;
}
