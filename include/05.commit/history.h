#pragma once
#include <deque>

#include "collector.h"

class History {
public:
    void pushUndo(PatchPair&& patchPair) {
        if (undo.size() >= MAX_HISTORY_SIZE) {
            undo.pop_front();
        }
        undo.push_back(std::move(patchPair));
    }

    void pushRedo(PatchPair&& patchPair) {
        if (redo.size() >= MAX_HISTORY_SIZE) {
            redo.pop_front();
        }
        redo.push_back(std::move(patchPair));
    }

    std::optional<PatchPair> popUndo() {
        if (undo.empty()) {
            return std::nullopt;
        }

        auto patchPair = std::move(undo.back());
        undo.pop_back();
        return patchPair;
    }

    std::optional<PatchPair> popRedo() {
        if (redo.empty()) {
            return std::nullopt;
        }

        auto patchPair = std::move(redo.back());
        redo.pop_back();
        return patchPair;
    }

private:
    size_t MAX_HISTORY_SIZE{10};
    std::deque<PatchPair> undo{};
    std::deque<PatchPair> redo{};
};
