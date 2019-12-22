#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "messages.h"

// Common network router class
class SunNode {
public:
    SunNode(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours_, const Vector& coord) {};

    ~SunNode() {}
};
