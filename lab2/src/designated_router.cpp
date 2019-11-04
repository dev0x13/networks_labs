#include <chrono>

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

    lsaReceiveJob();
}

void DesignatedRouter::lsaReceiveJob() {
    std::string serializedLsa;

    auto lastLsaReceivingTime = std::chrono::system_clock::now();

    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now() - lastLsaReceivingTime
           ).count() < waitForLsaMs)
    {
        for (auto& ch : lsaReceive) {
            if (ch->receive(serializedLsa)) {
                std::cout << "[DesignatedRouter] Received LSA from " << ch->getMQName() << std::endl;

                std::stringstream ss;
                ss << serializedLsa;
                const Topology &newTopology = Topology::deserialize(ss);

                lastLsaReceivingTime = std::chrono::system_clock::now();

                if (!(knownTopology == newTopology)) {
                    knownTopology.merge(newTopology);
                    broadcastLsa();
                }
            }
        }
    }

    knownTopology.saveToDot("topology.dot");
    std::cout << "[DesignatedRouter] No LSA received during " << waitForLsaMs << " ms, terminating" << std::endl;
}

void DesignatedRouter::broadcastLsa() {
    std::stringstream ss;
    knownTopology.serialize(ss);
    const std::string serializedLsa = ss.str();

    for (auto& ch : lsaBroadcast) {
        ch->send(serializedLsa);
    }
}