#include "sending_transducer.h"
#include "receiving_transducer.h"

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: agent <mode: SEND | RECEIVE> <MQ ID> <ARQ protocol: GBN | SR>\n";
        return 1;
    }

    const std::string mode = argv[1];
    const std::string arqProtocol = argv[3];

    const std::string dataToSend = "hello world!";

    if (mode == "SEND") {
        if (arqProtocol == "GBN") {
            SendingTransducer<GBN> t(argv[2], 3, reinterpret_cast<const uint8_t *>(dataToSend.data()), dataToSend.size());
        } else if (arqProtocol == "SR") {
            SendingTransducer<SR> t(argv[2], 3, reinterpret_cast<const uint8_t *>(dataToSend.data()), dataToSend.size());
        } else {
            std::cout << "Invalid ARQ protocol\n";
            return 1;
        }
        std::cout << "Transmission finished!\n";
        std::cout << "Sent: " << dataToSend << "\n";
    } else if (mode == "RECEIVE") {

        boost::interprocess::message_queue::remove((std::string(argv[2]) + "_ack").c_str());
        boost::interprocess::message_queue::remove((std::string(argv[2]) + "_receive").c_str());

        if (arqProtocol == "GBN") {
            ReceivingTransducer<GBN> t(argv[2], 0.2);
            std::cout << "Transmission finished!\n";
            std::cout << "Result: " << std::string(reinterpret_cast<const char*>(t.getResult().data()), t.getResult().size()) << "\n";
        } else if (arqProtocol == "SR") {
            ReceivingTransducer<SR> t(argv[2], 0.2);
            std::cout << "Transmission finished!\n";
            std::cout << "Result: " << std::string(reinterpret_cast<const char*>(t.getResult().data()), t.getResult().size()) << "\n";
        } else {
            std::cout << "Invalid ARQ protocol\n";
            return 1;
        }
    } else {
        std::cout << "Invalid mode\n";
        return 1;
    }

    return 0;
}