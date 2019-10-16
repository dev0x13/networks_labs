#include "sending_transducer.h"
#include "receiving_transducer.h"

#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace pt = boost::property_tree;

// Simple helper function for file reading
std::string readFile(const std::string& pathToFile) {
    std::ifstream is(pathToFile);

    if (!is.is_open()) {
        throw std::runtime_error("Unable to open input file: " + pathToFile);
    }

    std::string data;

    data.assign((std::istreambuf_iterator<char>(is)),(std::istreambuf_iterator<char>()));

    is.close();

    return data;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: agent <path to config>\n";
        return 1;
    }

    pt::ptree ptree;

    pt::read_ini(argv[1], ptree);

    std::string mode           = ptree.get<std::string>("mode");
    std::string mqID           = ptree.get<std::string>("mq_id");
    std::string arqProtocol    = ptree.get<std::string>("protocol");

    if (mode == "SEND") {
        // Shared memory used to verify that protocols of the agents are equal
        boost::interprocess::managed_shared_memory segment(
                boost::interprocess::open_only,
                "ARQProtocol"
        );

        const std::string fileToSendPath = ptree.get<std::string>("file");
        const size_t windowSize          = ptree.get<size_t>("window_size");
        const size_t ackTimeoutMs        = ptree.get<size_t>("timeout");

        std::string dataToSend = readFile(fileToSendPath);

        auto res = segment.find<ARQProtocol>("protocol");

        if (arqProtocol == "GBN") {
            if (*res.first != GBN) {
                std::cout << "Invalid ARQ protocol\n";
                return 1;
            }

            SendingTransducer<GBN> t(mqID, windowSize, ackTimeoutMs, reinterpret_cast<const uint8_t *>(dataToSend.data()), dataToSend.size());
            std::cout << t.getStats() << "\n";
        } else if (arqProtocol == "SR") {
            if (*res.first != SR) {
                std::cout << "Invalid ARQ protocol\n";
                return 1;
            }

            SendingTransducer<SR> t(mqID, windowSize, ackTimeoutMs, reinterpret_cast<const uint8_t *>(dataToSend.data()), dataToSend.size());
            std::cout << t.getStats() << "\n";
        } else {
            std::cout << "Invalid ARQ protocol\n";
            return 1;
        }

        std::cout << "Transmission finished!\n";
        std::cout << "Sent: " << dataToSend << "\n";

    } else if (mode == "RECEIVE") {
        // Shared memory used to verify that protocols of the agents are equal
        boost::interprocess::shared_memory_object::remove("ARQProtocol");
        boost::interprocess::managed_shared_memory segment(
                boost::interprocess::create_only,
                "ARQProtocol",
                sizeof(ARQProtocol) * 100
        );

        // Create MQs
        boost::interprocess::message_queue::remove((mqID + "_ack").c_str());
        boost::interprocess::message_queue::remove((mqID + "_receive").c_str());

        std::vector<uint8_t> receivedData;
        const float lossProbability = ptree.get<float>("loss_probability");

        if (arqProtocol == "GBN") {
            segment.construct<ARQProtocol>("protocol")(GBN);

            ReceivingTransducer<GBN> t(mqID, lossProbability);
            receivedData = t.getResult();
        } else if (arqProtocol == "SR") {
            segment.construct<ARQProtocol>("protocol")(SR);

            ReceivingTransducer<SR> t(mqID, lossProbability);
            receivedData = t.getResult();
        } else {
            std::cout << "Invalid ARQ protocol\n";
            return 1;
        }

        segment.destroy<ARQProtocol>("protocol");

        std::cout << "Transmission finished!\n";
        std::cout << "Received: " << std::string(reinterpret_cast<const char*>(receivedData.data()), receivedData.size()) << "\n";
    } else {
        std::cout << "Invalid mode\n";
        return 1;
    }

    return 0;
}