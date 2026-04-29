#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../dataset/PartitionStore.hpp"
#include "../model/OverlayConfig.hpp"
#include "../model/QueryRequest.hpp"
#include "../model/QueryResult.hpp"
#include "LocalQueryEngine.hpp"
#include "RequestState.hpp"

class IRemoteQueryClient {
public:
    virtual ~IRemoteQueryClient() = default;

    virtual bool submitSubQuery(const std::string& targetNodeId,
                                const QueryRequest& request,
                                const std::string& parentRequestId,
                                std::string& remoteRequestId,
                                std::string& message) = 0;

    virtual bool fetchSubChunk(const std::string& targetNodeId,
                               const std::string& remoteRequestId,
                               size_t maxRows,
                               std::vector<QueryResultRow>& rows,
                               bool& done,
                               std::string& message) = 0;

    virtual bool cancelSubQuery(const std::string& targetNodeId,
                                const std::string& remoteRequestId,
                                std::string& message) = 0;
};

struct ChunkFetchResult {
    bool found = false;
    bool done = false;
    std::string requestId;
    std::vector<QueryResultRow> rows;
    std::string message;
};

class QueryCoordinator {
public:
    using RowId = uint32_t;

    QueryCoordinator(const std::string& selfNodeId,
                     const PartitionStore& store,
                     const OverlayConfig& overlay,
                     const LocalQueryEngine& localEngine,
                     std::shared_ptr<IRemoteQueryClient> remoteClient);

    std::string submitClientQuery(const QueryRequest& request);
    std::string submitSubQuery(const QueryRequest& request, const std::string& parentRequestId);

    ChunkFetchResult fetchChunk(const std::string& requestId, size_t maxRows);
    bool cancel(const std::string& requestId, std::string& message);

    bool hasRequest(const std::string& requestId) const;
    std::optional<RequestState> getRequestStateSnapshot(const std::string& requestId) const;

private:
    std::string selfNodeId_;
    const PartitionStore& store_;
    const OverlayConfig& overlay_;
    const LocalQueryEngine& localEngine_;
    std::shared_ptr<IRemoteQueryClient> remoteClient_;

    mutable std::mutex mu_;
    std::unordered_map<std::string, RequestState> requests_;
    uint64_t nextRequestSeq_ = 1;

private:
    std::string makeRequestIdLocked();

    void runLocalQueryLocked(RequestState& st);
    void submitChildrenLocked(RequestState& st);

    size_t appendLocalRowsLocked(RequestState& st, size_t maxRows, std::vector<QueryResultRow>& out);
    size_t appendRemoteRowsLocked(RequestState& st, size_t maxRows, std::vector<QueryResultRow>& out);

    QueryResultRow materializeRow(RowId rowId) const;
    std::vector<std::string> nextHopNodeIds() const;
};