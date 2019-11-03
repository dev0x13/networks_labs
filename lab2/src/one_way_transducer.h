#pragma once

#include <boost/interprocess/ipc/message_queue.hpp>

#include <iostream>

// Hardcoded for simplicity
#define MQ_SIZE 1000
#define MAX_MESSAGE_SIZE_BYTES 1000

enum TransducerMode {
    SENDING,
    RECEIVING
};

// Simple one way sending/receiving transducer with no feedback
template <TransducerMode mode>
class OneWayTransducer {
public:
    explicit OneWayTransducer(const std::string& messageQueueName_) :
            messageQueueName(messageQueueName_),
            messageQueue(
                boost::interprocess::open_or_create,
                messageQueueName.c_str(),
                MQ_SIZE,
                MAX_MESSAGE_SIZE_BYTES
            )
    {
        buffer = new uint8_t[MAX_MESSAGE_SIZE_BYTES];
        std::cout
            << "Attached to "
            << (mode == SENDING ? "sending" : "receiving")
            << " queue `" << messageQueueName << "`"
            << std::endl;
    }

    OneWayTransducer(const OneWayTransducer& other) : OneWayTransducer(other.messageQueueName) {}

    ~OneWayTransducer() {
        delete[] buffer;
    }

    bool send(const std::string& message) {
        assert(mode == SENDING);
        assert(message.size() <= MAX_MESSAGE_SIZE_BYTES);

        return messageQueue.try_send(message.c_str(), message.size(), 0);
    }

    bool receive(std::string& message) {
        assert(mode == RECEIVING);
        assert(message.size() <= MAX_MESSAGE_SIZE_BYTES);

        unsigned int priority;
        boost::interprocess::message_queue::size_type recvdSize;
        bool res = messageQueue.try_receive(buffer, MAX_MESSAGE_SIZE_BYTES, recvdSize, priority);

        if (res) {
            message.resize(recvdSize);
            message.assign(buffer, buffer + recvdSize);
        }

        return res;
    }

    const std::string& getMQName() const {
        return messageQueueName;
    }

private:
    const std::string messageQueueName;
    boost::interprocess::message_queue messageQueue;
    uint8_t* buffer = nullptr;
};
