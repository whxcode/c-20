#pragma once

#include <vector>

#include "collector.h"
#include "model.h"

class Document {
public:
    Document();
    ~Document();

public:
    Model* createModel(UniqueKey id, std::string name);
    Model* createModel(std::string name);

    void triggerUpdated();

    // 打开收集器功能
    void closedCollector();

    // 关闭收集器功能
    void openCollector();

    // 获取收集器内的值
    std::optional<PatchPair> commit();

    Model* getModel(UniqueKey index);

    void dumpModel();

    void addChild(Model* model);
    void removeChild(Model* model);

    bool canMergePatch(const std::vector<PatchItem>& patches);
    void mergePatch(const std::vector<PatchItem>& patches);

    UniqueKey getID() {
        return 10000;
    }

private:
    Collector* docCollectors{nullptr};
    mutable std::unordered_map<UniqueKey, Collector*> collectors{};
    mutable std::unordered_map<UniqueKey, Model*> modelMap{};
    // 已删除的图层
    mutable std::unordered_map<UniqueKey, Model*> removedPool{};

    size_t seedId{0};
};
