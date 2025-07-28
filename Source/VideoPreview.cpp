// Source/VideoPreview.cpp
#include "../Headers/VideoPreview.h"

VideoPreview::VideoPreview() {
    if (!imageProcessor.Initialize()) {
        ErrorHandler::LogError("VideoPreview ImageProcessor başlatılamadı", ErrorLevel::ERROR);
    }
    
    ErrorHandler::LogInfo("VideoPreview oluşturuldu", InfoLevel::DEBUG);
}

VideoPreview::~VideoPreview() {
    ClearThumbnailCache();
    imageProcessor.Cleanup();
    ErrorHandler::LogInfo("VideoPreview yok edildi", InfoLevel::DEBUG);
}

bool VideoPreview::GenerateThumbnail(const std::wstring& videoPath, const std::wstring& outputPath) {
    if (videoPath.empty() || outputPath.empty()) {
        ErrorHandler::LogError("Geçersiz dosya yolları", ErrorLevel::ERROR);
        return false;
    }
    
    // Video format kontrolü
    if (!imageProcessor.IsSupportedVideoFormat(videoPath)) {
        ErrorHandler::LogError("Desteklenmeyen video formatı", ErrorLevel::ERROR);
        return false;
    }
    
    std::string videoPathStr(videoPath.begin(), videoPath.end());
    
    // Cache kontrolü
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (IsThumbnailCached(videoPathStr)) {
        ErrorHandler::LogInfo("Thumbnail cache'den alındı: " + videoPathStr, InfoLevel::DEBUG);
        return true;
    }
    
    // DirectShow ile thumbnail oluştur
    bool success = CreateThumbnailWithDirectShow(videoPath, outputPath);
    
    if (success) {
        // Cache'e ekle
        thumbnailCache[videoPathStr] = outputPath;
        UpdateCacheSize();
        
        ErrorHandler::LogInfo("Thumbnail oluşturuldu: " + videoPathStr, InfoLevel::INFO);
    } else {
        ErrorHandler::LogError("Thumbnail oluşturulamadı: " + videoPathStr, ErrorLevel::ERROR);
    }
    
    return success;
}

VideoPreview::VideoInfo VideoPreview::GetVideoInfo(const std::wstring& videoPath) {
    VideoInfo info;
    
    if (videoPath.empty()) {
        ErrorHandler::LogError("Boş video yolu", ErrorLevel::ERROR);
        return info;
    }
    
    // Dosya boyutunu al
    WIN32_FILE_ATTRIBUTE_DATA fileData;
    if (GetFileAttributesEx(videoPath.c_str(), GetFileExInfoStandard, &fileData)) {
        LARGE_INTEGER fileSize;
        fileSize.LowPart = fileData.nFileSizeLow;
        fileSize.HighPart = fileData.nFileSizeHigh;
        info.fileSize = static_cast<size_t>(fileSize.QuadPart);
    }
    
    // DirectShow ile video bilgilerini al
    IGraphBuilder* pGraphBuilder = nullptr;
    IBasicVideo* pBasicVideo = nullptr;
    IMediaPosition* pMediaPosition = nullptr;
    
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IGraphBuilder, (void**)&pGraphBuilder);
    if (SUCCEEDED(hr)) {
        hr = pGraphBuilder->RenderFile(videoPath.c_str(), nullptr);
        if (SUCCEEDED(hr)) {
            // Video boyutları
            hr = pGraphBuilder->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
            if (SUCCEEDED(hr)) {
                long width, height;
                if (SUCCEEDED(pBasicVideo->get_VideoWidth(&width)) && 
                    SUCCEEDED(pBasicVideo->get_VideoHeight(&height))) {
                    info.width = width;
                    info.height = height;
                }
                pBasicVideo->Release();
            }
            
            // Video süresi
            hr = pGraphBuilder->QueryInterface(IID_IMediaPosition, (void**)&pMediaPosition);
            if (SUCCEEDED(hr)) {
                double duration;
                if (SUCCEEDED(pMediaPosition->get_Duration(&duration))) {
                    info.duration = duration;
                }
                pMediaPosition->Release();
            }
        }
        pGraphBuilder->Release();
    }
    
    // FPS bilgisi (yaklaşık)
    if (info.duration > 0) {
        info.fps = 25.0; // Default 25 FPS, gerçek implementasyonda hesaplanabilir
    }
    
    // Codec bilgisi (placeholder)
    info.codec = "H.264"; // Gerçek implementasyonda detect edilecek
    
    std::string videoPathStr(videoPath.begin(), videoPath.end());
    ErrorHandler::LogInfo("Video bilgileri alındı: " + videoPathStr, InfoLevel::DEBUG);
    
    return info;
}

