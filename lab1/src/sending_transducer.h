#pragma once

#include "primitive_transducers.h"

#include <thread>
#include <mutex>
#include <iostream>

enum ARQProtocol {
    GBN,
    SR
};

template <ARQProtocol protocol>
class SendingTransducer {
public:
    SendingTransducer(const std::string& mqID, size_t windowSize_, const uint8_t* data, size_t dataLength) :
            windowSize(windowSize_)
    {
        for (size_t i = 0; i < dataLength - 1; ++i) {
            outcoming.emplace_back(data[i]);
        }

        outcoming.emplace_back(data[dataLength - 1], true);

        cursor = 0;

        sender = new Sender(mqID + "_receive");
        ack    = new Receiver(mqID + "_ack");
        std::thread(&SendingTransducer::receiveAck, this).detach();
        sendJob();
    }

    void sendJob() {
        static const size_t ackTimeoutMs = 1000;

        while (cursor < outcoming.size() - 1) {
            {
                std::unique_lock<std::mutex> lock(rwLock);

                for (size_t i = cursor; i < cursor + windowSize && i < outcoming.size(); ++i) {

                    if (outcoming[i].status != Message::ACK) {
                        outcoming[i].seqNum = i;

                        if (sender->send(outcoming[i])) {
                            std::cout << "Sent: " << outcoming[i] << " to `" << sender->getMQName() + "`" << std::endl;
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(ackTimeoutMs));
        }
    }

    void receiveAck() {
        while (cursor < outcoming.size()) {
            {
                std::unique_lock<std::mutex> lock(rwLock);
                Message message;

                if (!ack->receive(message)) {
                    continue;
                }

                if (!message.isValid()) {
                    continue;
                }

                std::cout << "Ack: " << message << " from `" << ack->getMQName() << "`" << std::endl;

                if (message.isFinal) {
                    ++cursor;
                    break;
                }

                const size_t cursorCopy = cursor;

                switch (protocol) {
                    case SR:
                        for (size_t i = cursorCopy; i < cursorCopy + windowSize && i < outcoming.size(); ++i) {
                            if (outcoming[i] == message) {
                                outcoming[i].status = Message::ACK;

                                if (i == 0 || outcoming[cursor].status == Message::ACK) {
                                    ++cursor;
                                }

                                break;
                            }
                        }
                        break;
                    case GBN:
                        if (outcoming[cursor] == message) {
                            outcoming[cursor].status = Message::ACK;
                            ++cursor;
                        }
                        break;
                }
            }
        }
    }

    ~SendingTransducer() {
        delete sender;
        delete ack;
    }

private:
    Sender* sender{ nullptr };
    Receiver* ack{ nullptr };

    size_t windowSize;

    std::vector<Message> outcoming;
    size_t cursor;
    std::mutex rwLock;
};