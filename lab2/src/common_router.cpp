#include "common_router.h"

CommonRouter::CommonRouter(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours_) :
    id(routerID),
    neighbours(neighbours_),
    lsaSend("DR_" + id + "_receive", boost::interprocess::open_only),
    lsaReceive("DR_" + id + "_send", boost::interprocess::open_only)
{
    // 1. Establish connections with neighbours and designated router

    for (const auto& n : neighbours) {
        // We build communication channels ID with lexicographical comparisons
        // of nodes IDs

        std::string forwardChannelId;
        std::string backwardChannelId;

        if (id < n.first) {
            const std::string dedicatedChannelId = id + "_" + n.first;
            forwardChannelId = dedicatedChannelId + "_forward";
            backwardChannelId = dedicatedChannelId + "_backward";
        } else {
            const std::string dedicatedChannelId = n.first + "_" + id;
            forwardChannelId = dedicatedChannelId + "_backward";
            backwardChannelId = dedicatedChannelId + "_forward";
        }

        neighboursForward[n.first] =
                new OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_or_create_t>
                        (forwardChannelId, boost::interprocess::open_or_create);
        neighboursBackward[n.first] =
                new OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_or_create_t>
                        (backwardChannelId, boost::interprocess::open_or_create);
    }

    // 2. Init topology

    knownTopology.addConnection(id, id, 0);

    // 3. Run jobs

    std::thread(&CommonRouter::pongJob, this).detach();
    std::thread(&CommonRouter::pingJob, this).detach();
    receiveLsaFromDrJob();
}

void CommonRouter::receiveLsaFromDrJob() {
    std::string serializedLsa;

    auto lastLsaReceivingTime = std::chrono::system_clock::now();

    while (!noNeighbours && (
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - lastLsaReceivingTime
            ).count() < waitForLsaMs))
    {
        if (lsaReceive.receive(serializedLsa)) {
            std::stringstream ss;
            ss << serializedLsa;
            const Topology& newTopology = Topology::deserialize(ss);

            std::cout << "[CommonRouter " << id << "] Received LSA from DR" << std::endl;

            lastLsaReceivingTime = std::chrono::system_clock::now();

            if (!(knownTopology == newTopology)) {
                std::unique_lock<std::mutex> lock(topologyLock);

                knownTopology.merge(newTopology);
                shortestPaths = knownTopology.getShortestPaths(id);
                std::cout << "[CommonRouter " << id << "] Shortest paths found" << std::endl;
            }
        }
    }

    if (noNeighbours) {
        std::cout << "[CommonRouter " << id << "] No neighbours discovered during " << noNeighboursTimeoutMs << " ms, terminating"
                  << std::endl;
    } else {
        std::cout << "[CommonRouter " << id << "] No LSA received during " << waitForLsaMs << " ms, terminating"
                  << std::endl;
    }
}

void CommonRouter::pingJob() {
    for (;;) {
        for (auto& ch : neighboursForward) {
            ch.second->send("ping");

            if (knownTopology.isConnected(id, ch.first)) {
                std::unique_lock<std::mutex> lock(topologyLock);

                if (std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now() - lastSeenNeighbour[ch.first]
                ).count() > pingTimeoutMs)
                {
                    std::cout << "[CommonRouter " << id << "] Neighbour `" << ch.first << "` is dead" << std::endl;
                    knownTopology.removeConnection(id, ch.first);
                }
            }
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(pingIntervalMs));
    }
}

void CommonRouter::pongJob() {
    auto lastNeighbourTime = std::chrono::system_clock::now();

    for (;;) {
        std::string message;

        for (auto& ch : neighboursBackward) {
            if (ch.second->receive(message)) {
                if (message == "ping") {
                    //std::cout << "[CommonRouter] Ping from " << ch.first << std::endl;
                    neighboursForward[ch.first]->send("pong");
                } else if (message == "pong") {
                    lastNeighbourTime = std::chrono::system_clock::now();
                    //std::cout << "[CommonRouter] Pong from " << ch.first << std::endl;
                    std::unique_lock<std::mutex> lock(topologyLock);

                    lastSeenNeighbour[ch.first] = std::chrono::system_clock::now();
                    if (knownTopology.addConnection(id, ch.first, neighbours[ch.first])) {
                        std::cout << "[CommonRouter " << id << "] New neighbour discovered: `" << ch.first << "`" << std::endl;
                        sendLsaToDr();
                    }
                }
            }
        }

        // This is the case, when router has discovered no neighbours,
        // so there is no reason to function anymore
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - lastNeighbourTime
        ).count() > noNeighboursTimeoutMs)
        {
            noNeighbours = true;
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(pingIntervalMs));
    }
}