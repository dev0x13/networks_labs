#pragma once

#include "primitive_transducers.h"

#include <iostream>
#include <list>

template <ARQProtocol protocol>
class ReceivingTransducer {
public:
    ReceivingTransducer(const std::string& mqID, float lossProbability_):
        lossProbability(lossProbability_)
    {
        ack      = new Sender(mqID + "_ack");
        receiver = new Receiver(mqID + "_receive");

        incoming.resize(1);

        receiveJob();
    }

    ~ReceivingTransducer() {
        delete ack;
        delete receiver;
    }

    std::vector<uint8_t> getResult() {
        std::vector<uint8_t> res;

        for (const auto& m : incoming) {
            res.emplace_back(m.b);
        }

        return res;
    }

private:
    void receiveJob() {
        for (;;) {
            {
                Message message;

                if (!receiver->receive(message)) {
                    continue;
                }

                if (!message.isValid()) {
                    continue;
                }

                if (getRandomFloat() < lossProbability) {
                    std::cout << "Lost: " << message << " from `" << receiver->getMQName() << "`" <<std::endl;
                    continue;
                }

                if (protocol == ARQProtocol::GBN && message.seqNum != numSeqExpected) {
                    continue;
                }

                if (ack->send(message)) {
                    std::cout << "Received: " << message << " from `" << receiver->getMQName() << "`" <<std::endl;

                    if (message.seqNum >= incoming.size()) {
                        incoming.resize(message.seqNum + 1);
                    }

                    incoming[message.seqNum] = message;
                    ++numSeqExpected;

                    if (message.isFinal) {
                        break;
                    }
                }
            }
        }
    }

    static float getRandomFloat() {
        static std::default_random_engine e(time(nullptr));
        static std::uniform_real_distribution<> dis(0, 1);
        return dis(e);
    }

    Sender* ack{ nullptr };
    Receiver* receiver{ nullptr };

    std::vector<Message> incoming;
    int64_t numSeqExpected = 0;
    const float lossProbability;
};