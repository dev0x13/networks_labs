#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "messages.h"

class FocusNode {
public:
    FocusNode(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours_, const Vector& coord) {};

    ~FocusNode() {}
};
