#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>

// NOTE: Changed to 'enum class' for type safety and scoping.
// Added a semicolon at the end. This fixes numerous compilation errors.
enum class ErrorLevel {
    INFO,
    WARNING,
    CRITICAL
};

struct Error {
    std::string message;
    ErrorLevel level;
    std::chrono::system_clock::time_point timestamp;
};

class ErrorHandler {
public:
    ErrorHandler(const std::string& logFilePath = "error_log.txt");
    ~ErrorHandler();

    void HandleError(const std::string& errorMessage, ErrorLevel level = ErrorLevel::INFO);
    void HandleError(const std::wstring& errorMessage, ErrorLevel level = ErrorLevel::INFO);
    void HandleError(HRESULT hr, const std::string& functionName, ErrorLevel level = ErrorLevel::CRITICAL);
    void HandleError(HRESULT hr, const std::wstring& functionName, ErrorLevel level = ErrorLevel::CRITICAL);

    const std::vector<Error>& GetErrors() const;
    void ClearErrors();

private:
    void LogError(const Error& error);

    std::vector<Error> m_errors;
    std::ofstream m_logFile;
    std::mutex m_mutex;
};
