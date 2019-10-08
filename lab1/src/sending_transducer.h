#pragma once

#include "one_way_transducer.h"

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
    SendingTransducer(const std::string& mqID, size_t windowSize_, size_t timeoutMs_, const uint8_t* data, size_t dataLength) :
            sender(mqID + "_receive"),
            ack(mqID + "_ack"),
            windowSize(windowSize_),
            timeoutMs(timeoutMs_)
    {
        for (size_t i = 0; i < dataLength - 1; ++i) {
            outcoming.emplace_back(data[i]);
        }

        outcoming.emplace_back(data[dataLength - 1], true);

        cursor = 0;

        std::thread(&SendingTransducer::receiveAck, this).detach();
        sendJob();
    }

    void sendJob() {
        while (cursor < outcoming.size()) {
            {
                std::unique_lock<std::mutex> lock(rwLock);

                for (size_t i = cursor; i < cursor + windowSize && i < outcoming.size(); ++i) {

                    if (outcoming[i].status != Message::ACK) {
                        outcoming[i].seqNum = i;

                        if (sender.transduce(outcoming[i])) {
                            std::cout << "Sent: " << outcoming[i] << " to `" << sender.getMQName() + "`" << std::endl;
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
        }
    }

    void receiveAck() {
        while (cursor < outcoming.size()) {
            {
                std::unique_lock<std::mutex> lock(rwLock);
                Message message;

                if (!ack.transduce(message) || !message.isValid()) {
                    continue;
                }

                std::cout << "Ack: " << message << " from `" << ack.getMQName() << "`" << std::endl;

                switch (protocol) {
                    case SR:
                        outcoming[message.seqNum].status = Message::ACK;

                        while (outcoming[cursor].status == Message::ACK) {
                            ++cursor;
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

private:
    OneWayTransducer<SENDING> sender;
    OneWayTransducer<RECEIVING> ack;

    const size_t windowSize;
    size_t cursor;

    const size_t timeoutMs;

    std::vector<Message> outcoming;

    std::mutex rwLock;
};