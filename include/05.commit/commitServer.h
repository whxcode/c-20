#pragma once

#include <vector>

#include "collector.h"
#include "document.h"
#include "history.h"

class CommitServer {
public:
    void commit() {
        document->triggerUpdated();
        auto pair = document->commit();

        if (pair.has_value()) {
            history->pushUndo(std::move(pair.value()));
        }
    }

    void applyRemotePatch(const std::vector<PatchItem> redo) {
        // 关闭文档收集器
        document->closedCollector();
        // mergePatch(redo);
        document->openCollector();
    }

    void undo() {
        auto patch = history->popUndo();

        if (patch.has_value()) {
            // 关闭文档收集器
            document->closedCollector();
            mergePatch(patch->undo);
            document->openCollector();
            history->pushRedo(std::move(patch.value()));
        }
    }

    void redo() {
        auto patch = history->popRedo();

        if (patch.has_value()) {
            // 关闭文档收集器
            document->closedCollector();
            mergePatch(patch->redo);
            document->openCollector();
            history->pushUndo(std::move(patch.value()));
        }
    }

private:
    void mergePatch(const std::vector<PatchItem> redo) {
        printf("mergePatch[%zu]\n", redo.size());
    }

private:
    Document* document{nullptr};
    History* history{nullptr};
};
