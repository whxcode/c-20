#pragma once

#include <ratio>
#include <vector>

#include "collector.h"
#include "document.h"
#include "history.h"

class CommitServer {
public:
    CommitServer(Document* document);
    ~CommitServer();

public:
    void commit();

    void applyRemotePatch(const std::vector<PatchItem> redo);

    void undo();

    void redo();

    void dumpHistory();

private:
    void mergePatch(const std::vector<PatchItem> redo);

private:
    Document* document{nullptr};
    IHistory* history{nullptr};
};
