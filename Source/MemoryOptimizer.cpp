// Source/MemoryOptimizer.cpp
#include "MemoryOptimizer.h"
#include "framework.h"

MemoryOptimizer::MemoryOptimizer() 
    : currentMemoryUsage(0)
    , peakMemoryUsage(0)
    , memoryLimit(150 * 1024 * 1024)  // 150MB limit
    , isRunning(false) {
    // Otomatik temizleme thread'i başlat
    cleanupThread = std::make_unique<std::thread>(&MemoryOptimizer::CleanupLoop, this);
}

MemoryOptimizer::~MemoryOptimizer() {
    isRunning = false;
    if (cleanupThread) {
        cleanupThread->join();
    }
}

void MemoryOptimizer::AutoCleanup() {
    try {
        // Bellek kullanımını kontrol et
        size_t currentUsage = GetCurrentMemoryUsage();
        
        // Bellek limitini aşarsa temizlik yap
        if (currentUsage > memoryLimit) {
            ClearUnusedFrames();
            DynamicBufferResize();
        }
        
        // Peak bellek kullanımını güncelle
        peakMemoryUsage = std::max(peakMemoryUsage, currentUsage);
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Otomatik temizleme hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}

void MemoryOptimizer::ClearUnusedFrames() {
    try {
        // Tüm video oynatıcılarından kullanılmayan frame'leri temizle
        std::lock_guard<std::mutex> lock(mutex);
        
        // VideoPlayer ve VideoPreview nesnelerini kontrol et
        for (auto& player : VideoPlayer::GetAllInstances()) {
            player.ClearUnusedFrames();
        }
        
        // Bellek kullanımını güncelle
        currentMemoryUsage = GetCurrentMemoryUsage();
        
        // Gereksiz OpenCV bellek temizliği
        cv::Mat().release();
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Frame temizleme hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}

void MemoryOptimizer::DynamicBufferResize() {
    try {
        // Sistem bellek durumuna göre buffer boyutunu ayarla
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        
        double availableMemory = memInfo.ullAvailPhys / (1024.0 * 1024.0); // MB cinsinden
        
        // Bellek düşükse buffer boyutunu azalt
        if (availableMemory < 512.0) {  // 512MB altı
            VideoPlayer::SetMaxBufferFrames(2);  // Buffer boyutunu azalt
        }
        else if (availableMemory < 1024.0) {  // 1GB altı
            VideoPlayer::SetMaxBufferFrames(3);
        }
        else {
            VideoPlayer::SetMaxBufferFrames(5);  // Normal buffer boyutu
        }
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Buffer boyutu ayarlama hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}

size_t MemoryOptimizer::GetCurrentMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), 
                        (PROCESS_MEMORY_COUNTERS*)&pmc, 
                        sizeof(pmc));
    
    currentMemoryUsage = pmc.WorkingSetSize;
    return currentMemoryUsage;
}

void MemoryOptimizer::CleanupLoop() {
    isRunning = true;
    while (isRunning) {
        try {
            // Her 30 saniyede bir bellek kontrolü yap
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            // Bellek kullanımını monitor et
            MonitorMemoryUsage();
            
            // Gerektiğinde optimizasyon yap
            OptimizeMemoryAllocation();
        }
        catch (const std::exception& e) {
            ErrorHandler::LogError("Temizlik döngüsü hatası: " + std::string(e.what()),
                                 ErrorLevel::WARNING);
        }
    }
}

void MemoryOptimizer::MonitorMemoryUsage() {
    size_t currentUsage = GetCurrentMemoryUsage();
    
    // Bellek kullanımını logla
    if (currentUsage > peakMemoryUsage) {
        peakMemoryUsage = currentUsage;
        ErrorHandler::LogInfo("Peak bellek kullanımı: " + 
                            std::to_string(currentUsage / (1024 * 1024)) + " MB",
                            InfoLevel::DEBUG);
    }
}

void MemoryOptimizer::OptimizeMemoryAllocation() {
    try {
        // OpenCV bellek temizliği
        cv::Mat().release();
        
        // Windows API temizliği
        CoTaskMemFree(nullptr);
        
        // Çalışan thread sayısını kontrol et
        DWORD threadCount;
        GetProcessThreadCount(GetCurrentProcess(), &threadCount);
        
        if (threadCount > 10) {  // Çok fazla thread varsa
            // Gereksiz thread'leri sonlandır
            VideoPlayer::CleanupThreads();
        }
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Bellek optimizasyonu hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}