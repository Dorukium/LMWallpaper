// Source/VideoPlayer.cpp
#include "VideoPlayer.h"
#include "framework.h"

VideoPlayer::VideoPlayer(HMONITOR monitorHandle) 
    : monitorHandle(monitorHandle), isPlaying(false) {
    InitializeGraphBuilder();
    ConfigureVideoWindow();
}

VideoPlayer::~VideoPlayer() {
    Cleanup();
}

void VideoPlayer::InitializeGraphBuilder() {
    try {
        // DirectShow graf builder oluşturma
        CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&graphBuilder));
        
        // Media kontrol ve event nesneleri al
        graphBuilder->QueryInterface(IID_PPV_ARGS(&mediaControl));
        graphBuilder->QueryInterface(IID_PPV_ARGS(&mediaEvent));
        
        // Video pencere oluştur
        CoCreateInstance(CLSID_VideoWindow, nullptr, CLSCTX_INPROC,
                        IID_PPV_ARGS(&videoWindow));
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("DirectShow başlatma hatası: " + std::string(e.what()),
                             ErrorLevel::CRITICAL);
        throw;
    }
}

bool VideoPlayer::LoadVideo(const std::string& videoPath) {
    try {
        // Video dosyasını kontrol et
        if (!std::filesystem::exists(videoPath)) {
            ErrorHandler::LogError("Video dosyası bulunamadı: " + videoPath,
                                 ErrorLevel::ERROR);
            return false;
        }

        // Graf builder'ı temizle
        if (isPlaying) Stop();
        
        // Yeni video dosyasını yükle
        IGraphBuilder* pGraph = graphBuilder.get();
        IParse* pParse = nullptr;
        
        CoCreateInstance(CLSID_ParseFilter, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&pParse));
        
        pParse->Init(videoPath.c_str(), nullptr);
        pGraph->AddFilter(pParse, L"Source Filter");
        
        // Bellek optimizasyonu için buffer'ı temizle
        frameBuffer.clear();
        MemoryOptimizer::ClearUnusedFrames();
        
        currentVideoPath = videoPath;
        return true;
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Video yükleme hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
        return false;
    }
}

void VideoPlayer::Play() {
    try {
        if (!mediaControl) {
            ErrorHandler::LogError("Media kontrol nesnesi bulunamadı",
                                 ErrorLevel::ERROR);
            return;
        }

        mediaControl->Run();
        isPlaying = true;
        StartVideoProcessingThread();
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Video oynatma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::Stop() {
    try {
        if (mediaControl) {
            mediaControl->Stop();
        }
        isPlaying = false;
        Cleanup();
    }
    catch (const std::exception& e) {
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
    // Video işleme thread'i başlat
    std::thread([this]() {
        while (isPlaying) {
            ProcessVideoFrame();
            // CPU kullanımını optimize et
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }).detach();
}

void VideoPlayer::ProcessVideoFrame() {
    try {
        // Frame buffer'ı kontrol et
        if (frameBuffer.size() >= MAX_BUFFER_SIZE) {
            frameBuffer.pop_front();
        }

        // Yeni frame'ı işle
        cv::Mat frame;
        // ... Frame işleme mantığı burada eklenecek ...

        // Frame buffer'a ekle
        frameBuffer.push_back(std::move(frame));

        // Bellek optimizasyonu
        MemoryOptimizer::DynamicBufferResize();
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Frame işleme hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}

void VideoPlayer::Cleanup() {
    try {
        if (mediaControl) {
            mediaControl->Stop();
        }
        frameBuffer.clear();
        MemoryOptimizer::ClearUnusedFrames();
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Temizlik hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
    }
}