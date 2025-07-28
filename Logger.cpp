#include "Logger.h"
#include <locale>
#include <codecvt>

// NOTE: Removed dependency on external 'fmt' library.
// Using standard C++ libraries for logging.

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // Default constructor
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void Logger::Log(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    LogEntry entry{ level, message, std::chrono::system_clock::now() };
    m_history.push_back(entry);
    WriteToFile(entry);
}

void Logger::Log(const std::wstring& message, LogLevel level) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    Log(converter.to_bytes(message), level);
}

const std::vector<LogEntry>& Logger::GetHistory() const {
    return m_history;
}

void Logger::SetLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logFile.open(filePath, std::ios_base::app);
}

void Logger::WriteToFile(const LogEntry& entry) {
    if (!m_logFile.is_open()) return;

    std::string levelStr;
    switch (entry.level) {
    case LogLevel::INFO:    levelStr = "INFO"; break;
    case LogLevel::WARNING: levelStr = "WARNING"; break;
    case LogLevel::ERROR:   levelStr = "ERROR"; break;
    }

    auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");

    m_logFile << "[" << ss.str() << "] [" << levelStr << "] " << entry.message << std::endl;
}
