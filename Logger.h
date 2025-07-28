// Logger.h
#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <chrono>
#include <fmt/format.h>

class Logger {
public:
    static void initialize();
    static void log(const std::string& message, LogLevel level = INFO);
    
private:
    static std::mutex mutex_;
    static std::ofstream fileLog_;
    static std::string getCurrentTime();
    static std::string getLogLevelString(LogLevel level);
};