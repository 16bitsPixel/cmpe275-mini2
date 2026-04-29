#pragma once

#include <cstdint>
#include <optional>
#include <string>

template <typename T>
struct Range {
    T lo;
    T hi;
};

struct QueryRequest {
    std::optional<Range<int64_t>> pickupRange;
    std::optional<Range<int64_t>> dropoffRange;
    std::optional<Range<float>> distanceRange;
    std::optional<Range<int32_t>> totalCentsRange;
    std::optional<Range<int32_t>> tipCentsRange;
    std::optional<int32_t> paymentType;
    uint32_t preferredChunkSize = 64;
    std::string clientTag;
};