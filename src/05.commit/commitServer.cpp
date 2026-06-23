#pragma once

#include "include/05commit/commitServer.h"

#include "include/05commit/document.h"
#include "include/05commit/history.h"

CommitServer::CommitServer(Document* document)
    : document(document),
      //  history(new DoubleQueueHistory)
      history(new SingleQueueHistory) {
}

CommitServer::~CommitServer() {
    // delete (DoubleQueueHistory*)history;
    delete history;
}

void CommitServer::commit() {
    document->triggerUpdated();
    auto pair = document->commit();

    if (pair.has_value()) {
        history->push(std::move(pair.value()));
    }
}

void CommitServer::applyRemotePatch(const std::vector<PatchItem> redo) {
    // 关闭文档收集器
    document->closedCollector();
    mergePatch(redo);
    document->openCollector();
}

void CommitServer::undo() {
    auto patch = history->popUndo();

    if (patch.has_value()) {
        // 关闭文档收集器
        document->closedCollector();
        mergePatch(patch->undo);
        document->openCollector();
        history->pushRedo(std::move(patch.value()));
    }
}
void CommitServer::redo() {
    auto patch = history->popRedo();

    if (patch.has_value()) {
        // 关闭文档收集器
        document->closedCollector();
        mergePatch(patch->redo);
        document->openCollector();
        history->pushUndo(std::move(patch.value()));
    }
}
void CommitServer::dumpHistory() {
    history->dumpHistory();
}

void CommitServer::mergePatch(const std::vector<PatchItem> redo) {
    document->mergePatch(redo);
};
