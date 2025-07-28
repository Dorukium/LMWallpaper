// Headers/MemoryOptimizer.h
#pragma once

#include <Windows.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <memory>
#include "resource.h"
#include "ErrorHandler.h"

class MemoryOptimizer {
private:
    std::atomic<size_t> currentMemoryUsage;
    std::atomic<size_t> peakMemoryUsage;
    std::atomic<size_t> memoryLimit;
    std::unique_ptr<std::thread> cleanupThread;
    std::atomic<bool> isRunning;
    std::mutex mutex;

public:
    MemoryOptimizer();
    ~MemoryOptimizer();
    void AutoCleanup();
    void ClearUnusedFrames();
    void DynamicBufferResize();
    size_t GetCurrentMemoryUsage();

private:
    void CleanupLoop();
    void MonitorMemoryUsage();
    void OptimizeMemoryAllocation();
};