// Source/VideoPreview.cpp
#include "VideoPreview.h"
#include "framework.h"

VideoPreview::VideoPreview() {
    InitializeGraphBuilder();
}

VideoPreview::~VideoPreview() {
    // Kaynakları temizle
    graphBuilder.reset();
    mediaControl.reset();
    videoWindow.reset();
    mediaEvent.reset();
    thumbnailCache.clear();
}

bool VideoPreview::GenerateThumbnail(const std::string& videoPath, const std::string& outputPath) {
    try {
        // Video dosyasını kontrol et
        if (!std::filesystem::exists(videoPath)) {
            ErrorHandler::LogError("Video dosyası bulunamadı: " + videoPath, ErrorLevel::ERROR);
            return false;
        }

        // Önbellekte var mı kontrol et
        if (IsThumbnailCached(videoPath)) {
            return true;
        }

        // Graf builder'ı temizle
        if (mediaControl) {
            mediaControl->Stop();
        }

        // Yeni video dosyasını yükle
        IGraphBuilder* pGraph = graphBuilder.get();
        IParse* pParse = nullptr;
        
        CoCreateInstance(CLSID_ParseFilter, nullptr, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARGS(&pParse));
        
        pParse->Init(videoPath.c_str(), nullptr);
        pGraph->AddFilter(pParse, L"Source Filter");

        // İlk kareyi al
        cv::Mat frame;
        ExtractFrame(videoPath, 0, frame);

        // Mini resmi kaydet
        cv::resize(frame, frame, cv::Size(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        cv::imwrite(outputPath, frame);

        // Önbelleğe ekle
        thumbnailCache[videoPath] = frame;
        UpdateCacheSize();

        return true;
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Mini resim oluşturma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
        return false;
    }
}

VideoPreview::VideoInfo VideoPreview::GetVideoInfo(const std::string& videoPath) {
    VideoInfo info;
    try {
        cv::VideoCapture cap(videoPath);
        if (!cap.isOpened()) {
            throw std::runtime_error("Video dosyası açılamadı");
        }

        info.width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        info.height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        info.duration = cap.get(cv::CAP_PROP_DURATION);
        info.fps = cap.get(cv::CAP_PROP_FPS);
        info.fileSize = std::filesystem::file_size(videoPath);
        
        // Codec bilgisini al
        IBaseFilter* pSourceFilter;
        graphBuilder->FindFilterByIndex(0, &pSourceFilter);
        WCHAR codecName[256] = {0};
        pSourceFilter->QueryFilterInfo(codecName, nullptr);
        info.codec = codecName;
        pSourceFilter->Release();

        return info;
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Video bilgi alma hatası: " + std::string(e.what()),
                             ErrorLevel::ERROR);
        throw;
    }
}

void VideoPreview::ClearThumbnailCache() {
    thumbnailCache.clear();
    MemoryOptimizer::ClearUnusedFrames();
}

bool VideoPreview::IsThumbnailCached(const std::string& videoPath) {
    return thumbnailCache.find(videoPath) != thumbnailCache.end();
}

void VideoPreview::UpdateCacheSize() {
    // Önbellek boyutunu kontrol et
    while (thumbnailCache.size() > MAX_CACHE_SIZE) {
        thumbnailCache.erase(thumbnailCache.begin());
    }
    MemoryOptimizer::DynamicBufferResize();
}

void VideoPreview::OptimizeMemoryUsage() {
    // Bellek kullanımını optimize et
    size_t currentMemory = MemoryOptimizer::GetCurrentMemoryUsage();
    if (currentMemory > 100 * 1024 * 1024) {  // 100MB
        ClearThumbnailCache();
    }
}