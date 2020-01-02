#include <chrono>
#include <iostream>

#include "control_node.h"

ControlNode::ControlNode(const std::vector<NodeIndex>& neighbours) : log("ControlNode") {
    for (const auto& n : neighbours) {
        lsaReceive[n] = new OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>(
                "DR_" + n + "_backward",
                boost::interprocess::create_only
        );

        lsaBroadcast[n] = new OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>(
                "DR_" + n + "_forward",
                boost::interprocess::create_only
        );
    }

    receiveOperationJob();
}

void ControlNode::receiveOperationJob() {
    // 1. Build network topology just as in lab #2

    Message serializedTopologyOperationMessage;

    Timer t(waitForLsaMs);

    while (!t.expired()) {
        for (auto& ch : lsaReceive) {
            if (ch.second->receive(serializedTopologyOperationMessage)) {
                if (serializedTopologyOperationMessage.messageType != MessageType::TOPOLOGY_OPERATION) {
                    continue;
                }

                const TopologyOperation op = serializedTopologyOperationMessage.getMessageContent<TopologyOperation>();

                if (knownTopology.applyOperation(op)) {
                    log << "Received topology update from " << ch.second->getMQName() << std::endl;
                    broadcastOperation(op);
                }
            }
        }
    }

    // knownTopology.saveToDot("built_topology.dot");

    // 2. Try to trigger focusing process

    log << "Topology researched, determining invocation order" << std::endl;

    invokeFirstWorker();
}

void ControlNode::broadcastOperation(const TopologyOperation& op) {
    Message message(MessageType::TOPOLOGY_OPERATION, op);

    for (auto& ch : lsaBroadcast) {
        ch.second->send(message);
    }
}

void ControlNode::invokeFirstWorker() {
    // We take the network graph, make sure, that its topology is just a line,
    // than find any node with one neighbour (obviously it is the last node or the first
    // node in this line) and invoke it for focusing.

    const Graph& networkGraph = knownTopology.getGraph();

    std::vector<NodeIndex> nodesWithOneNeighbour;

    for (const auto& nodeConnection : networkGraph) {
        const size_t numNeighbours = nodeConnection.second.size();

        if (numNeighbours == 0) {
            log << "Found node with zero neighbours, required topology is line, terminating" << std::endl;
            return;
        } else if (numNeighbours > 2) {
            log << "Found node with more than two neighbours, required topology is line, terminating" << std::endl;
            return;
        } else if (numNeighbours == 1) {
            nodesWithOneNeighbour.push_back(nodeConnection.first);
        }
    }

    if (nodesWithOneNeighbour.size() != 2) {
        log << "Required topology is line, terminating" << std::endl;
        return;
    }

    lsaBroadcast[nodesWithOneNeighbour[0]]->send({MessageType::INVOKE, Invoke{}});
}