#pragma once

#include <string>

struct IngestJob {
    std::string filePath;
    std::string assignedNodeId;

    bool valid() const {
        return !filePath.empty();
    }
};