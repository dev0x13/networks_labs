#pragma once

#include <string>
#include <iostream>
#include <sstream>

enum class MessageType {
    TOPOLOGY_OPERATION,
    INVOKE,
    VECTOR,
    INTENSITY,
    PING,
    PONG,
    UNDEFINED
};

struct Message {
    MessageType messageType = MessageType::UNDEFINED;
    std::stringstream serializedMessage;

    void serialize(std::stringstream& os) const {
        os << static_cast<int>(messageType) << std::endl;
        os << serializedMessage.str() << std::endl;
    }

    static Message deserialize(std::stringstream& is) {
        int messageTypeIntRepr;
        is >> messageTypeIntRepr;

        return { static_cast<MessageType>(messageTypeIntRepr), is };
    }

    template <typename T>
    T getMessageContent() {
        return T::deserialize(serializedMessage);
    }

    template <typename T>
    void setMessageContent(const T& t) {
        t.serialize(serializedMessage);
    }

    Message() = default;

    template <typename T>
    Message(const MessageType& messageType_, const T& t) : messageType(messageType_) { setMessageContent(t); }

    Message(const MessageType& messageType_, std::stringstream& serializedMessage_)
      : messageType(messageType_), serializedMessage(serializedMessage_.str())
    {
        serializedMessage.seekg(serializedMessage_.tellg());
        serializedMessage.seekp(serializedMessage_.tellp());
        serializedMessage.setstate(serializedMessage_.rdstate());
    }
};

struct Vector {
    float x = 0;
    float y = 0;

    Vector(const float& x_, const float& y_) : x(x_), y(y_) {}

    void serialize(std::ostream& os) const {
        os << x << std::endl;
        os << y << std::endl;
    }

    static Vector deserialize(std::istream& is) {
        float x, y;

        is >> x;
        is >> y;

        return {x, y};
    }
};

struct EmptyMessage {
    void serialize(std::ostream&) const {}
    static EmptyMessage deserialize(std::istream&) { return {}; }
};

using Ping = EmptyMessage;
using Pong = EmptyMessage;
using Invoke = EmptyMessage;

struct Intensity {
    float intensity = 0;

    Intensity(float intensity_) : intensity(intensity_) {}

    void serialize(std::ostream& os) const {
        os << intensity << std::endl;
    }

    static Intensity deserialize(std::istream& is) {
        float intensity;

        is >> intensity;

        return {intensity};
    }
};