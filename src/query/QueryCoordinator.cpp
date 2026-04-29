#include "QueryCoordinator.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>

QueryCoordinator::QueryCoordinator(const std::string& selfNodeId,
                                   const PartitionStore& store,
                                   const OverlayConfig& overlay,
                                   const LocalQueryEngine& localEngine,
                                   std::shared_ptr<IRemoteQueryClient> remoteClient)
    : selfNodeId_(selfNodeId),
      store_(store),
      overlay_(overlay),
      localEngine_(localEngine),
      remoteClient_(std::move(remoteClient)) {}

std::string QueryCoordinator::submitClientQuery(const QueryRequest& request) {
    std::lock_guard<std::mutex> lock(mu_);

    RequestState st;
    st.requestId = makeRequestIdLocked();
    st.ownerNodeId = selfNodeId_;
    st.query = request;

    runLocalQueryLocked(st);
    submitChildrenLocked(st);
    st.localDone = st.localExhausted();
    st.complete = st.isFullyDone();
    st.touch();

    std::string requestId = st.requestId;
    requests_.emplace(requestId, std::move(st));
    return requestId;
}

std::string QueryCoordinator::submitSubQuery(const QueryRequest& request, const std::string& parentRequestId) {
    std::lock_guard<std::mutex> lock(mu_);

    RequestState st;
    st.requestId = makeRequestIdLocked();
    st.ownerNodeId = selfNodeId_;
    st.query = request;

    // local only for subquery on this node
    runLocalQueryLocked(st);
    st.localDone = st.localExhausted();
    st.complete = st.isFullyDone();
    st.touch();

    std::string requestId = st.requestId;
    requests_.emplace(requestId, std::move(st));
    (void)parentRequestId;
    return requestId;
}

ChunkFetchResult QueryCoordinator::fetchChunk(const std::string& requestId, size_t maxRows) {
    std::lock_guard<std::mutex> lock(mu_);

    ChunkFetchResult result;
    result.requestId = requestId;

    auto it = requests_.find(requestId);
    if (it == requests_.end()) {
        result.found = false;
        result.done = true;
        result.message = "request not found";
        return result;
    }

    RequestState& st = it->second;
    st.touch();

    if (st.cancelled) {
        result.found = true;
        result.done = true;
        result.message = "request cancelled";
        return result;
    }

    result.found = true;

    size_t budget = (maxRows == 0) ? 64 : maxRows;
    result.rows.reserve(budget);

    size_t added = 0;
    added += appendLocalRowsLocked(st, budget - added, result.rows);

    if (added < budget) {
        added += appendRemoteRowsLocked(st, budget - added, result.rows);
    }

    st.localDone = st.localExhausted();
    st.complete = st.isFullyDone();
    st.rowsServed += result.rows.size();
    st.chunksServed += 1;

    result.done = st.complete || st.cancelled;
    result.message = result.done ? "done" : "more";
    return result;
}

bool QueryCoordinator::cancel(const std::string& requestId, std::string& message) {
    std::lock_guard<std::mutex> lock(mu_);

    auto it = requests_.find(requestId);
    if (it == requests_.end()) {
        message = "request not found";
        return false;
    }

    RequestState& st = it->second;
    st.cancelled = true;
    st.complete = true;
    st.touch();

    if (remoteClient_) {
        for (auto& child : st.children) {
            if (child.accepted && !child.remoteRequestId.empty() && !child.done && !child.failed) {
                std::string childMsg;
                bool ok = remoteClient_->cancelSubQuery(child.nodeId, child.remoteRequestId, childMsg);
                if (!ok) {
                    child.failed = true;
                }
                child.lastMessage = childMsg;
                child.done = true;
            }
        }
    }

    message = "cancelled";
    return true;
}

bool QueryCoordinator::hasRequest(const std::string& requestId) const {
    std::lock_guard<std::mutex> lock(mu_);
    return requests_.find(requestId) != requests_.end();
}

