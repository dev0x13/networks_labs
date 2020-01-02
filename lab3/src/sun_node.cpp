#include "sun_node.h"

SunNode::SunNode(
    const std::string& nodeID,
    const std::vector<NodeIndex>& workers,
    const Vector& initialCoord,
    const float movingSpeed_,
    const float absMovingBound_)
    :
    log("SunNode"),
    coord(initialCoord),
    movingSpeed(movingSpeed_),
    absMovingBound(absMovingBound_)
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
    }

    std::thread(&SunNode::pongJob, this).detach();
    moveJob();
}

void SunNode::moveJob() {
    while (!lifeTimer.expired())
    {
        {
            std::unique_lock<std::mutex> lock(movingMutex);

            if (std::abs(coord.x) >= absMovingBound) {
                movingDirectionSign = -movingDirectionSign;
            }

            coord.x += movingSpeed * movingDirectionSign;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(coordUpdateTimeMs));
    }
}

void SunNode::pongJob() {
    Message message;

    while (!lifeTimer.expired()) {
        for (auto& ch : workersBackward) {
            if (ch.second->receive(message)) {
                switch (message.messageType) {
                    case MessageType::PING:
                        workersForward[ch.first]->send({MessageType::VECTOR, coord});
                        break;
                    default:
                        break;
                }
            }
        }
    }
}