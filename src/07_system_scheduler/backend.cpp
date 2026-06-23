#pragma once
#include "include/07_system_scheduler/backend.h"

#include <future>
void Backend::submitExportImageTask(ExportImageTask&& task) {
    exportImageQueue.push(std::move(task));
}

std::vector<std::future<std::vector<ExportImageTask>>> Backend::downImageTask() {
    std::vector<std::thread> customeies{};
    std::vector<std::future<std::vector<ExportImageTask>>> result{};

    size_t constomerCount = 2;

    for (size_t i = 0; i < constomerCount; ++i) {
        std::shared_ptr<std::packaged_task<std::vector<ExportImageTask>()>> t =

            std::make_shared<std::packaged_task<std::vector<ExportImageTask>()>>([this]() {
                std::vector<ExportImageTask> r;

                while (auto v = this->getExportImageTask()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(300));
                    r.push_back(v.value());
                }

                return r;
            });

        result.push_back(t->get_future());

        customeies.push_back(std::thread([t]() {
            (*t)();
        }));
    }

    for (auto& p : customeies) {
        p.join();
    }

    return result;
}

std::optional<ExportImageTask> Backend::getExportImageTask() {
    return exportImageQueue.pop();
}
