// Source/MemoryOptimizer.cpp
#include "../Headers/MemoryOptimizer.h"
#include "../Headers/VideoPlayer.h"

MemoryOptimizer::MemoryOptimizer() 
    : currentMemoryUsage(0)
    , peakMemoryUsage(0)
    , memoryLimit(150 * 1024 * 1024)  // 150MB limit
    , isRunning(false) {
    
    // Otomatik temizleme thread'i başlat
    isRunning = true;
    cleanupThread = std::make_unique<std::thread>(&MemoryOptimizer::CleanupLoop, this);
}

MemoryOptimizer::~MemoryOptimizer() {
    isRunning = false;
    if (cleanupThread && cleanupThread->joinable()) {
        cleanupThread->join();
    }
}

void MemoryOptimizer::AutoCleanup() {
    try {
        // Bellek kullanımını kontrol et
        size_t currentUsage = GetCurrentMemoryUsage();
        
        // Bellek limitini aşarsa temizlik yap
        if (currentUsage > memoryLimit) {
            ErrorHandler::LogInfo("Bellek limiti aşıldı, temizlik yapılıyor: " + 
                                std::to_string(currentUsage / (1024 * 1024)) + " MB", 
                                InfoLevel::DEBUG);
            
            ClearUnusedFrames();
            DynamicBufferResize();
            
            // Temizlik sonrası kontrol
            currentUsage = GetCurrentMemoryUsage();
            ErrorHandler::LogInfo("Temizlik sonrası bellek kullanımı: " + 
                                std::to_string(currentUsage / (1024 * 1024)) + " MB", 
                                InfoLevel::DEBUG);
        }
        
        // Peak bellek kullanımını güncelle
        if (currentUsage > peakMemoryUsage) {
            peakMemoryUsage = currentUsage;
        }
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Otomatik temizleme hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}

void MemoryOptimizer::ClearUnusedFrames() {
    try {
        std::lock_guard<std::mutex> lock(mutex);
        
        // Tüm video oynatıcılarından kullanılmayan frame'leri temizle
        auto& instances = VideoPlayer::GetAllInstances();
        for (auto* player : instances) {
            if (player) {
                player->ClearUnusedFrames();
            }
        }
        
        // Bellek kullanımını güncelle
        currentMemoryUsage = GetCurrentMemoryUsage();
        
        // Windows bellek temizliği
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        
        ErrorHandler::LogInfo("Frame temizliği tamamlandı", InfoLevel::DEBUG);
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
        if (!GlobalMemoryStatusEx(&memInfo)) {
            ErrorHandler::LogError("Sistem bellek bilgisi alınamadı", ErrorLevel::WARNING);
            return;
        }
        
        double availableMemoryMB = memInfo.ullAvailPhys / (1024.0 * 1024.0);
        double memoryUsagePercent = (double)(memInfo.ullTotalPhys - memInfo.ullAvailPhys) * 100.0 / memInfo.ullTotalPhys;
        
        // Bellek kullanım yüzdesine göre buffer boyutunu ayarla
        int newBufferSize;
        if (memoryUsagePercent > 85.0) {
            newBufferSize = 1;  // Çok yüksek kullanım - minimum buffer
            ErrorHandler::LogInfo("Sistem belleği kritik seviyede (%85+), buffer boyutu minimize edildi", InfoLevel::WARNING);
        }
        else if (memoryUsagePercent > 70.0) {
            newBufferSize = 2;  // Yüksek kullanım - küçük buffer
            ErrorHandler::LogInfo("Sistem belleği yüksek seviyede (%70+), buffer boyutu azaltıldı", InfoLevel::INFO);
        }
        else if (availableMemoryMB < 512.0) {
            newBufferSize = 2;  // 512MB altı - küçük buffer
        }
        else if (availableMemoryMB < 1024.0) {
            newBufferSize = 3;  // 1GB altı - orta buffer
        }
        else {
            newBufferSize = 5;  // Normal buffer boyutu
        }
        
        VideoPlayer::SetMaxBufferFrames(newBufferSize);
        
        ErrorHandler::LogInfo("Buffer boyutu ayarlandı: " + std::to_string(newBufferSize) + 
                            " (Kullanılabilir bellek: " + std::to_string((int)availableMemoryMB) + " MB)", 
                            InfoLevel::DEBUG);
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Buffer boyutu ayarlama hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}

size_t MemoryOptimizer::GetCurrentMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), 
                            (PROCESS_MEMORY_COUNTERS*)&pmc, 
                            sizeof(pmc))) {
        currentMemoryUsage = pmc.WorkingSetSize;
        return currentMemoryUsage;
    }
    
    ErrorHandler::LogError("Bellek kullanım bilgisi alınamadı", ErrorLevel::WARNING);
    return 0;
}

