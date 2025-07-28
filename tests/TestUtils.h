// tests/TestUtils.h
#pragma once

#include <gtest/gtest.h>
#include <Windows.h>
#include <string>
#include <memory>
#include "../Headers/MemoryOptimizer.h"
#include "../Headers/VideoPlayer.h"
#include "../Headers/VideoPreview.h"

class TestUtils {
public:
    static void InitializeTestEnvironment() {
        // Test ortamını hazırla
        SetEnvironmentVariable(L"LMWALLPAPER_TEST_MODE", L"1");
        
        // Test için gerekli dosyaları oluştur
        CreateDirectory(L"test_data", nullptr);
        CreateDirectory(L"test_data\\videos", nullptr);
    }

    static void CleanupTestEnvironment() {
        // Test dosyalarını temizle
        RemoveDirectory(L"test_data");
    }

    static std::wstring GetTestVideoPath() {
        return L"test_data\\test_video.mp4";
    }

    static std::wstring GetTestThumbnailPath() {
        return L"test_data\\thumbnail.jpg";
    }
};