void VideoPreview::ClearThumbnailCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Cache'deki thumbnail dosyalarını sil
    for (const auto& pair : thumbnailCache) {
        DeleteFile(pair.second.c_str());
    }
    
    thumbnailCache.clear();
    ErrorHandler::LogInfo("Thumbnail cache temizlendi", InfoLevel::DEBUG);
}

bool VideoPreview::IsThumbnailCached(const std::string& videoPath) {
    auto it = thumbnailCache.find(videoPath);
    if (it != thumbnailCache.end()) {
        // Dosyanın hala var olup olmadığını kontrol et
        if (GetFileAttributes(it->second.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return true;
        } else {
            // Dosya silinmişse cache'den kaldır
            thumbnailCache.erase(it);
        }
    }
    return false;
}

bool VideoPreview::CreateThumbnailWithDirectShow(const std::wstring& videoPath, const std::wstring& outputPath) {
    IGraphBuilder* pGraphBuilder = nullptr;
    IMediaControl* pMediaControl = nullptr;
    IBasicVideo* pBasicVideo = nullptr;
    IVideoWindow* pVideoWindow = nullptr;
    
    HRESULT hr = S_OK;
    bool success = false;
    
    try {
        // Graph builder oluştur
        hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                             IID_IGraphBuilder, (void**)&pGraphBuilder);
        if (FAILED(hr)) throw std::runtime_error("GraphBuilder oluşturulamadı");
        
        // Video dosyasını yükle
        hr = pGraphBuilder->RenderFile(videoPath.c_str(), nullptr);
        if (FAILED(hr)) throw std::runtime_error("Video dosyası yüklenemedi");
        
        // Interface'leri al
        hr = pGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&pMediaControl);
        if (FAILED(hr)) throw std::runtime_error("MediaControl alınamadı");
        
        hr = pGraphBuilder->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
        if (FAILED(hr)) throw std::runtime_error("BasicVideo alınamadı");
        
        hr = pGraphBuilder->QueryInterface(IID_IVideoWindow, (void**)&pVideoWindow);
        if (FAILED(hr)) throw std::runtime_error("VideoWindow alınamadı");
        
        // Gizli pencere oluştur (thumbnail için)
        HWND hWndDummy = CreateWindow(L"STATIC", L"", WS_POPUP,
                                     -1000, -1000, THUMBNAIL_SIZE, THUMBNAIL_SIZE,
                                     nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!hWndDummy) throw std::runtime_error("Dummy pencere oluşturulamadı");
        
        // Video penceresini ayarla
        pVideoWindow->put_Owner((OAHWND)hWndDummy);
        pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
        pVideoWindow->SetWindowPosition(0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
        
        // Video oynatmayı başlat
        hr = pMediaControl->Run();
        if (FAILED(hr)) throw std::runtime_error("Video oynatılamadı");
        
        // Biraz bekle (ilk frame için)
        Sleep(500);
        
        // Screenshot al
        HDC hDC = GetDC(hWndDummy);
        if (hDC) {
            HDC hMemDC = CreateCompatibleDC(hDC);
            HBITMAP hBitmap = CreateCompatibleBitmap(hDC, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);
            
            // Pencere içeriğini kopyala
            BitBlt(hMemDC, 0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE, hDC, 0, 0, SRCCOPY);
            
            // Bitmap'i dosyaya kaydet (basit BMP formatında)
            success = SaveBitmapToFile(hBitmap, outputPath);
            
            // Cleanup
            SelectObject(hMemDC, hOldBitmap);
            DeleteObject(hBitmap);
            DeleteDC(hMemDC);
            ReleaseDC(hWndDummy, hDC);
        }
        
        // Video oynatmayı durdur
        pMediaControl->Stop();
        
        // Dummy pencereyi kapat
        DestroyWindow(hWndDummy);
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Thumbnail oluşturma hatası: " + std::string(e.what()), ErrorLevel::ERROR);
        success = false;
    }
    
    // Cleanup
    if (pVideoWindow) pVideoWindow->Release();
    if (pBasicVideo) pBasicVideo->Release();
    if (pMediaControl) pMediaControl->Release();
    if (pGraphBuilder) pGraphBuilder->Release();
    
    return success;
}

