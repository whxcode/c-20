#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

class Model;
using UniqueKey = uint8_t;
enum class PropKey { zId, zName, zX, zY };

enum class PatchType { zModifyProps, zAddChild, zRemoveChild };
using Props = std::unordered_map<PropKey, std::any>;

struct PatchItem {
    UniqueKey id{0};
    PatchType type{PatchType::zModifyProps};
    Props props{};
};

struct PatchPair {
    std::vector<PatchItem> undo{};
    std::vector<PatchItem> redo{};
};

class Collector {
public:
    void close();

    void open();

    bool canCollect() const;

    template <class T>
    void dataChanged(UniqueKey id, PropKey key, const T& oldValue, const T& newValue) {
        if (!canCollect()) {
            return;
        }

        auto& item = items[id];
        item.id = id;
        item.undoPatchType = PatchType::zModifyProps;
        item.redoPatchType = PatchType::zModifyProps;

        if (!item.used.contains(key)) {
            item.oldProps[key] = oldValue;
            item.used.insert(key);
        }

        item.newProps[key] = newValue;
    }

    void addChildren(Model* model);

    void removeChildren(Model* model);

    std::optional<PatchPair> commit();

private:
    struct Item {
        UniqueKey id{0};
        PatchType undoPatchType{PatchType::zModifyProps};
        PatchType redoPatchType{PatchType::zModifyProps};
        std::set<PropKey> used{};
        Props oldProps{};
        Props newProps{};
    };

    bool enabled{true};
    int disabledDepth{0};
    std::unordered_map<UniqueKey, Item> items{};
};
