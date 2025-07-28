// Source/ErrorHandler.cpp
#include "../Headers/ErrorHandler.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <comdef.h>

std::mutex ErrorHandler::logMutex;
std::ofstream ErrorHandler::logFile;
bool ErrorHandler::initialized = false;

void ErrorHandler::InitializeLogger() {
    if (initialized) return;
    
    std::lock_guard<std::mutex> lock(logMutex);
    if (initialized) return;
    
    try {
        logFile.open("LMWallpaper.log", std::ios::app);
        if (logFile.is_open()) {
            logFile << "\n=== LMWallpaper Log Started - " << GetCurrentTime() << " ===\n";
            logFile.flush();
            initialized = true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Log dosyası açılamadı: " << e.what() << std::endl;
    }
}

std::string ErrorHandler::GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string ErrorHandler::ErrorLevelToString(ErrorLevel level) {
    switch (level) {
        case ErrorLevel::INFO: return "INFO";
        case ErrorLevel::WARNING: return "WARNING";
        case ErrorLevel::ERROR: return "ERROR";
        case ErrorLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string ErrorHandler::InfoLevelToString(InfoLevel level) {
    switch (level) {
        case InfoLevel::INFO: return "INFO";
        case InfoLevel::DEBUG: return "DEBUG";
        case InfoLevel::TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

void ErrorHandler::WriteToFile(const std::string& message) {
    if (!initialized) {
        InitializeLogger();
    }
    
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.flush();
    }
}

void ErrorHandler::LogError(const std::string& message, ErrorLevel level) {
    std::string timestamp = GetCurrentTime();
    std::string levelStr = ErrorLevelToString(level);
    std::string fullMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    WriteToFile(fullMessage);
    
    // Debug modunda console'a da yazdır
    #ifdef _DEBUG
    std::cout << fullMessage << std::endl;
    #endif
    
    // Critical hatalar için MessageBox göster
    if (level == ErrorLevel::CRITICAL) {
        ShowErrorDialog(message, "Kritik Hata");
    }
}

void ErrorHandler::LogInfo(const std::string& message, InfoLevel level) {
    std::string timestamp = GetCurrentTime();
    std::string levelStr = InfoLevelToString(level);
    std::string fullMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    WriteToFile(fullMessage);
    
    // Debug modunda console'a da yazdır
    #ifdef _DEBUG
    if (level != InfoLevel::TRACE) {
        std::cout << fullMessage << std::endl;
    }
    #endif
}

void ErrorHandler::ShowErrorDialog(const std::string& message, const std::string& title) {
    std::wstring wMessage(message.begin(), message.end());
    std::wstring wTitle(title.begin(), title.end());
    MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(), MB_ICONERROR | MB_OK);
}

void ErrorHandler::ShowInfoDialog(const std::string& message, const std::string& title) {
    std::wstring wMessage(message.begin(), message.end());
    std::wstring wTitle(title.begin(), title.end());
    MessageBoxW(nullptr, wMessage.c_str(), wTitle.c_str(), MB_ICONINFORMATION | MB_OK);
}

std::string ErrorHandler::GetLastErrorAsString() {
    DWORD errorMessageID = GetLastError();
    if (errorMessageID == 0) {
        return "Hata yok";
    }
    
    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&messageBuffer, 0, nullptr);
    
    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    
    return "Hata " + std::to_string(errorMessageID) + ": " + message;
}

std::string ErrorHandler::HRESULTToString(HRESULT hr) {
    _com_error err(hr);
    std::wstring wMessage = err.ErrorMessage();
    std::string message(wMessage.begin(), wMessage.end());
    
    std::stringstream ss;
    ss << "HRESULT 0x" << std::hex << hr << ": " << message;
    return ss.str();
}

void ErrorHandler::Cleanup() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile << "=== LMWallpaper Log Ended - " << GetCurrentTime() << " ===\n";
        logFile.close();
    }
    initialized = false;
}