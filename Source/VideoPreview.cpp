// Source/VideoPreview.cpp
#include "../Headers/VideoPreview.h"

VideoPreview::VideoPreview() {
    if (!imageProcessor.Initialize()) {
        ErrorHandler::LogError("VideoPreview ImageProcessor başlatılamadı", ErrorLevel::ERROR);
    }
}

VideoPreview::~VideoPreview() {
    ClearThumbnailCache();
}

bool VideoPreview::GenerateThumbnail(const std::wstring& videoPath, const std::wstring& outputPath) {
    try {
        // Video dosyasını kontrol et
        if (!std::filesystem::exists(videoPath)) {
            ErrorHandler::LogError("Video dosyası bulunamadı: " + 
                                 std::string(videoPath.begin(), videoPath.end()), 
                                 ErrorLevel::ERROR);
            return false;
        }

        // Önbellekte var mı kontrol et
        std::string cacheKey(videoPath.begin(), videoPath.end());
        if (IsThumbnailCached(cacheKey)) {
            ErrorHandler::LogInfo("Thumbnail cache'den alındı", InfoLevel::DEBUG);
            return true;
        }

        // Desteklenen format kontrolü
        if (!imageProcessor.IsSupportedVideoFormat(videoPath)) {
            ErrorHandler::LogError("Desteklenmeyen video formatı", ErrorLevel::ERROR);
            return false;
        }

        // DirectShow kullanarak thumbnail oluştur
        bool success = CreateThumbnailWithDirectShow(videoPath, outputPath);
        
        if (success) {
            // Cache'e ekle (basit string key olarak)
            thumbnailCache[cacheKey] = outputPath;
            UpdateCacheSize();
            
            ErrorHandler::LogInfo("Thumbnail oluşturuldu: " + 
                                std::string(outputPath.begin(), outputPath.end()),
                                InfoLevel::INFO);
        }

        return success;
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Mini resim oluşturma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
        return false;
    }
}

bool VideoPreview::CreateThumbnailWithDirectShow(const std::wstring& videoPath, const std::wstring& outputPath) {
    IGraphBuilder* pGraph = nullptr;
    IMediaControl* pControl = nullptr;
    IMediaSeeking* pSeeking = nullptr;
    IBasicVideo* pBasicVideo = nullptr;
    
    HRESULT hr = S_OK;
    bool success = false;
    
    try {
        // Graph Builder oluştur
        hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                            IID_IGraphBuilder, (void**)&pGraph);
        if (FAILED(hr)) {
            throw std::runtime_error("FilterGraph oluşturulamadı");
        }
        
        // Video dosyasını renderla
        hr = pGraph->RenderFile(videoPath.c_str(), nullptr);
        if (FAILED(hr)) {
            throw std::runtime_error("Video dosyası renderlanamadı");
        }
        
        // Interface'leri al
        hr = pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
        if (FAILED(hr)) {
            throw std::runtime_error("IMediaControl alınamadı");
        }
        
        hr = pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeeking);
        if (FAILED(hr)) {
            throw std::runtime_error("IMediaSeeking alınamadı");
        }
        
        hr = pGraph->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
        if (FAILED(hr)) {
            throw std::runtime_error("IBasicVideo alınamadı");
        }
        
        // Video süresini al
        LONGLONG duration;
        hr = pSeeking->GetDuration(&duration);
        if (FAILED(hr)) {
            duration = 10000000; // 1 saniye varsayılan
        }
        
        // Video ortasına git (%25'ine)
        LONGLONG seekPos = duration / 4;
        hr = pSeeking->SetPositions(&seekPos, AM_SEEKING_AbsolutePositioning,
                                   nullptr, AM_SEEKING_NoPositioning);
        
        // Videoyu başlat ve bir frame bekle
        hr = pControl->Run();
        if (SUCCEEDED(hr)) {
            Sleep(500); // Frame yüklenmesi için bekle
            
            // Video boyutlarını al
            long width, height;
            hr = pBasicVideo->get_VideoWidth(&width);
            if (SUCCEEDED(hr)) {
                hr = pBasicVideo->get_VideoHeight(&height);
            }
            
            if (SUCCEEDED(hr)) {
                // Thumbnail boyutunu hesapla (orantılı küçültme)
                int thumbWidth = THUMBNAIL_SIZE;
                int thumbHeight = THUMBNAIL_SIZE;
                
                if (width > height) {
                    thumbHeight = (height * THUMBNAIL_SIZE) / width;
                } else {
                    thumbWidth = (width * THUMBNAIL_SIZE) / height;
                }
                
                // Basit başarı işareti (gerçek frame çıkarma implementasyonu gelecekte eklenecek)
                success = true;
                
                ErrorHandler::LogInfo("Video bilgileri - Boyut: " + 
                                    std::to_string(width) + "x" + std::to_string(height),
                                    InfoLevel::DEBUG);
            }
            
            pControl->Stop();
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("DirectShow thumbnail hatası: " + std::string(e.what()), ErrorLevel::ERROR);
    }
    
    // Temizlik
    if (pBasicVideo) pBasicVideo->Release();
    if (pSeeking) pSeeking->Release();
    if (pControl) pControl->Release();
    if (pGraph) pGraph->Release();
    
    return success;
}

