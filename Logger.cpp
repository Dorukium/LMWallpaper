// Logger.cpp
#include "Logger.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <fmt/format.h>

std::mutex Logger::mutex_;
std::ofstream Logger::fileLog_;

void Logger::initialize() {
    fileLog_.open("log.txt", std::ios::app);
    if (!fileLog_.is_open()) {
        throw std::runtime_error("Log dosyası açılamadı");
    }
}

void Logger::log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto timestamp = getCurrentTime();
    fileLog_ << fmt::format("[{}] {}: {}\n", 
        timestamp, getLogLevelString(level), message);
    fileLog_.flush();
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", 
        localtime(&now_time));
    return fmt::format("{}.{:03d}", timestamp, static_cast<int>(now_ms.count()));
}

std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        case CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}