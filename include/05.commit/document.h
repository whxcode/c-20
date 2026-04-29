#pragma once

#include <vector>

#include "collector.h"
#include "model.h"
class Document {
public:
    void createModel(std::string name) {
        auto collector = new Collector;
        auto model = new Model(models.size() + 1, collector);

        collector->close();

        model->triggerChanged(PropKey::Name, std::string(""), name);

        collectors.push_back(collector);
        models.push_back(model);
    }

    void triggerUpdated() {
        printf("Document triggerUpdated\n");
    }

    void closedCollector() {
        for (auto collector : collectors) {
            collector->close();
        }
    }

    void openCollector() {
        for (auto collector : collectors) {
            collector->open();
        }
    }

    std::optional<PatchPair> commit() {
        auto pair = std::make_optional<PatchPair>();

        for (auto collector : collectors) {
            auto p = collector->commit();
            if (p.has_value()) {
                pair.undo.insert(pair.undo.end(), p->undo.begin(), p->undo.end());
                pair.redo.insert(pair.redo.end(), p->redo.begin(), p->redo.end());
            }
        }

        return pair;
    }

    Model* getModel(size_t index) {
        if (index < models.size()) {
            return models[index];
        }

        return nullptr;
    }

private:
    std::vector<Collector*> collectors{};
    std::vector<Model*> models{};
};
