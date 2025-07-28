#include "../Headers/ErrorHandler.h"
#include <comdef.h>
#include <locale>
#include <codecvt>

ErrorHandler::ErrorHandler(const std::string& logFilePath) {
    m_logFile.open(logFilePath, std::ios_base::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
    }
}

ErrorHandler::~ErrorHandler() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void ErrorHandler::HandleError(const std::string& errorMessage, ErrorLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Error error = { errorMessage, level, std::chrono::system_clock::now() };
    m_errors.push_back(error);
    LogError(error);
}

void ErrorHandler::HandleError(const std::wstring& errorMessage, ErrorLevel level) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    HandleError(converter.to_bytes(errorMessage), level);
}

void ErrorHandler::HandleError(HRESULT hr, const std::string& functionName, ErrorLevel level) {
    _com_error err(hr);
    // NOTE: err.ErrorMessage() returns a TCHAR*, which needs to be converted for std::string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::string errorMessage = functionName + " failed with HRESULT: " + std::to_string(hr) + " - " + converter.to_bytes(err.ErrorMessage());
    HandleError(errorMessage, level);
}

void ErrorHandler::HandleError(HRESULT hr, const std::wstring& functionName, ErrorLevel level) {
    _com_error err(hr);
    // NOTE: Explicitly cast the result of err.ErrorMessage() to const wchar_t* for concatenation.
    std::wstring errorMessage = functionName + L" failed with HRESULT: " + std::to_wstring(hr) + L" - " + (const wchar_t*)err.ErrorMessage();
    HandleError(errorMessage, level);
}

void ErrorHandler::LogError(const Error& error) {
    if (!m_logFile.is_open()) return;

    std::string levelStr;
    switch (error.level) {
    case ErrorLevel::INFO:
        levelStr = "INFO";
        break;
    case ErrorLevel::WARNING:
        levelStr = "WARNING";
        break;
    case ErrorLevel::CRITICAL:
        levelStr = "CRITICAL";
        break;
    }

    auto time_t = std::chrono::system_clock::to_time_t(error.timestamp);
    struct tm timeinfo;
    localtime_s(&timeinfo, &time_t);

    std::stringstream ss;
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");

    m_logFile << "[" << ss.str() << "] [" << levelStr << "] " << error.message << std::endl;
}

const std::vector<Error>& ErrorHandler::GetErrors() const {
    return m_errors;
}

void ErrorHandler::ClearErrors() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errors.clear();
}
