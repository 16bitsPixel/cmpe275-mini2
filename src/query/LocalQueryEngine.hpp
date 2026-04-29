#pragma once

#include <cstdint>
#include <vector>

#include "../dataset/PartitionStore.hpp"
#include "../model/QueryRequest.hpp"

class LocalQueryEngine {
public:
    using RowId = uint32_t;

    std::vector<RowId> execute(const PartitionStore& store,
                               const QueryRequest& query) const;
};