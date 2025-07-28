// Source/VideoPlayer.cpp
#include "../Headers/VideoPlayer.h"

std::vector<VideoPlayer*> VideoPlayer::allInstances;
int VideoPlayer::maxBufferFrames = 3;

VideoPlayer::VideoPlayer(HMONITOR hMonitor) 
    : pGraphBuilder(nullptr)
    , pMediaControl(nullptr)
    , pVideoWindow(nullptr)
    , pMediaEvent(nullptr)
    , pBasicVideo(nullptr)
    , isPlaying(false)
    , monitorHandle(hMonitor)
    , targetWindow(nullptr)
    , shouldStop(false) {
    
    allInstances.push_back(this);
    
    if (!imageProcessor.Initialize()) {
        ErrorHandler::LogError("ImageProcessor başlatılamadı", ErrorLevel::ERROR);
    }
    
    ErrorHandler::LogInfo("VideoPlayer oluşturuldu", InfoLevel::DEBUG);
}

VideoPlayer::~VideoPlayer() {
    shouldStop = true;
    
    if (videoThread && videoThread->joinable()) {
        videoThread->join();
    }
    
    Cleanup();
    
    // Instance'ı listeden kaldır
    auto it = std::find(allInstances.begin(), allInstances.end(), this);
    if (it != allInstances.end()) {
        allInstances.erase(it);
    }
    
    ErrorHandler::LogInfo("VideoPlayer yok edildi", InfoLevel::DEBUG);
}

