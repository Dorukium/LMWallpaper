// Headers/ErrorHandler.h
#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <Windows.h>

enum class ErrorLevel {
    INFO = 0,
    WARNING = 1,
    ERROR = 2,
    CRITICAL = 3
};

enum class InfoLevel {
    INFO = 0,
    DEBUG = 1,
    TRACE = 2
};

class ErrorHandler {
private:
    static std::mutex logMutex;
    static std::ofstream logFile;
    static bool initialized;
    
    static void InitializeLogger();
    static std::string GetCurrentTime();
    static std::string ErrorLevelToString(ErrorLevel level);
    static std::string InfoLevelToString(InfoLevel level);
    static void WriteToFile(const std::string& message);

public:
    static void LogError(const std::string& message, ErrorLevel level = ErrorLevel::ERROR);
    static void LogInfo(const std::string& message, InfoLevel level = InfoLevel::INFO);
    static void ShowErrorDialog(const std::string& message, const std::string& title = "Hata");
    static void ShowInfoDialog(const std::string& message, const std::string& title = "Bilgi");
    static void Cleanup();
    
    // Windows API hata kodlarını string'e çevirme
    static std::string GetLastErrorAsString();
    static std::string HRESULTToString(HRESULT hr);
};