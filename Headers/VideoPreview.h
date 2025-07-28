// Headers/VideoPreview.h
#pragma once

#include "framework.h"
#include "resource.h"
#include "MemoryOptimizer.h"
#include "ErrorHandler.h"
#include "ImageProcessor.h"

class VideoPreview {
public:
    struct VideoInfo {
        int width;
        int height;
        double duration;
        double fps;
        std::string codec;
        size_t fileSize;
        
        VideoInfo() : width(0), height(0), duration(0.0), fps(0.0), fileSize(0) {}
    };

private:
    ImageProcessor imageProcessor;
    std::map<std::string, std::wstring> thumbnailCache; // path -> thumbnail path
    std::mutex cacheMutex;
    
    static const int MAX_CACHE_SIZE = 20;  // Maksimum 20 mini resim
    static const int THUMBNAIL_SIZE = 128; // Mini resim boyutu (128x128)

    bool CreateThumbnailWithDirectShow(const std::wstring& videoPath, const std::wstring& outputPath);

public:
    VideoPreview();
    ~VideoPreview();
    
    bool GenerateThumbnail(const std::wstring& videoPath, const std::wstring& outputPath);
    VideoInfo GetVideoInfo(const std::wstring& videoPath);
    void ClearThumbnailCache();
    bool IsThumbnailCached(const std::string& videoPath);

private:
    void UpdateCacheSize();
    void OptimizeMemoryUsage();
};