// tests/test_video_player.cpp
#include "TestUtils.h"
#include "../Headers/VideoPlayer.h"
#include <gtest/gtest.h>

class TestVideoPlayer : public ::testing::Test {
protected:
    std::unique_ptr<VideoPlayer> videoPlayer;

    void SetUp() override {
        TestUtils::InitializeTestEnvironment();
        videoPlayer = std::make_unique<VideoPlayer(nullptr));
    }

    void TearDown() override {
        videoPlayer.reset();
        TestUtils::CleanupTestEnvironment();
    }
};

TEST_F(TestVideoPlayer, VideoLoading) {
    // Video yükleme testi
    std::wstring videoPath = TestUtils::GetTestVideoPath();
    EXPECT_TRUE(videoPlayer->LoadVideo(videoPath));
}

TEST_F(TestVideoPlayer, FrameProcessing) {
    // Frame işleme testi
    std::wstring videoPath = TestUtils::GetTestVideoPath();
    videoPlayer->LoadVideo(videoPath);
    videoPlayer->ProcessVideoFrame();
    EXPECT_GT(videoPlayer->GetFrameCount(), 0);
}

TEST_F(TestVideoPlayer, MemoryOptimization) {
    // Bellek optimizasyonu testi
    MemoryOptimizer optimizer;
    videoPlayer->LoadVideo(TestUtils::GetTestVideoPath());
    optimizer.AutoCleanup();
    size_t memoryUsage = optimizer.GetCurrentMemoryUsage();
    EXPECT_LT(memoryUsage, 100 * 1024 * 1024);  // 100MB altı
}