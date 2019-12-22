#pragma once

#include <boost/interprocess/ipc/message_queue.hpp>
#include "messages.h"

// Hardcoded for simplicity
#define MQ_SIZE 1000

// This value is actually critical. If network is quite
// big, then it should be increased.
#define MAX_MESSAGE_SIZE_BYTES 1000

enum TransducerMode {
    SENDING,
    RECEIVING
};

// Simple one way sending/receiving transducer with no feedback
template <TransducerMode mode, typename BoostQueueMode>
class OneWayTransducer {
public:
    explicit OneWayTransducer(const std::string& messageQueueName_, BoostQueueMode) :
            messageQueueName(messageQueueName_),
            messageQueue(
                BoostQueueMode(),
                messageQueueName.c_str(),
                MQ_SIZE,
                MAX_MESSAGE_SIZE_BYTES)
    {
        init();
    }

    OneWayTransducer(const OneWayTransducer& other) : OneWayTransducer(other.messageQueueName) {}

    ~OneWayTransducer() {
        delete[] buffer;
    }

    bool send(const Message& message) {
        static_assert(mode == SENDING, "Cannot send to a receiving queue");

        std::stringstream ss;
        message.serialize(ss);

        const std::string dataToSend = ss.str();

        assert(dataToSend.size() <= MAX_MESSAGE_SIZE_BYTES);

        return messageQueue.try_send(dataToSend.c_str(), dataToSend.size(), 0);
    }

    bool receive(Message& message) {
        static_assert(mode == RECEIVING, "Cannot receive from a sending queue");

        unsigned int priority;
        boost::interprocess::message_queue::size_type recvdSize;
        bool res = messageQueue.try_receive(buffer, MAX_MESSAGE_SIZE_BYTES, recvdSize, priority);

        if (res) {
            std::string dataToReceive(buffer, buffer + recvdSize);

            std::stringstream ss;

            ss << dataToReceive;

            message = Message::deserialize(ss);
        }

        return res;
    }

    const std::string& getMQName() const {
        return messageQueueName;
    }

private:
    void init() {
        buffer = new uint8_t[MAX_MESSAGE_SIZE_BYTES];

        /*
        std::cout
                << "[OneWayTransducer] Attached to "
                << (mode == SENDING ? "sending" : "receiving")
                << " queue `" << messageQueueName << "`"
                << std::endl;
        */
    }

    const std::string messageQueueName;
    boost::interprocess::message_queue messageQueue;
    uint8_t* buffer = nullptr;
};

// This is a very weird way to do it, but boost::interprocess::message_queue is poorly designed
// and there is no flexibility to call constructor with different queue opening modes
template<>
inline OneWayTransducer<SENDING, boost::interprocess::open_only_t>::OneWayTransducer(const std::string& messageQueueName_, boost::interprocess::open_only_t) :
        messageQueueName(messageQueueName_),
        messageQueue(boost::interprocess::open_only, messageQueueName.c_str())
{
    init();
}

template<>
inline OneWayTransducer<RECEIVING, boost::interprocess::open_only_t>::OneWayTransducer(const std::string& messageQueueName_, boost::interprocess::open_only_t) :
        messageQueueName(messageQueueName_),
        messageQueue(boost::interprocess::open_only, messageQueueName.c_str())
{
    init();
}