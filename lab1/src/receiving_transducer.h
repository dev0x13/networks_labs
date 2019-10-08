#pragma once

#include "one_way_transducer.h"

template <ARQProtocol protocol>
class ReceivingTransducer {
public:
    ReceivingTransducer(const std::string& mqID, float lossProbability_):
        ack(mqID + "_ack"),
        receiver(mqID + "_receive"),
        lossProbability(lossProbability_)
    {
        incoming.resize(1);

        receiveJob();
    }

    std::vector<uint8_t> getResult() const {
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

                if (!receiver.transduce(message) ||
                    !message.isValid() ||
                    (protocol == ARQProtocol::GBN && message.seqNum != numSeqExpected))
                {
                    continue;
                }

                if (getRandomFloat() < lossProbability) {
                    std::cout << "Lost: " << message << " from `" << receiver.getMQName() << "`" <<std::endl;
                    continue;
                }

                if (ack.transduce(message)) {
                    std::cout << "Received: " << message << " from `" << receiver.getMQName() << "`" <<std::endl;

                    if ((size_t) message.seqNum >= incoming.size()) {
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

private:
    OneWayTransducer<SENDING>   ack;
    OneWayTransducer<RECEIVING> receiver;

    // Received messages
    std::vector<Message> incoming;

    // For GBN
    int64_t numSeqExpected = 0;

    const float lossProbability;
};