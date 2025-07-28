// TrayManager.h
#pragma once

#include <Windows.h>
#include <shellapi.h>
#include "resource.h"
#include "ErrorHandler.h"

class TrayManager {
private:
    HWND hWndMain;
    NOTIFYICONDATA nid;
    HMENU hMenu;
    bool isPlaying;

    void CreateTrayMenu();
    void UpdateTrayIcon();
    void ShowSettingsWindow();
    void TogglePlayback();
    void ShowPerformance();
    void ExitApplication();

public:
    TrayManager(HWND hWnd);
    ~TrayManager();
    bool Initialize(bool autoStart);
    void HandleMenuItem(WORD menuItemId);
};