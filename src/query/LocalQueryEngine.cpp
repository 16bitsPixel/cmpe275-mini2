#include "LocalQueryEngine.hpp"

std::vector<LocalQueryEngine::RowId>
LocalQueryEngine::execute(const PartitionStore& store,
                          const QueryRequest& query) const {
    (void)store;
    (void)query;

    // Empty-store test mode: no local matches.
    return {};
}

/*
#include "LocalQueryEngine.hpp"

#include <cstddef>

std::vector<LocalQueryEngine::RowId>
LocalQueryEngine::execute(const PartitionStore& store,
                          const QueryRequest& query) const {
    std::vector<RowId> result;

    const std::size_t n = store.size();
    result.reserve(n);

    for (std::size_t i = 0; i < n; ++i) {

        // ---------- FILTERS ----------
        // NOTE: Right now store is empty, so this loop won't run.
        // This is just the correct structure for later.

        if (query.pickupRange) {
            auto v = store.pickupDatetimeAt(static_cast<RowId>(i));
            if (!(v >= query.pickupRange->lo && v < query.pickupRange->hi)) {
                continue;
            }
        }

        if (query.dropoffRange) {
            auto v = store.dropoffDatetimeAt(static_cast<RowId>(i));
            if (!(v >= query.dropoffRange->lo && v < query.dropoffRange->hi)) {
                continue;
            }
        }

        if (query.distanceRange) {
            auto v = store.tripDistanceAt(static_cast<RowId>(i));
            if (!(v >= query.distanceRange->lo && v <= query.distanceRange->hi)) {
                continue;
            }
        }

        if (query.totalCentsRange) {
            auto v = store.totalAmountAt(static_cast<RowId>(i));
            if (!(v >= query.totalCentsRange->lo && v <= query.totalCentsRange->hi)) {
                continue;
            }
        }

        if (query.tipCentsRange) {
            auto v = store.tipAmountAt(static_cast<RowId>(i));
            if (!(v >= query.tipCentsRange->lo && v <= query.tipCentsRange->hi)) {
                continue;
            }
        }

        if (query.paymentType) {
            auto v = store.paymentTypeAt(static_cast<RowId>(i));
            if (v != *query.paymentType) {
                continue;
            }
        }

        // ---------- MATCH ----------
        result.push_back(static_cast<RowId>(i));
    }

    return result;
}
*/