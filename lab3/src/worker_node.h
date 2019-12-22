#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "messages.h"

#include <thread>
#include <mutex>

// Common network router class
class WorkerNode {
public:
    WorkerNode(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours_, const Vector& coord);

    ~WorkerNode() {
        for (auto &ch : neighboursForward) {
            delete ch.second;
        }

        for (auto &ch : neighboursBackward) {
            delete ch.second;
        }
    }

    // "Pong" repetitive job, handles `ping` message to this router
    // and `pong` messages from neighbours.
    void pongJob();

    // "Ping" repetitive job, sends `ping` message to this router
    // neighbours.
    void pingJob();

    // "Ping" repetitive job, sends `ping` message to this router
    // neighbours.
    void invocationJob();

    // Serializes a topology operation and send it to DR
    void sendOperationToDr(const TopologyOperation& op) {
        lsaSend.send({ MessageType::TOPOLOGY_OPERATION, op });
    }

    // Repetitive job, handles topology changes received from DR.
    void receiveOperationJob();
private:
    const int64_t waitForLsaMs = 30000;

    const int64_t pingIntervalMs = 200;

    // Router ID
    const NodeIndex id;

    const Vector coord;

    float angle;

    // A piece of network topology, which router knows
    Topology knownTopology;

    // Stores router's neighbours
    std::unordered_map<NodeIndex, Cost> neighbours;

    // Communication channels with designated router to send and receive
    // Linked State Advertisement
    OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_only_t> lsaSend;
    OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_only_t> lsaReceive;

    // Stores communication channels between router and its neighbours.
    // We store pointers, because it's hard to implement a proper copy
    // constructor for OneWayTransducer.
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_or_create_t>*> neighboursForward;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_or_create_t>*> neighboursBackward;

    // A mutex for r/w operations with topology
    std::mutex topologyLock;
};