bool VideoPreview::SaveBitmapToFile(HBITMAP hBitmap, const std::wstring& filePath) {
    if (!hBitmap || filePath.empty()) return false;
    
    // GDI+ kullanarak bitmap kaydet
    Gdiplus::Bitmap bitmap(hBitmap, nullptr);
    
    // JPEG encoder'ı bul
    CLSID jpegClsid;
    if (GetEncoderClsid(L"image/jpeg", &jpegClsid) < 0) {
        ErrorHandler::LogError("JPEG encoder bulunamadı", ErrorLevel::ERROR);
        return false;
    }
    
    // Kalite ayarları
    Gdiplus::EncoderParameters encoderParams;
    encoderParams.Count = 1;
    encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
    encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
    encoderParams.Parameter[0].NumberOfValues = 1;
    ULONG quality = 85; // %85 kalite
    encoderParams.Parameter[0].Value = &quality;
    
    // Dosyayı kaydet
    Gdiplus::Status status = bitmap.Save(filePath.c_str(), &jpegClsid, &encoderParams);
    
    return status == Gdiplus::Ok;
}

int VideoPreview::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;
    
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == nullptr) return -1;
    
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    
    free(pImageCodecInfo);
    return -1;
}

void VideoPreview::UpdateCacheSize() {
    if (thumbnailCache.size() > MAX_CACHE_SIZE) {
        // En eski cache girişlerini sil
        auto it = thumbnailCache.begin();
        while (thumbnailCache.size() > MAX_CACHE_SIZE && it != thumbnailCache.end()) {
            DeleteFile(it->second.c_str());
            it = thumbnailCache.erase(it);
        }
        
        ErrorHandler::LogInfo("Thumbnail cache boyutu optimize edildi", InfoLevel::DEBUG);
    }
}

void VideoPreview::OptimizeMemoryUsage() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Kullanılmayan thumbnail'leri temizle
    auto it = thumbnailCache.begin();
    while (it != thumbnailCache.end()) {
        // Dosya 1 saatten eskiyse sil
        WIN32_FILE_ATTRIBUTE_DATA fileData;
        if (GetFileAttributesEx(it->second.c_str(), GetFileExInfoStandard, &fileData)) {
            FILETIME currentTime;
            GetSystemTimeAsFileTime(&currentTime);
            
            ULARGE_INTEGER current, created;
            current.LowPart = currentTime.dwLowDateTime;
            current.HighPart = currentTime.dwHighDateTime;
            created.LowPart = fileData.ftCreationTime.dwLowDateTime;
            created.HighPart = fileData.ftCreationTime.dwHighDateTime;
            
            // 1 saat = 10,000,000 * 3600 (100-nanosecond intervals)
            if (current.QuadPart - created.QuadPart > 36000000000ULL) {
                DeleteFile(it->second.c_str());
                it = thumbnailCache.erase(it);
                continue;
            }
        }
        ++it;
    }
    
    ErrorHandler::LogInfo("VideoPreview bellek kullanımı optimize edildi", InfoLevel::DEBUG);
}