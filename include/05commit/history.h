#pragma once
#include <cstdio>
#include <deque>

#include "collector.h"

class IHistory {
public:
    virtual void push(PatchPair&& patchPair) = 0;
    virtual void pushUndo(PatchPair&& patchPair) = 0;
    virtual void pushRedo(PatchPair&& patchPair) = 0;
    virtual std::optional<PatchPair> popUndo() = 0;
    virtual std::optional<PatchPair> popRedo() = 0;
    virtual void dumpHistory() = 0;
    virtual ~IHistory() = default;
};

class DoubleQueueHistory : public IHistory {
public:
public:
    void push(PatchPair&& patchPair) override {
        if (undo.size() >= MAX_HISTORY_SIZE) {
            undo.pop_front();
        }

        undo.push_back(std::move(patchPair));
        redo.clear();
    }

    void pushUndo(PatchPair&& patchPair) override {
        if (undo.size() >= MAX_HISTORY_SIZE) {
            undo.pop_front();
        }

        undo.push_back(std::move(patchPair));
        redo.clear();
    }

    void pushRedo(PatchPair&& patchPair) override {
        if (redo.size() >= MAX_HISTORY_SIZE) {
            redo.pop_front();
        }
        redo.push_back(std::move(patchPair));
    }

    std::optional<PatchPair> popUndo() override {
        if (undo.empty()) {
            return std::nullopt;
        }

        auto patchPair = std::move(undo.back());
        undo.pop_back();
        return patchPair;
    }

    std::optional<PatchPair> popRedo() override {
        if (redo.empty()) {
            return std::nullopt;
        }

        auto patchPair = std::move(redo.back());
        redo.pop_back();
        return patchPair;
    }

    void dumpHistory() override {
        printf("undo history size: %zu\n", undo.size());
        printf("redo history size: %zu\n", redo.size());
    }

private:
    size_t MAX_HISTORY_SIZE{10};
    std::deque<PatchPair> undo{};
    std::deque<PatchPair> redo{};
};

class SingleQueueHistory : public IHistory {
public:
public:
    void push(PatchPair&& patchPair) override {
        // 说明已经 redo 过，重新记录undo 需要清理之后的redo
        while (records.size() > cursor) {
            records.pop_back();
        }

        // 已经到达最大历史记录，丢弃之前的
        while (records.size() >= MAX_HISTORY_SIZE) {
            records.pop_front();

            if (cursor > 0) {
                --cursor;
            }
        }

        records.push_back(patchPair);
        cursor = records.size();
    }

    void pushUndo(PatchPair&& patchPair) override {
    }

    void pushRedo(PatchPair&& patchPair) override {
        // ++cursor;
        /*
        if (redo.size() >= MAX_HISTORY_SIZE) {
            redo.pop_front();
        }
        redo.push_back(std::move(patchPair));
    */
    }

    std::optional<PatchPair> popUndo() override {
        if (records.empty() || cursor <= 0) {
            return std::nullopt;
        }

        auto& patchPair = records[cursor - 1];
        --cursor;

        return patchPair;
    }

    std::optional<PatchPair> popRedo() override {
        if (records.empty() || cursor >= records.size()) {
            return std::nullopt;
        }

        auto& patchPair = records[cursor];
        cursor++;
        return patchPair;
    }

    void dumpHistory() override {
        printf("records(%d),cursor(%d) \n", records.size(), cursor);
    }

private:
    size_t MAX_HISTORY_SIZE{10};
    size_t cursor{0};
    std::deque<PatchPair> records{};
};
