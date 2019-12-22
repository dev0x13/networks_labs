#include <iostream>

#include "worker_node.h"

WorkerNode::WorkerNode(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours_, const Vector& coord_) :
    id(routerID),
    coord(coord_),
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

    std::thread(&WorkerNode::pongJob, this).detach();
    std::thread(&WorkerNode::pingJob, this).detach();
    std::thread(&WorkerNode::invocationJob, this).detach();
    receiveOperationJob();
}

void WorkerNode::receiveOperationJob() {
    Message message;

    auto lastLsaReceivingTime = std::chrono::system_clock::now();

    while ((
            std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now() - lastLsaReceivingTime
            ).count() < waitForLsaMs))
    {
        if (lsaReceive.receive(message)) {
            switch (message.messageType) {
                case MessageType::TOPOLOGY_OPERATION:
                    {
                        const TopologyOperation op = message.getMessageContent<TopologyOperation>();

                        std::unique_lock<std::mutex> lock(topologyLock);

                        if (knownTopology.applyOperation(op)) {
                            lastLsaReceivingTime = std::chrono::system_clock::now();
                            std::cout << "[WorkerNode " << id << "] Received topology update from DR" << std::endl;
                        }
                    }

                    break;
                case MessageType::INVOKE:
                    /// INVOKE
                    std::cout << "[WorkerNode " << id << "] INVOKE" << std::endl;

                    {
                        for (const auto& neighbour : neighboursForward) {
                            neighbour.second->send({ MessageType::INVOKE, Invoke{} });
                            break;
                        }
                    }

                    break;
                default:
                    continue;
            }
        }
    }
}

void WorkerNode::pingJob() {
    const Message ping(MessageType::PING, Ping{});

    for (;;) {
        for (auto& ch : neighboursForward) {
            ch.second->send(ping);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(pingIntervalMs));
    }
}

void WorkerNode::pongJob() {
    Message message;
    const Message pong(MessageType::PONG, Pong{});

    for (;;) {
        for (auto& ch : neighboursBackward) {
            if (ch.second->receive(message)) {
                switch (message.messageType) {
                    case MessageType::PING:
                        neighboursForward[ch.first]->send(pong);
                        break;
                    case MessageType::PONG:
                        {
                            std::unique_lock<std::mutex> lock(topologyLock);

                            neighbours[ch.first] = true;

                            const TopologyOperation op(id, ch.first, neighbours[ch.first]);

                            if (knownTopology.applyOperation(op)) {
                                std::cout << "[WorkerNode " << id << "] New neighbour discovered: `" << ch.first << "`"
                                          << std::endl;
                                sendOperationToDr(op);
                            }
                        }

                        break;
                    case MessageType::INVOKE:
                        /// INVOKE
                        std::cout << "[WorkerNode " << id << "] INVOKE" << std::endl;

                        {
                            NodeIndex nodeToInvoke = ch.first;

                            for (const auto& neighbours : neighboursForward) {
                                if (neighbours.first != nodeToInvoke) {
                                    nodeToInvoke = neighbours.first;
                                    break;
                                }
                            }

                            neighboursForward[nodeToInvoke]->send({ MessageType::INVOKE, Invoke{} });
                        }

                        break;
                    default:
                        continue;
                }
            }
        }
    }
}

void WorkerNode::invocationJob() {
    Message message;

    for (;;) {
        for (auto& ch : neighboursBackward) {
            if (ch.second->receive(message)) {
                switch (message.messageType) {
                    case MessageType::INVOKE:
                        /// INVOKE
                        std::cout << "[WorkerNode " << id << "] INVOKE" << std::endl;

                        {
                            NodeIndex nodeToInvoke = ch.first;

                            for (const auto& neighbours : neighboursForward) {
                                if (neighbours.first != nodeToInvoke) {
                                    nodeToInvoke = neighbours.first;
                                    break;
                                }
                            }

                            neighboursForward[nodeToInvoke]->send({ MessageType::INVOKE, Invoke{} });
                        }

                        break;
                    default:
                        continue;
                }
            }
        }
    }
}