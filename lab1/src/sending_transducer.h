#pragma once

#include "one_way_transducer.h"

#include <thread>
#include <mutex>
#include <iostream>

enum ARQProtocol {
    GBN,
    SR
};

struct SendingStats {
    const size_t packetsToSendNum = 0;
    size_t packetsActuallySentNum = 0;
    float efficiencyCoeff = -1;
    float totalTimeMs = 0;

    explicit SendingStats(size_t packetsToSendNum_) : packetsToSendNum(packetsToSendNum_) {}

    void incActuallySentNum() {
        ++packetsActuallySentNum;
    }

    void start() {
        if (efficiencyCoeff != -1) {
            throw std::runtime_error("Stats has already been finalized");
        }

        startTime = std::chrono::high_resolution_clock::now();
    }

    void finalize() {
        if (packetsActuallySentNum != 0) {
            efficiencyCoeff = (float) packetsToSendNum / packetsActuallySentNum;
        }

        const auto endTime = std::chrono::high_resolution_clock::now();
        totalTimeMs = (endTime - startTime) / std::chrono::milliseconds(1);
    }

    friend std::ostream& operator<<(std::ostream& os, const SendingStats& obj) {
        os << "SendingStats[" <<
            "packetsToSendNum: "       << obj.packetsToSendNum       << ", "
            "packetsActuallySentNum: " << obj.packetsActuallySentNum << ", "
            "efficiencyCoeff: "        << obj.efficiencyCoeff        << ", "
            "totalTimeMs: "            << obj.totalTimeMs            << "]";
        return os;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

};

template <ARQProtocol protocol>
class SendingTransducer {
public:
    SendingTransducer(const std::string& mqID, size_t windowSize_, size_t timeoutMs_, const uint8_t* data, size_t dataLength) :
            sender(mqID + "_receive"),
            ack(mqID + "_ack"),
            windowSize(windowSize_),
            timeoutMs(timeoutMs_),
            stats(dataLength)
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
        stats.start();

        while (cursor < outcoming.size() - 1) {
            {
                std::unique_lock<std::mutex> lock(rwLock);

                for (size_t i = cursor; i < cursor + windowSize && i < outcoming.size() - 1; ++i) {
                    if (outcoming[i].status != Message::ACK) {
                        outcoming[i].seqNum = i;

                        if (sender.transduce(outcoming[i])) {
                            stats.incActuallySentNum();
                            std::cout << "Sent: " << outcoming[i] << " to `" << sender.getMQName() + "`" << std::endl;
                        }
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
        }

        // This is a special case: last message is marked as final.
        // Receiver will stop receiving on final message, so we have to
        // hold it from send until all previous message have been sent
        // (necessary for SR protocol).
        outcoming[cursor].seqNum = cursor;

        while (cursor < outcoming.size()) {
            if (sender.transduce(outcoming[cursor])) {
                stats.incActuallySentNum();
                std::cout << "Sent: " << outcoming[cursor] << " to `" << sender.getMQName() + "`" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
        }

        stats.finalize();
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

    const SendingStats& getStats() {
        return stats;
    }

private:
    OneWayTransducer<SENDING> sender;
    OneWayTransducer<RECEIVING> ack;

    const size_t windowSize;
    size_t cursor;

    const size_t timeoutMs;

    std::vector<Message> outcoming;

    std::mutex rwLock;

    SendingStats stats;
};