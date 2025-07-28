// Headers/SettingsWindow.h
#pragma once

#include <Windows.h>
#include <dwmapi.h>
#include <string>
#include <memory>
#include "resource.h"
#include "ThemeManager.h"
#include "VideoPreview.h"
#include "MonitorManager.h"

class SettingsWindow {
private:
    HWND hWnd;
    ThemeManager themeManager;
    VideoPreview videoPreview;
    MonitorManager monitorManager;
    std::unique_ptr<ID2D1HwndRenderTarget> renderTarget;
    std::unique_ptr<ID2D1SolidColorBrush> brush;
    bool isDragging;
    POINT dragOffset;

    void InitializeD2D();
    void CreateModernWindow();
    void SetupWindowShadow();
    void InitializeControls();
    void UpdateTheme();
    void UpdateWindowFrame();

public:
    SettingsWindow();
    ~SettingsWindow();
    void Show();
    void Hide();
    void Update();
    LRESULT MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};