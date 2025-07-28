#pragma once
#include "../framework.h"
#include "resource.h"
// NOTE: Added shellapi.h for NOTIFYICONDATA
#include <shellapi.h>

#define WM_TRAY_ICON (WM_USER + 1)

class TrayManager {
public:
    TrayManager(HWND hWnd);
    ~TrayManager();

    void ShowContextMenu(HWND hWnd);

private:
    // NOTE: Correctly defined the member variable
    NOTIFYICONDATA nid;
    HWND m_hWnd;
};
