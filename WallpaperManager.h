// WallpaperManager.h
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

class WallpaperManager {
public:
    WallpaperManager();
    ~WallpaperManager();
    
    void setVideoWallpaper(const std::string& path);
    void setLoopEnabled(bool enable);
    void setMonitorCount(int count);
    
private:
    struct MonitorInfo {
        RECT rect;
        bool isActive;
    };
    
    std::vector<MonitorInfo> monitors_;
    std::atomic<bool> isLooping_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::unique_ptr<void, std::function<void(void*)>> videoHandle_;
};