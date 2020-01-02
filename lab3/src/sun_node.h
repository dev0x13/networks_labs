#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "messages.h"
#include "common.h"

#include <thread>
#include <mutex>

// Common network router class
class SunNode : public BaseNode {
public:
    SunNode(
        const std::string& nodeID,
        const std::vector<NodeIndex>& workers,
        const Vector& initialCoord,
        const float movingSpeed_,
        const float absMovingBound_);

    // Moves Sun by changing its coordinates in given bounds
    void moveJob();

    // Handles ping messages and returns its coordinates
    void pongJob();

    ~SunNode() {
        for (auto &ch : workersForward) {
            delete ch.second;
        }

        for (auto &ch : workersBackward) {
            delete ch.second;
        }

        log << "Terminated" << std::endl;
    }

private:
    Logger log;

    // Channels for interaction with workers
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>*> workersForward;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>*> workersBackward;

    Vector coord;

    // Time step for moving
    const int64_t coordUpdateTimeMs = 100;

    // Space step for moving
    const float movingSpeed;

    // Absolute bound for moving, i.e. if initial coord is (1; 0) and absolute bound is 5,
    // than bounds are (5;0) and (-5;0)
    const float absMovingBound;

    // Used for changing moving direction when bound is reached
    int16_t movingDirectionSign = 1;

    // Mutex for changing coordinates
    std::mutex movingMutex;
};
