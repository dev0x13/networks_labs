#pragma once

#include "one_way_transducer.h"
#include "topology.h"

#include <thread>
#include <mutex>

// Common network router class
class CommonRouter {
public:
    CommonRouter(const std::string& routerID, const std::unordered_map<NodeIndex, Cost>& neighbours_);

    ~CommonRouter() {
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

    // Serializes a topology operation and send it to DR
    void sendOperationToDr(const TopologyOperation& op) {
        std::stringstream ss;
        op.serialize(ss);
        lsaSend.send(ss.str());
    }

    // Repetitive job, handles topology changes received from DR.
    void receiveOperationJob();
private:
    // Timeouts ans intervals
    const int64_t noNeighboursTimeoutMs = 5000;
    const int64_t waitForLsaMs = 2000;
    //const int64_t pingIntervalMs = 500;
    const int64_t pingTimeoutMs = 1000;

    // Router ID
    const NodeIndex id;

    // A piece of network topology, which router knows
    Topology knownTopology;

    // At every moment stores shortest paths to every
    // node from the known topology. In this demo it
    // is built, but not used after.
    Topology::ShortestPaths shortestPaths;

    // Stores router's neighbours
    std::unordered_map<NodeIndex, Cost> neighbours;

    // A flag for initial neighbours search (if no neighbours
    // were discovered router terminates)
    bool noNeighbours = false;

    // Communication channels with designated router to send and receive
    // Linked State Advertisement
    OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_only_t> lsaSend;
    OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_only_t> lsaReceive;

    // Stores every neighbour last seen timestamp to determine if neighbour died
    std::unordered_map<NodeIndex, std::chrono::time_point<std::chrono::system_clock>> lastSeenNeighbour;

    // Stores communication channels between router and its neighbours.
    // We store pointers, because it's hard to implement a proper copy
    // constructor for OneWayTransducer.
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::open_or_create_t>*> neighboursForward;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::open_or_create_t>*> neighboursBackward;

    // A mutex for r/w operations with topology
    std::mutex topologyLock;
};
