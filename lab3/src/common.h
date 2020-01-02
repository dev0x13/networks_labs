#pragma once

#include <chrono>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <iostream>

// Some type definitions for network formulation
using NodeIndex = std::string;
using Cost = int64_t;
using Graph = std::unordered_map<NodeIndex, std::unordered_map<NodeIndex, Cost>>;

// Basic timer class
struct Timer {
    Timer(int64_t lifeTimeMs_) : lifeTimeMs(lifeTimeMs_) {
        startTime = std::chrono::system_clock::now();
    }

    bool expired() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now() - startTime
               ).count() >= lifeTimeMs;
    }

    const int64_t lifeTimeMs;

private:
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> startTime;
};

// Base node class, contains life timer
class BaseNode {
public:
    BaseNode(const int64_t lifeTimeMs = 15000) : lifeTimer(lifeTimeMs) {}

    Timer lifeTimer;
};

// Simple logger
class Logger {
public:
    Logger(const std::string& prefix_, bool logToStdout_ = true) : prefix("[" + prefix_ + "] "), logToStdout(logToStdout_) {
        logFile.open(prefix);

        if (!logFile.is_open()) {
            throw std::runtime_error("Cannot open log file");
        }

        logFile << prefix;

        if (logToStdout) {
            std::cout << prefix;
        }
    }

    Logger &operator<<(std::ostream& (*pf) (std::ostream&)) {
        logFile << pf;

        if (logToStdout) {
            std::cout << pf;
        }

        // Just to provide correct prefix for log lines
        if (pf == static_cast<std::ostream& (*)(std::ostream&)>(&std::endl<char, std::char_traits<char>>)) {
            logFile << prefix;

            if (logToStdout) {
                std::cout << prefix;
            }
        }

        return *this;
    }

    template <typename T>
    Logger& operator<<(const T& info) {
        logFile << info;

        if (logToStdout) {
            std::cout << info;
        }
        return *this;
    }

    ~Logger() {
        logFile.close();
    }

private:
    const std::string prefix;
    const bool logToStdout;
    std::ofstream logFile;
};