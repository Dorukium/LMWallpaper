#pragma once
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

class Logger {
public:
    static void Log(const std::string& message) {
        // Log dosyasını "append" modunda (sonuna ekle) açıyoruz.
        // Dosya, .exe'nin yanında oluşacak.
        std::ofstream log_file("lmwallpaper_log.txt", std::ios_base::app);
        if (log_file.is_open()) {
            log_file << GetTimestamp() << " | " << message << std::endl;
        }
    }

private:
    static std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }
};