void MemoryOptimizer::CleanupLoop() {
    ErrorHandler::LogInfo("Bellek temizleme döngüsü başlatıldı", InfoLevel::DEBUG);
    
    while (isRunning) {
        try {
            // Her 30 saniyede bir bellek kontrolü yap
            for (int i = 0; i < 30 && isRunning; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            if (!isRunning) break;
            
            // Bellek kullanımını monitor et
            MonitorMemoryUsage();
            
            // Gerektiğinde optimizasyon yap
            OptimizeMemoryAllocation();
            
            // Otomatik temizlik
            AutoCleanup();
        }
        catch (const std::exception& e) {
            ErrorHandler::LogError("Temizlik döngüsü hatası: " + std::string(e.what()),
                                 ErrorLevel::WARNING);
        }
    }
    
    ErrorHandler::LogInfo("Bellek temizleme döngüsü sonlandırıldı", InfoLevel::DEBUG);
}

void MemoryOptimizer::MonitorMemoryUsage() {
    size_t currentUsage = GetCurrentMemoryUsage();
    
    // Bellek kullanımını logla (sadece önemli değişikliklerde)
    static size_t lastLoggedUsage = 0;
    size_t usageDifference = currentUsage > lastLoggedUsage ? 
                            currentUsage - lastLoggedUsage : 
                            lastLoggedUsage - currentUsage;
    
    // 10MB'dan fazla değişiklik varsa logla
    if (usageDifference > 10 * 1024 * 1024) {
        ErrorHandler::LogInfo("Bellek kullanımı: " + 
                            std::to_string(currentUsage / (1024 * 1024)) + " MB",
                            InfoLevel::DEBUG);
        lastLoggedUsage = currentUsage;
    }
    
    // Peak değeri güncelle
    if (currentUsage > peakMemoryUsage) {
        peakMemoryUsage = currentUsage;
        ErrorHandler::LogInfo("Yeni peak bellek kullanımı: " + 
                            std::to_string(currentUsage / (1024 * 1024)) + " MB",
                            InfoLevel::INFO);
    }
    
    // Kritik seviye kontrolü
    if (currentUsage > memoryLimit * 1.5) {  // %150 limit aşımı
        ErrorHandler::LogError("Kritik bellek kullanımı tespit edildi: " + 
                             std::to_string(currentUsage / (1024 * 1024)) + " MB",
                             ErrorLevel::WARNING);
        
        // Acil temizlik yap
        ClearUnusedFrames();
        DynamicBufferResize();
    }
}

void MemoryOptimizer::OptimizeMemoryAllocation() {
    try {
        // Windows'un çalışan küme boyutunu optimize et
        SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
        
        // COM bellek temizliği
        CoFreeUnusedLibraries();
        
        // Heap compaction (Windows 10+)
        HANDLE hHeap = GetProcessHeap();
        if (hHeap) {
            HeapCompact(hHeap, 0);
        }
        
        // GDI nesnelerini temizle
        GdiFlush();
        
        ErrorHandler::LogInfo("Bellek optimizasyonu tamamlandı", InfoLevel::DEBUG);
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Bellek optimizasyonu hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}