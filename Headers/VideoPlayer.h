// Headers/VideoPlayer.h
#pragma once

#include "framework.h"
#include "MemoryOptimizer.h"
#include "ErrorHandler.h"
#include "ImageProcessor.h"

class VideoPlayer {
private:
    struct FrameData {
        ID2D1Bitmap* pBitmap;
        DWORD timestamp;
        
        FrameData() : pBitmap(nullptr), timestamp(0) {}
        ~FrameData() {
            if (pBitmap) {
                pBitmap->Release();
                pBitmap = nullptr;
            }
        }
    };
    
    IGraphBuilder* pGraphBuilder;
    IMediaControl* pMediaControl;
    IVideoWindow* pVideoWindow;
    IMediaEvent* pMediaEvent;
    IBasicVideo* pBasicVideo;
    
    std::deque<std::unique_ptr<FrameData>> frameBuffer;
    static const int MAX_BUFFER_SIZE = 3;
    static int maxBufferFrames;
    
    bool isPlaying;
    std::wstring currentVideoPath;
    HMONITOR monitorHandle;
    HWND targetWindow;
    
    std::unique_ptr<std::thread> videoThread;
    std::atomic<bool> shouldStop;
    std::mutex bufferMutex;
    
    ImageProcessor imageProcessor;
    
    static std::vector<VideoPlayer*> allInstances;

public:
    VideoPlayer(HMONITOR monitorHandle);
    ~VideoPlayer();
    
    bool LoadVideo(const std::wstring& videoPath);
    void Play();
    void Stop();
    void TogglePlayback();
    void SetTargetWindow(HWND hWnd);
    
    // Static methods for memory management
    static std::vector<VideoPlayer*>& GetAllInstances() { return allInstances; }
    static void SetMaxBufferFrames(int frames) { maxBufferFrames = frames; }
    static void CleanupThreads();
    
    // Instance methods
    void ClearUnusedFrames();
    size_t GetFrameCount() const { return frameBuffer.size(); }
    bool IsPlaying() const { return isPlaying; }

private:
    bool InitializeGraphBuilder();
    void ConfigureVideoWindow();
    void StartVideoProcessingThread();
    void VideoProcessingLoop();
    void ProcessVideoFrame();
    void Cleanup();
    HRESULT BuildGraph(const std::wstring& videoPath);
};