std::optional<RequestState> QueryCoordinator::getRequestStateSnapshot(const std::string& requestId) const {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = requests_.find(requestId);
    if (it == requests_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::string QueryCoordinator::makeRequestIdLocked() {
    std::ostringstream oss;
    oss << selfNodeId_ << "-req-" << nextRequestSeq_++;
    return oss.str();
}

void QueryCoordinator::runLocalQueryLocked(RequestState& st) {
    st.localRowIds = localEngine_.execute(store_, st.query);
    st.localCursor = 0;
    st.localReady = true;
}

void QueryCoordinator::submitChildrenLocked(RequestState& st) {
    if (!remoteClient_) {
        return;
    }

    for (const auto& nodeId : nextHopNodeIds()) {
        ChildRequestState child;
        child.nodeId = nodeId;

        std::string remoteRequestId;
        std::string message;
        bool ok = remoteClient_->submitSubQuery(nodeId, st.query, st.requestId, remoteRequestId, message);

        child.accepted = ok;
        child.remoteRequestId = remoteRequestId;
        child.lastMessage = message;
        child.failed = !ok;
        child.done = !ok ? true : false;

        st.children.push_back(std::move(child));
    }
}

size_t QueryCoordinator::appendLocalRowsLocked(RequestState& st,
                                               size_t maxRows,
                                               std::vector<QueryResultRow>& out) {
    size_t added = 0;

    while (added < maxRows && st.localCursor < st.localRowIds.size()) {
        RowId rowId = st.localRowIds[st.localCursor++];
        out.push_back(materializeRow(rowId));
        ++added;
    }

    return added;
}

size_t QueryCoordinator::appendRemoteRowsLocked(RequestState& st,
                                                size_t maxRows,
                                                std::vector<QueryResultRow>& out) {
    if (!remoteClient_ || maxRows == 0) {
        return 0;
    }

    size_t added = 0;
    const size_t perChildBudget = std::max<size_t>(1, maxRows / std::max<size_t>(1, st.children.size()));

    for (auto& child : st.children) {
        if (added >= maxRows) {
            break;
        }
        if (!child.accepted || child.done || child.failed || child.remoteRequestId.empty()) {
            continue;
        }

        std::vector<QueryResultRow> rows;
        bool done = false;
        std::string message;

        size_t want = std::min(perChildBudget, maxRows - added);
        bool ok = remoteClient_->fetchSubChunk(child.nodeId, child.remoteRequestId, want, rows, done, message);

        if (!ok) {
            child.failed = true;
            child.done = true;
            child.lastMessage = message;
            continue;
        }

        child.lastMessage = message;
        child.done = done;

        for (auto& row : rows) {
            if (added >= maxRows) {
                break;
            }
            out.push_back(std::move(row));
            ++added;
        }
    }

    return added;
}

QueryResultRow QueryCoordinator::materializeRow(RowId rowId) const {
    QueryResultRow row{};
    row.rowId = rowId;
    row.sourceNodeId = selfNodeId_;

    // match PartitionStore API.
    row.vendorId = store_.vendorIdAt(rowId);
    row.pickupDatetime = store_.pickupDatetimeAt(rowId);
    row.dropoffDatetime = store_.dropoffDatetimeAt(rowId);
    row.passengerCount = store_.passengerCountAt(rowId);
    row.tripDistance = store_.tripDistanceAt(rowId);
    row.rateCodeId = store_.rateCodeIdAt(rowId);
    row.storeAndFwdFlag = store_.storeAndFwdFlagAt(rowId);
    row.puLocationId = store_.puLocationIdAt(rowId);
    row.doLocationId = store_.doLocationIdAt(rowId);
    row.paymentType = store_.paymentTypeAt(rowId);
    row.fareAmount = store_.fareAmountAt(rowId);
    row.extra = store_.extraAt(rowId);
    row.mtaTax = store_.mtaTaxAt(rowId);
    row.tipAmount = store_.tipAmountAt(rowId);
    row.tollsAmount = store_.tollsAmountAt(rowId);
    row.improvementSurcharge = store_.improvementSurchargeAt(rowId);
    row.totalAmount = store_.totalAmountAt(rowId);
    row.congestionSurcharge = store_.congestionSurchargeAt(rowId);

    return row;
}

std::vector<std::string> QueryCoordinator::nextHopNodeIds() const {
    // Adapt to OverlayConfig API.
    // For now, assume OverlayConfig can answer "outgoing neighbors for self"
    return overlay_.neighborNodeIds(selfNodeId_);
}