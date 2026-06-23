
#include "include/05commit/document.h"

#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include "include/05commit/collector.h"
#include "include/05commit/prop.h"
namespace {

size_t rank(const PatchType type) {
    switch (type) {
        case PatchType::zAddChild:
            return 0;
        case PatchType::zModifyProps:
            return 1;
        case PatchType::zRemoveChild:
            return 2;
    }
    return 99;
};

void sortPatches(std::vector<PatchItem>& patches) {
    std::sort(patches.begin(), patches.end(), [](const auto& a, const auto& b) {
        return rank(a.type) < rank(b.type);
    });
}

}  // namespace

Document::Document() {
    docCollectors = new Collector();
    collectors[getID()] = docCollectors;
    closedCollector();

    auto model1 = createModel("默认名称1");
    auto model2 = createModel("默认名称2");

    addChild(model1);
    addChild(model2);

    openCollector();
}

Document::~Document() {
    for (auto [_, model] : collectors) {
        delete model;
    }

    for (const auto [_, model] : modelMap) {
        delete model;
    }

    for (const auto [_, model] : removedPool) {
        delete model->getCollector();
        delete model;
    }
}

Model* Document::createModel(const std::string name) {
    return createModel(seedId++, name);
}

Model* Document::createModel(UniqueKey id, const std::string name) {
    auto collector = new Collector;
    auto model = new Model(id, collector);

    collector->close();
    model->setName(name);
    collector->open();

    return model;
}

void Document::triggerUpdated() {
    // printf("Document triggerUpdated\n");
}

void Document::closedCollector() {
    for (auto [_, collector] : collectors) {
        collector->close();
    }
}

void Document::openCollector() {
    for (auto [_, collector] : collectors) {
        collector->open();
    }
}

std::optional<PatchPair> Document::commit() {
    auto pair = std::make_optional<PatchPair>();
    bool empty{true};

    for (auto [_, collector] : collectors) {
        auto p = collector->commit();

        if (p.has_value()) {
            empty = false;

            pair->undo.insert(pair->undo.end(), p->undo.begin(), p->undo.end());
            pair->redo.insert(pair->redo.end(), p->redo.begin(), p->redo.end());
        }
    }

    sortPatches(pair->undo);
    sortPatches(pair->redo);

    return empty ? std::nullopt : pair;
}

Model* Document::getModel(UniqueKey id) {
    auto it = modelMap.find(id);

    return it == modelMap.end() ? nullptr : it->second;
}

void Document::dumpModel() {
    for (const auto& [_, model] : modelMap) {
        model->dump();
    }
}

void Document::addChild(Model* model) {
    modelMap[model->getID()] = model;
    collectors[model->getID()] = model->getCollector();

    removedPool.erase(model->getID());

    docCollectors->addChildren(model);
}

void Document::removeChild(Model* model) {
    const auto modelID = model->getID();

    modelMap.erase(modelID);
    collectors.erase(modelID);

    removedPool[modelID] = model;

    docCollectors->removeChildren(model);
}

bool Document::canMergePatch(const std::vector<PatchItem>& patches) {
    std::unordered_set<UniqueKey> alive;

    for (const auto& [id, _] : modelMap) {
        alive.insert(id);
    }

    for (const auto& item : patches) {
        switch (item.type) {
            case PatchType::zAddChild: {
                alive.insert(item.id);
                break;
            }

            case PatchType::zModifyProps: {
                if (!alive.contains(item.id)) {
                    return false;
                }
                break;
            }

            case PatchType::zRemoveChild: {
                if (!alive.contains(item.id)) {
                    return false;
                }
                alive.erase(item.id);
                break;
            }
        }
    }

    return true;
}

void Document::mergePatch(const std::vector<PatchItem>& patches) {
    if (!canMergePatch(patches)) {
        ASSERT(true, "存在严重问题.");
        return;
    }

    for (const auto& item : patches) {
        switch (item.type) {
            case PatchType::zModifyProps: {
                auto model = getModel(item.id);
                ASSERT(model != nullptr, "zModifyProps");

                model->setProps(item.props);

                break;
            }

            case PatchType::zAddChild: {
                auto it = removedPool.find(item.id);
                auto isExist = it != removedPool.end();
                auto model = isExist ? it->second : createModel(item.id, "");

                model->setProps(item.props);
                this->addChild(model);

                break;
            }

            case PatchType::zRemoveChild: {
                auto model = getModel(item.id);
                ASSERT(model != nullptr, "zRemoveChild");
                removeChild(model);

                break;
            }
        }
    }
}
