#include "OverlayConfig.hpp"

#include <stdexcept>

void OverlayConfig::setSelfNodeId(const std::string& nodeId) {
    selfNodeId_ = nodeId;
}

const std::string& OverlayConfig::selfNodeId() const {
    return selfNodeId_;
}

void OverlayConfig::addNode(const OverlayNodeInfo& node) {
    nodes_[node.nodeId] = node;
}

bool OverlayConfig::hasNode(const std::string& nodeId) const {
    return nodes_.find(nodeId) != nodes_.end();
}

const OverlayNodeInfo& OverlayConfig::nodeInfo(const std::string& nodeId) const {
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        throw std::runtime_error("OverlayConfig missing node: " + nodeId);
    }
    return it->second;
}

std::string OverlayConfig::endpointFor(const std::string& nodeId) const {
    return nodeInfo(nodeId).endpoint();
}

std::vector<std::string> OverlayConfig::neighborNodeIds(const std::string& nodeId) const {
    return nodeInfo(nodeId).neighbors;
}

std::vector<std::string> OverlayConfig::queryForwardTargets(const std::string& nodeId) const {
    return neighborNodeIds(nodeId);
}