// Headers/VideoPlayer.h
#pragma once

#include <Windows.h>
#include <dshow.h>
#include <opencv2/opencv.hpp>
#include <deque>
#include <string>
#include "resource.h"
#include "MemoryOptimizer.h"
#include "ErrorHandler.h"

class VideoPlayer {
private:
    std::unique_ptr<IGraphBuilder> graphBuilder;
    std::unique_ptr<IMediaControl> mediaControl;
    std::unique_ptr<IVideoWindow> videoWindow;
    std::unique_ptr<IMediaEvent> mediaEvent;
    std::deque<cv::Mat> frameBuffer;
    const int MAX_BUFFER_SIZE = 3;  // Maksimum 3 frame bellekte tut
    bool isPlaying;
    std::string currentVideoPath;
    HMONITOR monitorHandle;

public:
    VideoPlayer(HMONITOR monitorHandle);
    ~VideoPlayer();
    bool LoadVideo(const std::string& videoPath);
    void Play();
    void Stop();
    void TogglePlayback();

private:
    void InitializeGraphBuilder();
    void ConfigureVideoWindow();
    void StartVideoProcessingThread();
    void ProcessVideoFrame();
    void Cleanup();
};