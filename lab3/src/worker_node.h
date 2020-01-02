#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "messages.h"
#include "common.h"

#include <thread>
#include <mutex>

// Main worker node, focuses on focus using Sun coordinates
class WorkerNode : public BaseNode {
public:
    WorkerNode(const std::string& routerID,
               const std::unordered_map<NodeIndex, Cost>& neighbours_,
               const Vector& coord_,
               const std::string& sunNodeId,
               const std::string& focusNodeId);

    ~WorkerNode() {
        for (auto &ch : neighboursForward) {
            delete ch.second;
        }

        for (auto &ch : neighboursBackward) {
            delete ch.second;
        }

        log << "Terminated" << std::endl;
    }

    // "Pong" repetitive job, handles `ping` message to this router
    // and `pong` messages from neighbours workers
    void pongJob();

    // "Ping" repetitive job, sends `ping` message to this worker
    // neighbours
    void pingJob();

    // Serializes a topology operation and send it to control node
    void sendOperationToDr(const TopologyOperation& op) {
        lsaSend.send({ MessageType::TOPOLOGY_OPERATION, op });
    }

    // Repetitive job, handles network topology changes received from control node
    void receiveOperationJob();

    // Perform focusing process
    void focusJob();

private:
    // Worker ID
    const NodeIndex id;

    Logger log;

    const int64_t pingIntervalMs = 200;

    // Worker coordinates
    const Vector coord;

    // Mirror rotation step in radians
    const float rotationAngle = 0.1;

    // Mirror rotation direction
    int16_t rotationDirSign = 1;

    // Current mirror normal
    Vector normal;

    // A piece of network topology, which worker currenly knows
    Topology knownTopology;

    // Stores workers's neighbours
    std::unordered_map<NodeIndex, Cost> neighbours;

    // Communication channels with control node to send and receive
    // Linked State Advertisement
    OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_only_t> lsaSend;
    OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_only_t> lsaReceive;

    // Stores communication channels between worker and its neighbours.
    // We store pointers, because it's hard to implement a proper copy
    // constructor for OneWayTransducer.
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_or_create_t>*> neighboursForward;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_or_create_t>*> neighboursBackward;

    // Channels for interacting with the Sun
    OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_only_t> sunSend;
    OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_only_t> sunReceive;

    // Channels for interacting with the focus
    OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_only_t> focusSend;
    OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_only_t> focusReceive;

    // A mutex for r/w operations with topology
    std::mutex topologyLock;
};