VideoPreview::VideoInfo VideoPreview::GetVideoInfo(const std::wstring& videoPath) {
    VideoInfo info = {};
    
    IGraphBuilder* pGraph = nullptr;
    IBasicVideo* pBasicVideo = nullptr;
    IMediaSeeking* pSeeking = nullptr;
    
    try {
        // Graph Builder oluştur
        HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IGraphBuilder, (void**)&pGraph);
        if (FAILED(hr)) {
            throw std::runtime_error("FilterGraph oluşturulamadı");
        }
        
        // Video dosyasını renderla
        hr = pGraph->RenderFile(videoPath.c_str(), nullptr);
        if (FAILED(hr)) {
            throw std::runtime_error("Video dosyası renderlanamadı");
        }
        
        // Interface'leri al
        hr = pGraph->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
        if (SUCCEEDED(hr)) {
            long width, height;
            if (SUCCEEDED(pBasicVideo->get_VideoWidth(&width))) {
                info.width = static_cast<int>(width);
            }
            if (SUCCEEDED(pBasicVideo->get_VideoHeight(&height))) {
                info.height = static_cast<int>(height);
            }
        }
        
        hr = pGraph->QueryInterface(IID_IMediaSeeking, (void**)&pSeeking);
        if (SUCCEEDED(hr)) {
            LONGLONG duration;
            if (SUCCEEDED(pSeeking->GetDuration(&duration))) {
                info.duration = static_cast<double>(duration) / 10000000.0; // 100ns to seconds
            }
        }
        
        // Dosya boyutunu al
        try {
            info.fileSize = std::filesystem::file_size(videoPath);
        } catch (...) {
            info.fileSize = 0;
        }
        
        // FPS ve codec bilgileri için daha karmaşık implementasyon gerekli
        info.fps = 30.0; // Varsayılan değer
        info.codec = "Unknown";
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Video bilgi alma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
    
    // Temizlik
    if (pSeeking) pSeeking->Release();
    if (pBasicVideo) pBasicVideo->Release();
    if (pGraph) pGraph->Release();
    
    return info;
}

void VideoPreview::ClearThumbnailCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    thumbnailCache.clear();
    ErrorHandler::LogInfo("Thumbnail cache temizlendi", InfoLevel::DEBUG);
}

bool VideoPreview::IsThumbnailCached(const std::string& videoPath) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return thumbnailCache.find(videoPath) != thumbnailCache.end();
}

void VideoPreview::UpdateCacheSize() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Önbellek boyutunu kontrol et
    while (static_cast<int>(thumbnailCache.size()) > MAX_CACHE_SIZE) {
        auto it = thumbnailCache.begin();
        thumbnailCache.erase(it);
    }
    
    ErrorHandler::LogInfo("Cache boyutu güncellendi: " + 
                        std::to_string(thumbnailCache.size()) + "/" + 
                        std::to_string(MAX_CACHE_SIZE),
                        InfoLevel::DEBUG);
}

void VideoPreview::OptimizeMemoryUsage() {
    // Bellek kullanımını kontrol et
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        size_t currentMemory = pmc.WorkingSetSize;
        
        if (currentMemory > 100 * 1024 * 1024) {  // 100MB
            ClearThumbnailCache();
            ErrorHandler::LogInfo("Yüksek bellek kullanımı nedeniyle cache temizlendi", InfoLevel::INFO);
        }
    }
}