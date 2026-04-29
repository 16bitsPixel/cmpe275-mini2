#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../model/QueryRequest.hpp"

struct ChildRequestState {
    std::string nodeId;
    std::string remoteRequestId;
    bool accepted = false;
    bool done = false;
    bool failed = false;
    std::string lastMessage;
};

struct RequestState {
    using RowId = uint32_t;

    std::string requestId;
    std::string ownerNodeId;
    QueryRequest query;

    // local execution results
    std::vector<RowId> localRowIds;
    size_t localCursor = 0;

    // distributed child state
    std::vector<ChildRequestState> children;

    // lifecycle flags
    bool localReady = false;
    bool localDone = false;
    bool cancelled = false;
    bool complete = false;
    bool clientDetached = false;

    // counters
    size_t rowsServed = 0;
    size_t chunksServed = 0;

    // timestamps
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastAccessAt;

    RequestState()
        : createdAt(std::chrono::steady_clock::now()),
          lastAccessAt(std::chrono::steady_clock::now()) {}

    bool allChildrenDone() const {
        for (const auto& c : children) {
            if (c.accepted && !c.done && !c.failed) {
                return false;
            }
        }
        return true;
    }

    bool localExhausted() const {
        return localCursor >= localRowIds.size();
    }

    bool isFullyDone() const {
        return localDone && allChildrenDone();
    }

    void touch() {
        lastAccessAt = std::chrono::steady_clock::now();
    }
};