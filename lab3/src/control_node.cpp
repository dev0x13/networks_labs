#include <chrono>
#include <iostream>

#include "control_node.h"

ControlNode::ControlNode(const std::vector<NodeIndex>& neighbours) {
    for (const auto& n : neighbours) {
        lsaReceive[n] = new OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>(
                "DR_" + n + "_receive",
                boost::interprocess::create_only
        );

        lsaBroadcast[n] = new OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>(
                "DR_" + n + "_send",
                boost::interprocess::create_only
        );
    }

    receiveOperationJob();
}

void ControlNode::receiveOperationJob() {
    Message serializedTopologyOperationMessage;

    auto lastLsaReceivingTime = std::chrono::system_clock::now();

    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now() - lastLsaReceivingTime
           ).count() < waitForLsaMs)
    {
        for (auto& ch : lsaReceive) {
            if (ch.second->receive(serializedTopologyOperationMessage)) {
                if (serializedTopologyOperationMessage.messageType != MessageType::TOPOLOGY_OPERATION) {
                    continue;
                }

                const TopologyOperation op = serializedTopologyOperationMessage.getMessageContent<TopologyOperation>();

                if (knownTopology.applyOperation(op)) {
                    lastLsaReceivingTime = std::chrono::system_clock::now();
                    std::cout << "[ControlNode] Received topology update from " << ch.second->getMQName() << std::endl;
                    broadcastOperation(op);
                }
            }
        }
    }

    knownTopology.saveToDot("built_topology.dot");
    std::cout << "[ControlNode] Topology researched, determining invocation order" << std::endl;

    invokeFirstWorker();
}

void ControlNode::broadcastOperation(const TopologyOperation& op) {
    Message message(MessageType::TOPOLOGY_OPERATION, op);

    for (auto& ch : lsaBroadcast) {
        ch.second->send(message);
    }
}

void ControlNode::invokeFirstWorker() {
    const Graph& networkGraph = knownTopology.getGraph();

    std::vector<NodeIndex> nodesWithOneNeighbour;

    for (const auto& nodeConnection : networkGraph) {
        const size_t numNeighbours = nodeConnection.second.size();

        if (numNeighbours == 0) {
            std::cout << "[ControlNode] Found node with zero neighbours, required topology is line, terminating" << std::endl;
            return;
        } else if (numNeighbours > 2) {
            std::cout << "[ControlNode] Found node with more than two neighbours, required topology is line, terminating" << std::endl;
            return;
        } else if (numNeighbours == 1) {
            nodesWithOneNeighbour.push_back(nodeConnection.first);
        }
    }

    if (nodesWithOneNeighbour.size() != 2) {
        std::cout << "[ControlNode] Required topology is line, terminating" << std::endl;
        return;
    }

    lsaBroadcast[nodesWithOneNeighbour[0]]->send({MessageType::INVOKE, Invoke{}});
}