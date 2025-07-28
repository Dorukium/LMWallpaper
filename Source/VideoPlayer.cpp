// Source/VideoPlayer.cpp
#include "../Headers/VideoPlayer.h"

// Static members
std::vector<VideoPlayer*> VideoPlayer::allInstances;
int VideoPlayer::maxBufferFrames = 3;

VideoPlayer::VideoPlayer(HMONITOR monitorHandle) 
    : monitorHandle(monitorHandle)
    , isPlaying(false)
    , pGraphBuilder(nullptr)
    , pMediaControl(nullptr)
    , pVideoWindow(nullptr)
    , pMediaEvent(nullptr)
    , pBasicVideo(nullptr)
    , targetWindow(nullptr)
    , shouldStop(false) {
    
    // Instance'ı listeye ekle
    allInstances.push_back(this);
    
    if (!InitializeGraphBuilder()) {
        ErrorHandler::LogError("VideoPlayer başlatılamadı", ErrorLevel::ERROR);
    }
    
    if (!imageProcessor.Initialize()) {
        ErrorHandler::LogError("ImageProcessor başlatılamadı", ErrorLevel::ERROR);
    }
}

VideoPlayer::~VideoPlayer() {
    Cleanup();
    
    // Instance'ı listeden kaldır
    auto it = std::find(allInstances.begin(), allInstances.end(), this);
    if (it != allInstances.end()) {
        allInstances.erase(it);
    }
}

bool VideoPlayer::InitializeGraphBuilder() {
    HRESULT hr = S_OK;
    
    try {
        // DirectShow graf builder oluşturma
        hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                            IID_IGraphBuilder, (void**)&pGraphBuilder);
        
        if (FAILED(hr)) {
            throw std::runtime_error("FilterGraph oluşturulamadı");
        }
        
        // Media kontrol ve event nesneleri al
        hr = pGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&pMediaControl);
        if (FAILED(hr)) {
            throw std::runtime_error("IMediaControl alınamadı");
        }
        
        hr = pGraphBuilder->QueryInterface(IID_IMediaEvent, (void**)&pMediaEvent);
        if (FAILED(hr)) {
            throw std::runtime_error("IMediaEvent alınamadı");
        }
        
        hr = pGraphBuilder->QueryInterface(IID_IVideoWindow, (void**)&pVideoWindow);
        if (FAILED(hr)) {
            ErrorHandler::LogError("IVideoWindow alınamadı (video olmayabilir)", ErrorLevel::WARNING);
        }
        
        hr = pGraphBuilder->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
        if (FAILED(hr)) {
            ErrorHandler::LogError("IBasicVideo alınamadı (video olmayabilir)", ErrorLevel::WARNING);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("DirectShow başlatma hatası: " + std::string(e.what()),
                             ErrorLevel::CRITICAL);
        return false;
    }
}

bool VideoPlayer::LoadVideo(const std::wstring& videoPath) {
    try {
        // Video dosyasını kontrol et
        if (!std::filesystem::exists(videoPath)) {
            ErrorHandler::LogError("Video dosyası bulunamadı: " + 
                                 std::string(videoPath.begin(), videoPath.end()),
                                 ErrorLevel::ERROR);
            return false;
        }
        
        // Desteklenen format kontrolü
        if (!imageProcessor.IsSupportedVideoFormat(videoPath)) {
            ErrorHandler::LogError("Desteklenmeyen video formatı", ErrorLevel::ERROR);
            return false;
        }

        // Mevcut video varsa durdur
        if (isPlaying) {
            Stop();
        }
        
        // Graf temizle
        if (pGraphBuilder) {
            IEnumFilters* pEnum = nullptr;
            pGraphBuilder->EnumFilters(&pEnum);
            if (pEnum) {
                IBaseFilter* pFilter = nullptr;
                while (pEnum->Next(1, &pFilter, nullptr) == S_OK) {
                    pGraphBuilder->RemoveFilter(pFilter);
                    pFilter->Release();
                    pEnum->Reset(); // Enum'u sıfırla çünkü liste değişti
                }
                pEnum->Release();
            }
        }
        
        // Yeni video dosyasını yükle
        HRESULT hr = BuildGraph(videoPath);
        if (FAILED(hr)) {
            ErrorHandler::LogError("Video graf oluşturulamadı", ErrorLevel::ERROR);
            return false;
        }
        
        // Video penceresini yapılandır
        ConfigureVideoWindow();
        
        // Bellek optimizasyonu için buffer'ı temizle
        ClearUnusedFrames();
        
        currentVideoPath = videoPath;
        
        ErrorHandler::LogInfo("Video yüklendi: " + 
                            std::string(videoPath.begin(), videoPath.end()),
                            InfoLevel::INFO);
        
        return true;
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Video yükleme hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
        return false;
    }
}

HRESULT VideoPlayer::BuildGraph(const std::wstring& videoPath) {
    if (!pGraphBuilder) {
        return E_FAIL;
    }
    
    // RenderFile kullanarak otomatik graf oluştur
    HRESULT hr = pGraphBuilder->RenderFile(videoPath.c_str(), nullptr);
    
    if (FAILED(hr)) {
        ErrorHandler::LogError("Video dosyası render edilemedi: HRESULT = 0x" + 
                             std::to_string(hr), ErrorLevel::ERROR);
        return hr;
    }
    
    return hr;
}

