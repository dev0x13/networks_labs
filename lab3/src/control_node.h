#pragma once

#include "one_way_transducer.h"
#include "topology.h"

#include <vector>

// Designated network router class. Intended to synchronize
// LSA between common routers
class ControlNode {
public:
    explicit ControlNode(const std::vector<NodeIndex>& neighbours);

    ~ControlNode() {
        for (auto &ch : lsaReceive) {
            delete ch.second;
        }

        for (auto &ch : lsaBroadcast) {
            delete ch.second;
        }
    }

    // Handles topology changes received from common routers
    void receiveOperationJob();

    // Broadcast topology changes to common routers
    void broadcastOperation(const TopologyOperation& op);

    void invokeFirstWorker();

private:
    static const constexpr int64_t waitForLsaMs = 5000;

    // Stores known topology
    Topology knownTopology;

    // Stores communication channels between DR and all common routers.
    // We store pointers, because it's hard to implement a proper copy
    // constructor for OneWayTransducer.
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>*> lsaReceive;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>*> lsaBroadcast;
};