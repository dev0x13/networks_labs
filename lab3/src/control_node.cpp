#include <chrono>
#include <iostream>

#include "designated_router.h"

DesignatedRouter::DesignatedRouter(const std::vector<NodeIndex>& neighbours) {
    for (const auto& n : neighbours) {
        auto receiveCh =
            new OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>(
                    "DR_" + n + "_receive",
                    boost::interprocess::create_only
            );
        lsaReceive.push_back(receiveCh);

        auto sendCh =
            new OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>(
                "DR_" + n + "_send",
                boost::interprocess::create_only
            );
        lsaBroadcast.push_back(sendCh);
    }

    receiveOperationJob();
}

void DesignatedRouter::receiveOperationJob() {
    std::string serializedTopologyOperation;

    auto lastLsaReceivingTime = std::chrono::system_clock::now();

    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now() - lastLsaReceivingTime
           ).count() < waitForLsaMs)
    {
        for (auto& ch : lsaReceive) {
            if (ch->receive(serializedTopologyOperation)) {
                std::stringstream ss;
                ss << serializedTopologyOperation;
                const TopologyOperation op = TopologyOperation::deserialize(ss);

                if (knownTopology.applyOperation(op)) {
                    lastLsaReceivingTime = std::chrono::system_clock::now();
                    std::cout << "[ControlNode] Received topology update from " << ch->getMQName() << std::endl;
                    broadcastOperation(op);
                }
            }
        }
    }

    knownTopology.saveToDot("topology.dot");
    std::cout << "[ControlNode] No topology updates during " << waitForLsaMs << " ms, terminating" << std::endl;
}

void DesignatedRouter::broadcastOperation(const TopologyOperation& op) {
    std::stringstream ss;
    op.serialize(ss);
    const std::string serializedOp = ss.str();

    for (auto& ch : lsaBroadcast) {
        ch->send(serializedOp);
    }
}