void VideoPlayer::ConfigureVideoWindow() {
    if (!pVideoWindow || !targetWindow) {
        return;
    }
    
    try {
        // Video penceresini hedef pencereye bağla
        pVideoWindow->put_Owner((OAHWND)targetWindow);
        pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
        
        // Pencere boyutunu al
        RECT rect;
        GetClientRect(targetWindow, &rect);
        
        // Video boyutunu ayarla
        pVideoWindow->SetWindowPosition(0, 0, rect.right, rect.bottom);
        pVideoWindow->put_Visible(OATRUE);
        
        ErrorHandler::LogInfo("Video penceresi yapılandırıldı", InfoLevel::DEBUG);
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Video penceresi yapılandırma hatası: " + std::string(e.what()),
                             ErrorLevel::WARNING);
    }
}

void VideoPlayer::SetTargetWindow(HWND hWnd) {
    targetWindow = hWnd;
    ConfigureVideoWindow();
}

void VideoPlayer::Play() {
    try {
        if (!pMediaControl) {
            ErrorHandler::LogError("Media kontrol nesnesi bulunamadı", ErrorLevel::ERROR);
            return;
        }

        HRESULT hr = pMediaControl->Run();
        if (FAILED(hr)) {
            ErrorHandler::LogError("Video oynatma başlatılamadı: HRESULT = 0x" + 
                                 std::to_string(hr), ErrorLevel::ERROR);
            return;
        }
        
        isPlaying = true;
        shouldStop = false;
        StartVideoProcessingThread();
        
        ErrorHandler::LogInfo("Video oynatma başlatıldı", InfoLevel::INFO);
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Video oynatma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::Stop() {
    try {
        shouldStop = true;
        isPlaying = false;
        
        if (pMediaControl) {
            pMediaControl->Stop();
        }
        
        // Video thread'ini bekle
        if (videoThread && videoThread->joinable()) {
            videoThread->join();
            videoThread.reset();
        }
        
        ClearUnusedFrames();
        
        ErrorHandler::LogInfo("Video oynatma durduruldu", InfoLevel::INFO);
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Video durdurma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::TogglePlayback() {
    if (isPlaying) {
        Stop();
    } else {
        Play();
    }
}

void VideoPlayer::StartVideoProcessingThread() {
    if (videoThread && videoThread->joinable()) {
        shouldStop = true;
        videoThread->join();
    }
    
    shouldStop = false;
    videoThread = std::make_unique<std::thread>(&VideoPlayer::VideoProcessingLoop, this);
}

void VideoPlayer::VideoProcessingLoop() {
    ErrorHandler::LogInfo("Video işleme döngüsü başlatıldı", InfoLevel::DEBUG);
    
    while (!shouldStop && isPlaying) {
        try {
            ProcessVideoFrame();
            
            // CPU kullanımını optimize et (60 FPS için ~16ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            
        } catch (const std::exception& e) {
            ErrorHandler::LogError("Video işleme döngüsü hatası: " + std::string(e.what()),
                                 ErrorLevel::ERROR);
            break;
        }
    }
    
    ErrorHandler::LogInfo("Video işleme döngüsü sonlandırıldı", InfoLevel::DEBUG);
}

void VideoPlayer::ProcessVideoFrame() {
    try {
        std::lock_guard<std::mutex> lock(bufferMutex);
        
        // Buffer boyutunu kontrol et
        while (static_cast<int>(frameBuffer.size()) >= maxBufferFrames) {
            frameBuffer.pop_front();
        }
        
        // Yeni frame verisi oluştur
        auto frameData = std::make_unique<FrameData>();
        frameData->timestamp = GetTickCount();
        
        // Frame'i buffer'a ekle (şimdilik boş frame)
        frameBuffer.push_back(std::move(frameData));
        
        // Bellek kullanımını kontrol et
        static int frameCounter = 0;
        if (++frameCounter % 60 == 0) {  // Her saniyede bir kontrol
            // Bellek optimizasyon sinyali gönder
            // MemoryOptimizer otomatik olarak çalışacak
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Frame işleme hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::ClearUnusedFrames() {
    try {
        std::lock_guard<std::mutex> lock(bufferMutex);
        
        // Tüm frame'leri temizle
        frameBuffer.clear();
        
        ErrorHandler::LogInfo("Frame buffer temizlendi", InfoLevel::DEBUG);
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Frame temizleme hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::Cleanup() {
    try {
        Stop();
        
        // DirectShow nesnelerini temizle
        if (pBasicVideo) {
            pBasicVideo->Release();
            pBasicVideo = nullptr;
        }
        
        if (pVideoWindow) {
            pVideoWindow->put_Visible(OAFALSE);
            pVideoWindow->put_Owner(NULL);
            pVideoWindow->Release();
            pVideoWindow = nullptr;
        }
        
        if (pMediaEvent) {
            pMediaEvent->Release();
            pMediaEvent = nullptr;
        }
        
        if (pMediaControl) {
            pMediaControl->Release();
            pMediaControl = nullptr;
        }
        
        if (pGraphBuilder) {
            pGraphBuilder->Release();
            pGraphBuilder = nullptr;
        }
        
        frameBuffer.clear();
        
        ErrorHandler::LogInfo("VideoPlayer temizlendi", InfoLevel::DEBUG);
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("VideoPlayer temizlik hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::CleanupThreads() {
    // Tüm instance'ların thread'lerini temizle
    for (auto* player : allInstances) {
        if (player && player->videoThread && player->videoThread->joinable()) {
            player->shouldStop = true;
            player->videoThread->join();
            player->videoThread.reset();
        }
    }
    
    ErrorHandler::LogInfo("Tüm video thread'leri temizlendi", InfoLevel::DEBUG);
}