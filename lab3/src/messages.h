#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <cmath>

// Each message consists of message type and string representation
// of message contents. This is not an efficient implementation, however
// it is pretty convenient

enum class MessageType {
    TOPOLOGY_OPERATION,
    INVOKE,
    VECTOR,
    VECTOR_AND_POINT,
    INTENSITY,
    PING,
    PONG,
    UNDEFINED
};

// Base message
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

// Vector message (actually should be moved to a separate header, but whatever)
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

    // Rotates vector to a given angle in radians
    void rotate(float angle) {
        const float newX = x * std::cos(angle) - y * std::sin(angle);
        const float newY = x * std::sin(angle) + y * std::cos(angle);

        x = newX;
        y = newY;
    }

    // Reflects vector using given surface normal
    static Vector reflect(const Vector& vector, const Vector& normal) {
        const float dotProduct = vector.x * normal.x + vector.y * normal.y;

        return {
            vector.x - 2 * normal.x * dotProduct,
            vector.y - 2 * normal.y * dotProduct
        };
    }
};

// Just a workaround to pass a pack of vector and point along
struct VectorAndPoint {
    Vector vector;
    Vector point;

    VectorAndPoint(const Vector& vector_, const Vector& point_) : vector(vector_), point(point_) {}

    void serialize(std::ostream& os) const {
        vector.serialize(os);
        point.serialize(os);
    }

    static VectorAndPoint deserialize(std::istream& is) {
        return {Vector::deserialize(is), Vector::deserialize(is)};
    }
};

// Basically a float number container
struct Intensity {
    size_t intensity = 0;

    Intensity(size_t intensity_) : intensity(intensity_) {}

    void serialize(std::ostream& os) const {
        os << intensity << std::endl;
    }

    static Intensity deserialize(std::istream& is) {
        size_t intensity;

        is >> intensity;

        return {intensity};
    }
};

// Some stuff used for type tagging messages
struct EmptyMessage {
    void serialize(std::ostream&) const {}
    static EmptyMessage deserialize(std::istream&) { return {}; }
};

using Ping = EmptyMessage;
using Pong = EmptyMessage;
using Invoke = EmptyMessage;