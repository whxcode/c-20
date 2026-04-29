#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

using UniqueKey = uint8_t;
enum class PropKey { Name, X, Y };

enum class PatchType { Update, Create, Delete };

struct PatchItem {
    UniqueKey id{0};
    PatchType type{PatchType::Update};
    std::unordered_map<PropKey, std::any> props{};
};

struct PatchPair {
    std::vector<PatchItem> undo{};
    std::vector<PatchItem> redo{};
};

class Collector {
public:
    void close() {
        ++disabledDepth;
    }
    void open() {
        if (disabledDepth > 0) {
            --disabledDepth;
        }
    }
    bool canCollect() const {
        return enabled && disabledDepth == 0;
    }

    template <class T>
    void dataChanged(UniqueKey id, PropKey key, const T& oldValue, const T& newValue) {
        if (!canCollect()) {
            return;
        }

        auto& item = items[id];
        item.id = id;

        if (!item.used.contains(key)) {
            item.oldProps[key] = oldValue;
            item.used.insert(key);
        }

        item.newProps[key] = newValue;
    }

    std::optional<PatchPair> commit() {
        if (items.empty()) {
            return {};
        }

        PatchPair pair{};

        for (auto& [id, item] : items) {
            pair.undo.push_back({.id = id, .type = PatchType::Update, .props = item.oldProps});
            pair.redo.push_back({.id = id, .type = PatchType::Update, .props = item.newProps});
        }

        items.clear();

        return pair;
    };

private:
    struct Item {
        int id = 0;
        std::set<PropKey> used{};
        std::unordered_map<PropKey, std::any> oldProps{};
        std::unordered_map<PropKey, std::any> newProps{};
    };

    bool enabled{true};
    int disabledDepth{0};
    std::unordered_map<UniqueKey, Item> items{};
};
