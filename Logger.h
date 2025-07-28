#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

enum class LogLevel {
    INFO,
    WARNING,
    ERROR
};

struct LogEntry {
    LogLevel level;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

class Logger {
public:
    static Logger& GetInstance();

    void Log(const std::string& message, LogLevel level = LogLevel::INFO);
    void Log(const std::wstring& message, LogLevel level = LogLevel::INFO);

    const std::vector<LogEntry>& GetHistory() const;
    void SetLogFile(const std::string& filePath);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void WriteToFile(const LogEntry& entry);

    std::vector<LogEntry> m_history;
    std::ofstream m_logFile;
    std::mutex m_mutex;
};
