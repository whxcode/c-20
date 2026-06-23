#pragma once
#include <any>
#include <functional>
#include <future>

#include "include/07_system_scheduler/queue.hpp"

// 图层 ID;
using ExportImageTask = size_t;  // std::function<void()>;

class Backend {
public:
    void submitExportImageTask(ExportImageTask&& task);

    std::vector<std::future<std::vector<ExportImageTask>>> downImageTask();
    std::optional<ExportImageTask> getExportImageTask();

private:
    SystemSafeQueue<ExportImageTask> exportImageQueue{10};
};
