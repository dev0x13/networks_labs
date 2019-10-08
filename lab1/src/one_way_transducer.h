#pragma once

#include <boost/interprocess/ipc/message_queue.hpp>

#include <iostream>

// Hardcoded for simplicity
#define MQ_SIZE 1000

// Basic message class
struct Message {
    enum Status {
        IDLE,
        SENT,
        ACK
    };

    uint8_t b;
    int64_t seqNum;
    Status status = IDLE;
    bool isFinal;

    Message(): b(0), seqNum(-1), isFinal(false) {}

    Message(const uint8_t b_, bool isFinal_ = false) : b(b_), seqNum(-1), isFinal(isFinal_) {}

    friend std::ostream& operator<<(std::ostream& os, const Message& obj) {
        os << "Message[" << "b: " << obj.b << "]";
        return os;
    }

    bool operator==(const Message& obj) const {
        return obj.b == this->b && obj.seqNum == this->seqNum;
    }

    bool isValid() const {
        return seqNum != -1;
    }
};

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
                sizeof(Message)
            )
    {
        std::cout
            << "Attached to "
            << (mode == SENDING ? "sending" : "receiving")
            << " queue `" << messageQueueName << "`"
            << std::endl;
    }

    bool transduce(Message& message) {
        if (mode == SENDING) {
            bool res = messageQueue.try_send(&message, sizeof(Message), 0);

            if (res) {
                message.status = Message::SENT;
            }

            return res;
        } else {
            unsigned int priority;
            boost::interprocess::message_queue::size_type recvdSize;

            messageQueue.try_receive(&message, sizeof(Message), recvdSize, priority);

            return recvdSize == sizeof(Message) && message.isValid();
        }
    }

    const std::string& getMQName() const {
        return messageQueueName;
    }

private:
    const std::string messageQueueName;
    boost::interprocess::message_queue messageQueue;
};
