#pragma once
#include <functional>
#include <string>

#include "collector.h"
using Listener = std::function<void(PropKey)>;

class Model {
public:
    explicit Model(UniqueKey id, Collector* collector) : id(id), collector(collector) {
    }

    template <class T>
    void triggerChanged(PropKey key, const T& oldValue, const T& newValue) {
        if (listener) {
            listener(key);
        };
        if (collector) {
            collector->dataChanged(id, key, oldValue, newValue);
        }
    }

    void setName(const std::string& newName) {
        triggerChanged(PropKey::Name, name, newName);
        name = newName;
    }

    const std::string& getName() const {
        return name;
    }

private:
    const UniqueKey id{};

    std::string name{};
    int x{0};
    int y{0};
    Collector* collector{nullptr};
    Listener listener{nullptr};
};