bool VideoPlayer::LoadVideo(const std::wstring& videoPath) {
    if (videoPath.empty()) {
        ErrorHandler::LogError("Boş video yolu", ErrorLevel::WARNING);
        return false;
    }
    
    // Mevcut video varsa temizle
    if (isPlaying) {
        Stop();
    }
    Cleanup();
    
    currentVideoPath = videoPath;
    
    // DirectShow graph oluştur
    if (!InitializeGraphBuilder()) {
        ErrorHandler::LogError("DirectShow graph başlatılamadı", ErrorLevel::ERROR);
        return false;
    }
    
    // Video dosyasını yükle
    HRESULT hr = BuildGraph(videoPath);
    if (FAILED(hr)) {
        ErrorHandler::LogError("Video yüklenemedi: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
        Cleanup();
        return false;
    }
    
    // Video penceresini yapılandır
    ConfigureVideoWindow();
    
    ErrorHandler::LogInfo("Video başarıyla yüklendi: " + std::string(videoPath.begin(), videoPath.end()), InfoLevel::INFO);
    return true;
}

void VideoPlayer::Play() {
    if (!pMediaControl) {
        ErrorHandler::LogError("MediaControl mevcut değil", ErrorLevel::ERROR);
        return;
    }
    
    HRESULT hr = pMediaControl->Run();
    if (FAILED(hr)) {
        ErrorHandler::LogError("Video oynatılamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
        return;
    }
    
    isPlaying = true;
    shouldStop = false;
    
    // Video işleme thread'ini başlat
    StartVideoProcessingThread();
    
    ErrorHandler::LogInfo("Video oynatma başladı", InfoLevel::INFO);
}

void VideoPlayer::Stop() {
    if (!pMediaControl) return;
    
    shouldStop = true;
    isPlaying = false;
    
    HRESULT hr = pMediaControl->Stop();
    if (FAILED(hr)) {
        ErrorHandler::LogError("Video durdurulamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::WARNING);
    }
    
    // Thread'in bitmesini bekle
    if (videoThread && videoThread->joinable()) {
        videoThread->join();
    }
    
    ErrorHandler::LogInfo("Video oynatma durduruldu", InfoLevel::INFO);
}

void VideoPlayer::TogglePlayback() {
    if (isPlaying) {
        Stop();
    } else {
        Play();
    }
}

void VideoPlayer::SetTargetWindow(HWND hWnd) {
    targetWindow = hWnd;
    if (pVideoWindow && hWnd) {
        pVideoWindow->put_Owner((OAHWND)hWnd);
    }
}

bool VideoPlayer::InitializeGraphBuilder() {
    HRESULT hr = CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                                 IID_IGraphBuilder, (void**)&pGraphBuilder);
    if (FAILED(hr)) {
        ErrorHandler::LogError("FilterGraph oluşturulamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
        return false;
    }
    
    // Media Control interface'ini al
    hr = pGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&pMediaControl);
    if (FAILED(hr)) {
        ErrorHandler::LogError("MediaControl interface alınamadı", ErrorLevel::ERROR);
        return false;
    }
    
    // Video Window interface'ini al
    hr = pGraphBuilder->QueryInterface(IID_IVideoWindow, (void**)&pVideoWindow);
    if (FAILED(hr)) {
        ErrorHandler::LogError("VideoWindow interface alınamadı", ErrorLevel::WARNING);
        // Video olmayabilir, devam et
    }
    
    // Media Event interface'ini al
    hr = pGraphBuilder->QueryInterface(IID_IMediaEvent, (void**)&pMediaEvent);
    if (FAILED(hr)) {
        ErrorHandler::LogError("MediaEvent interface alınamadı", ErrorLevel::WARNING);
    }
    
    // Basic Video interface'ini al
    hr = pGraphBuilder->QueryInterface(IID_IBasicVideo, (void**)&pBasicVideo);
    if (FAILED(hr)) {
        ErrorHandler::LogError("BasicVideo interface alınamadı", ErrorLevel::WARNING);
    }
    
    return true;
}

HRESULT VideoPlayer::BuildGraph(const std::wstring& videoPath) {
    if (!pGraphBuilder) return E_FAIL;
    
    // Dosyayı graph'a ekle
    HRESULT hr = pGraphBuilder->RenderFile(videoPath.c_str(), nullptr);
    if (FAILED(hr)) {
        return hr;
    }
    
    return S_OK;
}

void VideoPlayer::ConfigureVideoWindow() {
    if (!pVideoWindow || !targetWindow) return;
    
    // Video penceresini hedef pencereye ayarla
    pVideoWindow->put_Owner((OAHWND)targetWindow);
    pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
    
    // Boyutları ayarla
    RECT rc;
    GetClientRect(targetWindow, &rc);
    pVideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
    
    ErrorHandler::LogInfo("Video penceresi yapılandırıldı", InfoLevel::DEBUG);
}

void VideoPlayer::StartVideoProcessingThread() {
    if (videoThread && videoThread->joinable()) {
        videoThread->join();
    }
    
    videoThread = std::make_unique<std::thread>(&VideoPlayer::VideoProcessingLoop, this);
}

void VideoPlayer::VideoProcessingLoop() {
    ErrorHandler::LogInfo("Video işleme thread'i başladı", InfoLevel::DEBUG);
    
    while (!shouldStop && isPlaying) {
        try {
            ProcessVideoFrame();
            
            // Frame rate kontrolü (yaklaşık 30 FPS)
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
            
        } catch (const std::exception& e) {
            ErrorHandler::LogError("Video işleme hatası: " + std::string(e.what()), ErrorLevel::ERROR);
            break;
        }
    }
    
    ErrorHandler::LogInfo("Video işleme thread'i sonlandı", InfoLevel::DEBUG);
}

void VideoPlayer::ProcessVideoFrame() {
    // Bu fonksiyon gerçek implementasyonda video frame'lerini işleyecek
    // Şu an için basit bir placeholder
    
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    // Buffer boyutunu kontrol et
    if (frameBuffer.size() >= maxBufferFrames) {
        frameBuffer.pop_front(); // En eski frame'i kaldır
    }
    
    // Yeni frame ekle (placeholder)
    auto newFrame = std::make_unique<FrameData>();
    newFrame->timestamp = GetTickCount();
    // newFrame->pBitmap gerçek implementasyonda doldurulacak
    
    frameBuffer.push_back(std::move(newFrame));
}

void VideoPlayer::ClearUnusedFrames() {
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    // Eski frame'leri temizle
    DWORD currentTime = GetTickCount();
    const DWORD maxAge = 5000; // 5 saniye
    
    auto it = frameBuffer.begin();
    while (it != frameBuffer.end()) {
        if (currentTime - (*it)->timestamp > maxAge) {
            it = frameBuffer.erase(it);
        } else {
            ++it;
        }
    }
}

void VideoPlayer::Cleanup() {
    // COM interface'lerini temizle
    if (pBasicVideo) {
        pBasicVideo->Release();
        pBasicVideo = nullptr;
    }
    
    if (pVideoWindow) {
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
    
    // Frame buffer'ı temizle
    std::lock_guard<std::mutex> lock(bufferMutex);
    frameBuffer.clear();
    
    imageProcessor.Cleanup();
}

void VideoPlayer::CleanupThreads() {
    for (auto* player : allInstances) {
        if (player) {
            player->shouldStop = true;
            if (player->videoThread && player->videoThread->joinable()) {
                player->videoThread->join();
            }
        }
    }
}