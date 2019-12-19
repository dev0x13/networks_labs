#pragma once

#include <string>

struct Vector {
    float x = 0;
    float y = 0;

    Vector(const float& x_, const float& y_) : x(x_), y(y_) {}
};

struct InvokeMessage {
    std::string nextWorkerId;
};