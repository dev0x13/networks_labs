#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "common.h"

#include <vector>

// Control node class. Can be think as designed router producing LSA.
// It also triggering the process of focusing.
class ControlNode : public BaseNode {
public:
    explicit ControlNode(const std::vector<NodeIndex>& neighbours);

    ~ControlNode() {
        for (auto &ch : lsaReceive) {
            delete ch.second;
        }

        for (auto &ch : lsaBroadcast) {
            delete ch.second;
        }

        log << "Terminated" << std::endl;
    }

    // Handles topology changes received from common routers
    void receiveOperationJob();

    // Broadcast topology changes to worker nodes
    void broadcastOperation(const TopologyOperation& op);

    // Validates network topology and invokes first worker
    void invokeFirstWorker();

private:
    Logger log;

    // Timeout for LSA building. If passed, than network topology
    // is considered built and focsuing process is started
    static const constexpr int64_t waitForLsaMs = 5000;

    // Stores known topology
    Topology knownTopology;

    // Stores communication channels between DR and all common routers.
    // We store pointers, because it's hard to implement a proper copy
    // constructor for OneWayTransducer.
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>*> lsaReceive;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>*> lsaBroadcast;
};