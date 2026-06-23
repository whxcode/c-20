#include "include/05commit/collector.h"

#include "include/05commit/model.h"

void Collector::close() {
    ++disabledDepth;
}
void Collector::open() {
    if (disabledDepth > 0) {
        --disabledDepth;
    }
}

bool Collector::canCollect() const {
    return enabled && disabledDepth == 0;
}

void Collector::addChildren(Model* model) {
    if (!canCollect()) {
        return;
    }
    auto& item = items[model->getID()];

    // 说明这个图层删除还没commit.就添加.此时应该清理
    if (item.redoPatchType == PatchType::zRemoveChild) {
        items.erase(model->getID());
        return;
    }

    auto props = model->getProps();

    item.id = model->getID();
    item.undoPatchType = PatchType::zRemoveChild;
    item.redoPatchType = PatchType::zAddChild;
    item.newProps = std::move(props);
}

void Collector::removeChildren(Model* model) {
    if (!canCollect()) {
        return;
    }

    auto& item = items[model->getID()];

    // 说明这个图层时添加/还没调用commit合并.此时应该清理
    if (item.redoPatchType == PatchType::zAddChild) {
        items.erase(model->getID());
        return;
    }

    auto modelCollector = model->getCollector();
    modelCollector->close();

    auto undoCommit = modelCollector->commit();

    // 先将未提交的还原。
    if (undoCommit.has_value()) {
        for (auto item : undoCommit->undo) {
            model->setProps(item.props);
        }
    }

    auto props = model->getProps();

    item.id = model->getID();
    item.undoPatchType = PatchType::zAddChild;
    item.redoPatchType = PatchType::zRemoveChild;
    item.oldProps = std::move(props);
}

std::optional<PatchPair> Collector::commit() {
    if (items.empty()) {
        return {};
    }

    PatchPair pair{};

    for (auto& [id, item] : items) {
        pair.undo.push_back({.id = id, .type = item.undoPatchType, .props = item.oldProps});
        pair.redo.push_back({.id = id, .type = item.redoPatchType, .props = item.newProps});
    }

    items.clear();

    return pair;
};
;
