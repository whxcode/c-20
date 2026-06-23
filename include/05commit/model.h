#pragma once
#include <any>
#include <functional>
#include <string>

#include "collector.h"
#include "include/05commit/prop.h"

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

    void dump() {
        printf("Model:[%s][id=%zu,x=%d,y=%d]\n", zName.c_str(), id, zX, zY);
    }

    void setProp(PropKey key, std::any value) {
        switch (key) {
            case PropKey::zId:
                setID(std::any_cast<UniqueKey>(value));
                break;

            case PropKey::zName:
                setName(std::any_cast<std::string>(value));
                break;

            case PropKey::zX:
                setX(std::any_cast<int>(value));
                break;

            case PropKey::zY:
                setY(std::any_cast<int>(value));
                break;
        }
    }

    void setProps(const Props& props) {
        for (auto [key, value] : props) {
            setProp(key, value);
        }
    }

public:
    Props getProps() {
        Props props;
        props[PropKey::zId] = std::make_any<UniqueKey>(getID());
        props[PropKey::zName] = std::make_any<std::string>(getName());
        props[PropKey::zX] = std::make_any<int>(getX());
        props[PropKey::zY] = std::make_any<int>(getY());

        return props;
    }

    UniqueKey getID() {
        return id;
    }
    Collector* getCollector() {
        return collector;
    }

private:
    void setID(UniqueKey _id) {
        id = _id;
    }

private:
    UniqueKey id{};
    DEFINED_PROP(std::string, Name)
    DEFINED_PROP(int, X)
    DEFINED_PROP(int, Y)

private:
    Collector* collector{nullptr};
    Listener listener{nullptr};
};
