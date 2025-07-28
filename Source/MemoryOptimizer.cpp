// Source/MemoryOptimizer.cpp
#include "../Headers/MemoryOptimizer.h"
#include "../Headers/VideoPlayer.h"

MemoryOptimizer::MemoryOptimizer() 
    : currentMemoryUsage(0)
    , peakMemoryUsage(0)
    , memoryLimit(200 * 1024 * 1024) // 200MB default limit
    , isRunning(true) {
    
    // Başlangıç bellek kullanımını ölç
    MonitorMemoryUsage();
    
    // Cleanup thread'ini başlat
    cleanupThread = std::make_unique<std::thread>(&MemoryOptimizer::CleanupLoop, this);
    
    ErrorHandler::LogInfo("MemoryOptimizer başlatıldı", InfoLevel::INFO);
}

MemoryOptimizer::~MemoryOptimizer() {
    isRunning = false;
    
    if (cleanupThread && cleanupThread->joinable()) {
        cleanupThread->join();
    }
    
    ErrorHandler::LogInfo("MemoryOptimizer sonlandırıldı", InfoLevel::INFO);
}

void MemoryOptimizer::MonitorMemoryUsage() {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        size_t current = pmc.WorkingSetSize;
        currentMemoryUsage = current;
        
        if (current > peakMemoryUsage) {
            peakMemoryUsage = current;
        }
        
        // Bellek kullanımını log'la (sadece önemli değişiklikler)
        static size_t lastLogged = 0;
        if (current > lastLogged + (10 * 1024 * 1024) || current < lastLogged - (10 * 1024 * 1024)) {
            double memoryMB = current / (1024.0 * 1024.0);
            ErrorHandler::LogInfo("Bellek kullanımı: " + std::to_string(static_cast<int>(memoryMB)) + " MB", InfoLevel::TRACE);
            lastLogged = current;
        }
    }
}

size_t MemoryOptimizer::GetCurrentMemoryUsage() {
    MonitorMemoryUsage();
    return currentMemoryUsage;
}

void MemoryOptimizer::AutoCleanup() {
    MonitorMemoryUsage();
    
    if (currentMemoryUsage > memoryLimit) {
        ErrorHandler::LogInfo("Bellek limiti aşıldı, otomatik temizlik başlatılıyor", InfoLevel::WARNING);
        
        ClearUnusedFrames();
        DynamicBufferResize();
        OptimizeMemoryAllocation();
        
        // Sistem seviyesinde temizlik
        SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
        
        MonitorMemoryUsage();
        double memoryMB = currentMemoryUsage / (1024.0 * 1024.0);
        ErrorHandler::LogInfo("Temizlik sonrası bellek kullanımı: " + std::to_string(static_cast<int>(memoryMB)) + " MB", InfoLevel::INFO);
    }
}

void MemoryOptimizer::ClearUnusedFrames() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Tüm VideoPlayer instance'larındaki kullanılmayan frame'leri temizle
    auto& instances = VideoPlayer::GetAllInstances();
    for (auto* player : instances) {
        if (player) {
            player->ClearUnusedFrames();
        }
    }
    
    ErrorHandler::LogInfo("Kullanılmayan frame'ler temizlendi", InfoLevel::DEBUG);
}

void MemoryOptimizer::DynamicBufferResize() {
    MonitorMemoryUsage();
    
    // Bellek kullanımına göre buffer boyutunu ayarla
    if (currentMemoryUsage > memoryLimit * 0.8) { // %80'i aşınca
        VideoPlayer::SetMaxBufferFrames(2); // Buffer boyutunu küçült
        ErrorHandler::LogInfo("Buffer boyutu küçültüldü (2 frame)", InfoLevel::DEBUG);
    } else if (currentMemoryUsage < memoryLimit * 0.5) { // %50'nin altındaysa
        VideoPlayer::SetMaxBufferFrames(5); // Buffer boyutunu büyüt
        ErrorHandler::LogInfo("Buffer boyutu büyütüldü (5 frame)", InfoLevel::DEBUG);
    } else {
        VideoPlayer::SetMaxBufferFrames(3); // Normal boyut
    }
}

void MemoryOptimizer::OptimizeMemoryAllocation() {
    // Windows heap defragmentasyonu
    HANDLE hHeap = GetProcessHeap();
    if (hHeap) {
        HeapCompact(hHeap, 0);
    }
    
    // C++ heap'i optimize et
    #ifdef _MSC_VER
    _heapmin();
    #endif
    
    ErrorHandler::LogInfo("Bellek tahsisi optimize edildi", InfoLevel::DEBUG);
}

void MemoryOptimizer::CleanupLoop() {
    const auto cleanupInterval = std::chrono::seconds(10); // 10 saniyede bir kontrol
    
    ErrorHandler::LogInfo("MemoryOptimizer cleanup thread başladı", InfoLevel::DEBUG);
    
    while (isRunning) {
        try {
            AutoCleanup();
            
            std::this_thread::sleep_for(cleanupInterval);
            
        } catch (const std::exception& e) {
            ErrorHandler::LogError("MemoryOptimizer cleanup hatası: " + std::string(e.what()), ErrorLevel::ERROR);
            std::this_thread::sleep_for(std::chrono::seconds(30)); // Hata durumunda daha uzun bekle
        }
    }
    
    ErrorHandler::LogInfo("MemoryOptimizer cleanup thread sonlandı", InfoLevel::DEBUG);
}