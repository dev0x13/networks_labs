#pragma once

#include <boost/interprocess/ipc/message_queue.hpp>

#include <iostream>

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

class Sender {
public:
    Sender(const std::string& messageQueueName_) :
            messageQueueName(messageQueueName_)
    {
        //boost::interprocess::message_queue::remove(messageQueueName.c_str());

        sendingQueue = std::make_shared<boost::interprocess::message_queue>(
                boost::interprocess::open_or_create,
                messageQueueName.c_str(),
                1000, // TODO: param
                sizeof(Message)
        );

        std::cout << "Attached to sending queue `" << messageQueueName << "`" << std::endl;
    }

    bool send(Message& message) {
        bool res = sendingQueue->try_send(&message, sizeof(Message), 0);

        if (res) {
            message.status = Message::SENT;
        }

        return res;
    }

    const std::string& getMQName() const {
        return messageQueueName;
    }

private:
    const std::string messageQueueName;
    std::shared_ptr<boost::interprocess::message_queue> sendingQueue{ nullptr };
};

class Receiver {
public:
    Receiver(const std::string& messageQueueName_) :
            messageQueueName(messageQueueName_)
    {
        receivingQueue = std::make_shared<boost::interprocess::message_queue>(
                boost::interprocess::open_or_create,
                messageQueueName.c_str(),
                1000, // TODO: param
                sizeof(Message)
        );

        std::cout << "Attached to receiving queue `" << messageQueueName << "`" << std::endl;
    }

    bool receive(Message& message) {
        unsigned int priority;
        boost::interprocess::message_queue::size_type recvdSize;

        receivingQueue->try_receive(&message, sizeof(Message), recvdSize, priority);

        return recvdSize == sizeof(Message) && message.isValid();
    }

    const std::string& getMQName() const {
        return messageQueueName;
    }

private:
    const std::string messageQueueName;
    std::shared_ptr<boost::interprocess::message_queue> receivingQueue{ nullptr };
};
