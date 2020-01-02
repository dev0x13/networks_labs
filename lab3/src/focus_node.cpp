#include "focus_node.h"

FocusNode::FocusNode(const std::string& nodeID, const std::vector<NodeIndex>& workers, const Vector& coord_) :
    log("FocusNode"),
    coord(coord_)
{
    for (const auto& n : workers) {
        workersBackward[n] = new OneWayTransducer<TransducerMode::RECEIVING, boost::interprocess::create_only_t>(
                nodeID + "_" + n + "_backward",
                boost::interprocess::create_only
        );

        workersForward[n] = new OneWayTransducer<TransducerMode::SENDING, boost::interprocess::create_only_t>(
                nodeID + "_" + n + "_forward",
                boost::interprocess::create_only
        );

        workersStatus[n] = false;
    }

    intensityChangeJob();
};

void FocusNode::intensityChangeJob() {
    Message message;

    while (!lifeTimer.expired()) {
        for (const auto& ch : workersBackward) {
            if (ch.second->receive(message)) {
                switch (message.messageType) {
                    case MessageType::VECTOR_AND_POINT:
                    {
                        // Vector and point were received from worker:
                        // vector is reflected ray direction and point is workers coordinates

                        const VectorAndPoint vectorAndPoint = message.getMessageContent<VectorAndPoint>();

                        const Vector& reflectedRay = vectorAndPoint.vector;
                        const Vector& workerCoord  = vectorAndPoint.point;

                        // We compute distance from the line derived from given vector and point
                        // and focus itself
                        const float distance =
                                std::abs(
                                        reflectedRay.y * coord.x -
                                        reflectedRay.x * coord.y +
                                        reflectedRay.x * workerCoord.y -
                                        reflectedRay.y * workerCoord.x
                                ) / std::sqrt(reflectedRay.x * reflectedRay.x + reflectedRay.y * reflectedRay.y);

                        if (distance <= distanceThreshold) {
                            // Worker is focused, so update status and increase the intensity
                            if (!workersStatus[ch.first]) {
                                workersStatus[ch.first] = true;
                                ++intensity;
                            }
                        } else {
                            // Worker is unfocused, so update status and decrease the intensity
                            if (workersStatus[ch.first]) {
                                workersStatus[ch.first] = false;
                                --intensity;
                            }
                        }

                        // Give a feedback
                        workersForward[ch.first]->send({MessageType::INTENSITY, Intensity{intensity}});
                    }

                        break;
                    case MessageType::PING:
                        workersForward[ch.first]->send({MessageType::INTENSITY, Intensity{intensity}});
                        break;
                    default:
                        break;
                }
            }
        }
    }
}