// Headers/ThemeManager.h
#pragma once

#include "framework.h"
#include "ErrorHandler.h"

enum class Theme {
    DARK = 0,
    LIGHT = 1,
    AUTO = 2
};

class ThemeManager {
private:
    Theme currentTheme;
    bool systemDarkMode;
    
    void DetectSystemTheme();
    void ApplyThemeColors();

public:
    ThemeManager();
    ~ThemeManager();
    
    void SetTheme(Theme theme);
    Theme GetCurrentTheme() const { return currentTheme; }
    bool IsSystemDarkMode() const { return systemDarkMode; }
    
    // Renk getters
    D2D1_COLOR_F GetBackgroundColor() const;
    D2D1_COLOR_F GetForegroundColor() const;
    D2D1_COLOR_F GetAccentColor() const;
    D2D1_COLOR_F GetBorderColor() const;
    
    void UpdateSystemTheme();
};

// Headers/MonitorManager.h
#pragma once

#include "framework.h"
#include "ErrorHandler.h"

struct MonitorInfo {
    HMONITOR hMonitor;
    std::wstring deviceName;
    RECT bounds;
    bool isPrimary;
    int width;
    int height;
    
    MonitorInfo() : hMonitor(nullptr), isPrimary(false), width(0), height(0) {
        ZeroMemory(&bounds, sizeof(bounds));
    }
};

class MonitorManager {
private:
    std::vector<MonitorInfo> monitors;
    HMONITOR primaryMonitor;
    
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, 
                                        LPRECT lprcMonitor, LPARAM dwData);

public:
    MonitorManager();
    ~MonitorManager();
    
    bool RefreshMonitors();
    const std::vector<MonitorInfo>& GetMonitors() const { return monitors; }
    MonitorInfo GetPrimaryMonitor() const;
    MonitorInfo GetMonitorByIndex(int index) const;
    int GetMonitorCount() const { return static_cast<int>(monitors.size()); }
    
    // Wallpaper için gerekli fonksiyonlar
    bool SetWallpaperOnMonitor(HMONITOR hMonitor, const std::wstring& imagePath);
    HWND GetDesktopWindowForMonitor(HMONITOR hMonitor);
};