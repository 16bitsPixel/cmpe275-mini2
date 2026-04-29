#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct NodeInfo {
    std::string nodeId;
    std::string host;
    uint32_t port = 0;

    std::vector<std::string> neighbors;

    std::string endpoint() const {
        return host + ":" + std::to_string(port);
    }
};