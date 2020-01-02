#include <iostream>

#include "worker_node.h"

WorkerNode::WorkerNode(
    const std::string& routerID,
    const std::unordered_map<NodeIndex, Cost>& neighbours_,
    const Vector& coord_,
    const std::string& sunNodeId,
    const std::string& focusNodeId)
    :
    id(routerID),
    log("WorkerNode " + id),
    coord(coord_),
    normal(0, 1),
    neighbours(neighbours_),
    lsaSend("DR_" + id + "_backward", boost::interprocess::open_only),
    lsaReceive("DR_" + id + "_forward", boost::interprocess::open_only),
    sunSend(sunNodeId + "_" + id + "_backward", boost::interprocess::open_only),
    sunReceive(sunNodeId + "_" + id + "_forward", boost::interprocess::open_only),
    focusSend(focusNodeId + "_" + id + "_backward", boost::interprocess::open_only),
    focusReceive(focusNodeId + "_" + id + "_forward", boost::interprocess::open_only)
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
    receiveOperationJob();
}

void WorkerNode::receiveOperationJob() {
    Message message;

    while (!lifeTimer.expired()) {
        if (lsaReceive.receive(message)) {
            switch (message.messageType) {
                case MessageType::TOPOLOGY_OPERATION:
                    {
                        const TopologyOperation op = message.getMessageContent<TopologyOperation>();

                        std::unique_lock<std::mutex> lock(topologyLock);

                        if (knownTopology.applyOperation(op)) {
                            log << "Received topology update from control node" << std::endl;
                        }
                    }

                    break;
                case MessageType::INVOKE:
                    log << "Invoked by control node" << std::endl;

                    focusJob();

                    for (const auto& neighbour : neighboursForward) {
                        // log << "Invoking neighbour `" << neighbour.first << "`" << std::endl;
                        neighbour.second->send({ MessageType::INVOKE, Invoke{} });
                        break;
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

    while (!lifeTimer.expired()) {
        for (auto& ch : neighboursForward) {
            ch.second->send(ping);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(pingIntervalMs));
    }
}

void WorkerNode::pongJob() {
    Message message;
    const Message pong(MessageType::PONG, Pong{});

    while (!lifeTimer.expired()) {
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
                                log << "New neighbour discovered: `" << ch.first << "`" << std::endl;
                                sendOperationToDr(op);
                            }
                        }

                        break;
                    case MessageType::INVOKE:
                        log << "Invoked by neighbour `" << ch.first << "`" << std::endl;

                        // Focus mirror
                        focusJob();

                        // Invoke the next worker
                        {
                            NodeIndex nodeToInvoke = ch.first;

                            // After this loop nodeToInvoke may be or next neighbour,
                            // or the previous invoker if there are no neighbours except this one
                            for (const auto& neighbours_ : neighboursForward) {
                                if (neighbours_.first != nodeToInvoke) {
                                    nodeToInvoke = neighbours_.first;
                                    break;
                                }
                            }

                            // log << "Invoking neighbour `" << nodeToInvoke << "`" << std::endl;
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

void WorkerNode::focusJob() {
    Message message;

    size_t prevIntensity = 0;
    size_t intensity = 0;
    size_t cnt = 0;

    // 1. Receive initial intensity from focus

    focusSend.send({MessageType::PING, Ping{}});

    while (!lifeTimer.expired()) {
        if (focusReceive.receive(message)) {
            if (message.messageType == MessageType::INTENSITY) {
                prevIntensity = message.getMessageContent<Intensity>().intensity;
                break;
            }
        }
    }

    // 2. Try to focus workers mirror

    while (!lifeTimer.expired()) {
        ++cnt;

        // 2.1. Receive current Sun coordinates

        sunSend.send({MessageType::PING, Ping{}});

        Vector sunCoord(0, 0);

        while (!lifeTimer.expired()) {
            if (sunReceive.receive(message)) {
                if (message.messageType == MessageType::VECTOR) {
                    sunCoord = message.getMessageContent<Vector>();
                    break;
                }
            }
        }

        // 2.2. Send reflected ray to the focus node

        const Vector ray(sunCoord.x - coord.x, sunCoord.y - coord.y);

        focusSend.send({
            MessageType::VECTOR_AND_POINT,
            VectorAndPoint{Vector::reflect(ray, normal), coord}
        });

        // 2.3. Receive new intensity value

        while (!lifeTimer.expired()) {
            if (focusReceive.receive(message)) {
                if (message.messageType == MessageType::INTENSITY) {
                    intensity = message.getMessageContent<Intensity>().intensity;
                    break;
                }
            }
        }

        // 2.4. Check if it increased

        if (intensity > prevIntensity) {
            // If yes, stop rotation, make a log record and transfer control flow to the next worker
            log << "Focused for Sun coord (" << sunCoord.x << ", " << sunCoord.y << ") "
                << "after " << cnt << " rotations"
                << std::endl;
            break;
        } else {
            // If no, rotate the mirror and try again
            if (normal.y < 0) {
                rotationDirSign = -rotationDirSign;
            }

            normal.rotate(rotationAngle * rotationDirSign);
            prevIntensity = intensity;
        }
    }
}
