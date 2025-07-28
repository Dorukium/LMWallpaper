// SettingsDialog.h
#pragma once

#include <Windows.h>
#include <string>
#include <memory>

class SettingsDialog {
public:
    SettingsDialog();
    ~SettingsDialog();
    
    int showDialog();
    void setVideoPath(const std::string& path);
    bool isLoopEnabled() const;
    
private:
    static INT_PTR CALLBACK dialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    std::string videoPath_;
    bool loopEnabled_;
    HWND hwnd_;
};