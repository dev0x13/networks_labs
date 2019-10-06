#include <boost/interprocess/ipc/message_queue.hpp>
#include <iostream>
#include <thread>

struct Message {
    const int64_t num;

    Message() : num(-1) {}
    Message(const size_t num_) : num(num_) {}

    friend std::ostream& operator<<(std::ostream& os, const Message& obj) {
        os << "Message[" << "num: " << obj.num << "]";
        return os;
    }

    bool operator==(const Message& obj) const {
        return obj.num == this->num;
    }

    bool isValid() const {
        return num >= 0;
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

    bool send(const Message& message) {
        return sendingQueue->try_send(&message, sizeof(Message), 0);
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
        std::cout << "Trying to attach to receiving queue `" << messageQueueName << "`..." << std::endl;

        while (!receivingQueue) {
            try {
                receivingQueue = std::make_shared<boost::interprocess::message_queue>(
                        boost::interprocess::open_only,
                        messageQueueName.c_str()
                );
            } catch (boost::interprocess::interprocess_exception &ex) {}
        }

        std::cout << "Attached!" << std::endl;
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

class Transducer {
public:
    enum Mode {
        SENDING,
        LISTENING
    };

    Transducer(const std::string& transducerName, Mode mode_) :
        mode(mode_)
    {
        switch (mode) {
            case SENDING:
                sender   = new Sender(transducerName + "_receive");
                receiver = new Receiver(transducerName + "_send");
                std::thread(&Transducer::sendJob, this).detach();
                break;
            case LISTENING:
                sender   = new Sender(transducerName + "_send");
                receiver = new Receiver(transducerName + "_receive");
                std::thread(&Transducer::receiveJob, this).detach();
                break;
        }
    }

    void sendJob() {
        for (;;) {
            Message message(outMessageSeq++);

            if (sender->send(message)) {
                std::cout << "Sent: " << message << " to `" << sender->getMQName() + "`" << std::endl;

                Message message1;
                receiver->receive(message1);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            // ...
        }
    }

    void receiveJob() {
        for (;;) {
            Message message;

            if (!receiver->receive(message)) {
                continue;
            }

            std::cout << "Received: " << message << " from `" <<  receiver->getMQName() << "`" <<std::endl;

            // loss...

            sender->send(message);
            // ...
        }
    }

    ~Transducer() {
        delete sender;
        delete receiver;
    }

private:
    Sender* sender{ nullptr };
    Receiver* receiver{ nullptr };

    size_t outMessageSeq;
    size_t inMessageSeq;

    Mode mode;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: agent <receiver name> <sender name>\n";
        return 1;
    }

    Transducer receivingTransducer(argv[1], Transducer::LISTENING);
    Transducer sendingTransducer(argv[2], Transducer::SENDING);

    for (;;) {}

    return 0;
}