// tests/test_main.cpp
#include "TestUtils.h"
#include "../Headers/TrayManager.h"
#include <gtest/gtest.h>

class TestMain : public ::testing::Test {
protected:
    std::unique_ptr<TrayManager> trayManager;

    void SetUp() override {
        TestUtils::InitializeTestEnvironment();
        trayManager = std::make_unique<TrayManager>(nullptr);
    }

    void TearDown() override {
        trayManager.reset();
        TestUtils::CleanupTestEnvironment();
    }
};

TEST_F(TestMain, SingleInstanceCheck) {
    // Tek instance kontrolü testi
    EXPECT_FALSE(CheckInstanceRunning());
}

TEST_F(TestMain, TrayIconInitialization) {
    // Sistem tepsisi ikonu testi
    EXPECT_TRUE(trayManager->Initialize(false));
}

TEST_F(TestMain, MemoryUsage) {
    // Bellek kullanımı testi
    MemoryOptimizer optimizer;
    size_t initialMemory = optimizer.GetCurrentMemoryUsage();
    EXPECT_LT(initialMemory, 50 * 1024 * 1024);  // 50MB altı
}