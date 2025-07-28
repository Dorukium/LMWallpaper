// tests/test_memory_optimizer.cpp
#include "TestUtils.h"
#include "../Headers/MemoryOptimizer.h"
#include <gtest/gtest.h>

class TestMemoryOptimizer : public ::testing::Test {
protected:
    std::unique_ptr<MemoryOptimizer> optimizer;

    void SetUp() override {
        TestUtils::InitializeTestEnvironment();
        optimizer = std::make_unique<MemoryOptimizer>();
    }

    void TearDown() override {
        optimizer.reset();
        TestUtils::CleanupTestEnvironment();
    }
};

TEST_F(TestMemoryOptimizer, MemoryMonitoring) {
    // Bellek izleme testi
    size_t initialMemory = optimizer->GetCurrentMemoryUsage();
    optimizer->MonitorMemoryUsage();
    EXPECT_GE(optimizer->GetPeakMemoryUsage(), initialMemory);
}

TEST_F(TestMemoryOptimizer, DynamicBufferResize) {
    // Dinamik buffer boyutlandırma testi
    optimizer->DynamicBufferResize();
    EXPECT_LT(optimizer->GetCurrentMemoryUsage(), 
              150 * 1024 * 1024);  // 150MB limit
}

TEST_F(TestMemoryOptimizer, Cleanup) {
    // Bellek temizleme testi
    optimizer->ClearUnusedFrames();
    size_t memoryUsage = optimizer->GetCurrentMemoryUsage();
    EXPECT_LT(memoryUsage, 100 * 1024 * 1024);  // 100MB limit
}