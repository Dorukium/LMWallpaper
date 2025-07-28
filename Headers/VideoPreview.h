// Headers/VideoPreview.h
#pragma once

#include <Windows.h>
#include <dshow.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <map>
#include <memory>
#include "resource.h"
#include "MemoryOptimizer.h"
#include "ErrorHandler.h"

class VideoPreview {
private:
    struct VideoInfo {
        int width;
        int height;
        double duration;
        double fps;
        std::string codec;
        size_t fileSize;
    };

    std::unique_ptr<IGraphBuilder> graphBuilder;
    std::unique_ptr<IMediaControl> mediaControl;
    std::unique_ptr<IVideoWindow> videoWindow;
    std::unique_ptr<IMediaEvent> mediaEvent;
    std::map<std::string, cv::Mat> thumbnailCache;
    const int MAX_CACHE_SIZE = 20;  // Maksimum 20 mini resim
    const int THUMBNAIL_SIZE = 128; // Mini resim boyutu (128x128)

public:
    VideoPreview();
    ~VideoPreview();
    bool GenerateThumbnail(const std::string& videoPath, const std::string& outputPath);
    VideoInfo GetVideoInfo(const std::string& videoPath);
    void ClearThumbnailCache();
    bool IsThumbnailCached(const std::string& videoPath);

private:
    void InitializeGraphBuilder();
    void ExtractFrame(const std::string& videoPath, int frameNumber, cv::Mat& frame);
    void UpdateCacheSize();
    void OptimizeMemoryUsage();
};