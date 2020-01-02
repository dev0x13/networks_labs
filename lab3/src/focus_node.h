#pragma once

#include "one_way_transducer.h"
#include "topology.h"
#include "messages.h"
#include "common.h"

// Class implementing focus
class FocusNode : public BaseNode {
public:
    FocusNode(const std::string& nodeID, const std::vector<NodeIndex>& workers, const Vector& coord_);

    // Receives messages from workers, checks if rays are focused
    // and gives a feedback
    void intensityChangeJob();

    ~FocusNode() {
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

    // Channels used for interaction with workers
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>*> workersForward;
    std::unordered_map<NodeIndex, OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>*> workersBackward;

    // Used for checking if the worker node is already focused
    std::unordered_map<NodeIndex, bool> workersStatus;

    size_t intensity = 0;
    const Vector coord;

    // Just focus radius or so
    static const constexpr float distanceThreshold = 0.1;
};
