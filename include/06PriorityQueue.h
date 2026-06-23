#pragma once
#include <cstdio>
#include <functional>
#include <optional>
#include <queue>

using TaskFunc = std::function<void()>;

class PriorityQueue {
private:
    using TaskID = size_t;
    using TaskVersion = size_t;
    using Priority = size_t;
    struct Task {
        TaskID id{0};
        TaskVersion version{0};
        Priority priority{0};
        bool operator<(const Task& other) const {
            return priority < other.priority;
        }
    };

public:
    TaskID enqueue(const Priority priority, const TaskFunc func) {
        auto id{seedID++};
        zFuncs[id] = func;
        zVersion[id] = (TaskVersion)0;

        zPq.push({.id = id, .version = 0, .priority = priority});

        return id;
    }
    bool update(TaskID id, Priority priority) {
        auto it = zVersion.find(id);

        if (it == zVersion.end()) {
            return false;
        }

        ++it->second;
        zPq.push({.id = id, .version = it->second, .priority = priority});
        return true;
    }

    std::optional<TaskFunc> dequeue() {
        while (!zPq.empty()) {
            auto top = std::move(zPq.top());

            zPq.pop();

            auto it = zVersion.find(top.id);

            if (it == zVersion.end()) {
                continue;
            }

            // 旧任务
            if (it->second != top.version) {
                printf("包含旧任务\n");
                continue;
            }

            auto func = zFuncs.find(top.id);
            if (func == zFuncs.end()) {
                zVersion.erase(top.id);
                continue;
            }

            auto f = std::move(func->second);

            zFuncs.erase(top.id);
            zVersion.erase(top.id);

            return f;
        }

        return std::nullopt;
    }

    void clearExpired() {
        while (zPq.empty()) {
            auto take = zPq.top();
            auto it = zVersion.find(take.id);

            if (it == zVersion.end() || it->second != take.version) {
                zPq.pop();
                continue;
            }
        }
    }

    bool empty() {
        return zPq.empty();
    }

private:
    TaskID seedID{0};
    std::unordered_map<TaskID, TaskFunc> zFuncs;
    std::unordered_map<TaskID, TaskVersion> zVersion;
    std::priority_queue<Task> zPq;
};
