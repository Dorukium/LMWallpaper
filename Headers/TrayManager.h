// Headers/TrayManager.h
#pragma once

#include "framework.h"
#include "resource.h"
#include "ErrorHandler.h"

class TrayManager {
private:
    struct TrayIconData {
        NOTIFYICONDATA nid;
        HMENU hMenu;
        bool isPlaying;
        HWND hWndMain;
        
        TrayIconData() {
            ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
            hMenu = nullptr;
            isPlaying = false;
            hWndMain = nullptr;
        }
    };
    
    std::unique_ptr<TrayIconData> iconData;

    void CreateTrayMenu();
    void UpdateTrayIcon(bool isPlaying);
    void ShowSettingsWindow();
    void TogglePlayback();
    void ShowPerformance();
    void ExitApplication();

public:
    TrayManager(HWND hWnd);
    ~TrayManager();
    
    bool Initialize(bool autoStart);
    void HandleMenuItem(WORD menuItemId);
    void UpdatePlayState(bool isPlaying);
    
    LRESULT HandleTrayMessage(WPARAM wParam, LPARAM lParam);
};