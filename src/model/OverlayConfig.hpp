#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct OverlayNodeInfo {
    std::string nodeId;
    std::string host;
    uint32_t port = 0;
    std::vector<std::string> neighbors;

    std::string endpoint() const {
        return host + ":" + std::to_string(port);
    }
};

class OverlayConfig {
public:
    OverlayConfig() = default;

    void setSelfNodeId(const std::string& nodeId);
    const std::string& selfNodeId() const;

    void addNode(const OverlayNodeInfo& node);
    bool hasNode(const std::string& nodeId) const;

    const OverlayNodeInfo& nodeInfo(const std::string& nodeId) const;
    std::string endpointFor(const std::string& nodeId) const;

    std::vector<std::string> neighborNodeIds(const std::string& nodeId) const;

    // Optional alias if later want query routing to differ from generic neighbors
    std::vector<std::string> queryForwardTargets(const std::string& nodeId) const;

private:
    std::string selfNodeId_;
    std::unordered_map<std::string, OverlayNodeInfo> nodes_;
};