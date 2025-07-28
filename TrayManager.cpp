// TrayManager.cpp
#include "TrayManager.h"
#include "framework.h"

TrayManager::TrayManager(HWND hWnd) : hWndMain(hWnd), isPlaying(false) {
    // Sistem tepsisi menüsü oluştur
    CreateTrayMenu();
}

TrayManager::~TrayManager() {
    // Sistem tepsisi ikonunu kaldır
    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hMenu) {
        DestroyMenu(hMenu);
    }
}

bool TrayManager::Initialize(bool autoStart) {
    try {
        // Sistem tepsisi ikonu ayarları
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hWndMain;
        nid.uID = ID_TRAY_ICON;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAY_MESSAGE;
        nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TRAY_ICON));
        wcscpy_s(nid.szTip, L"LMWallpaper");

        // Sistem tepsisine ikonu ekle
        if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
            ErrorHandler::LogError("Sistem tepsisi ikonu eklenemedi", ErrorLevel::CRITICAL);
            return false;
        }

        // Başlangıç durumunu ayarla
        UpdateTrayIcon(autoStart);

        return true;
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError(e.what(), ErrorLevel::CRITICAL);
        return false;
    }
}

void TrayManager::CreateTrayMenu() {
    hMenu = CreatePopupMenu();
    if (!hMenu) {
        ErrorHandler::LogError("Sistem tepsisi menüsü oluşturulamadı", ErrorLevel::CRITICAL);
        return;
    }

    // Menü öğelerini ekle
    AppendMenu(hMenu, MF_STRING, ID_TRAY_PLAYPAUSE, L"▶ Wallpaper Başlat/Durdur");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_CHANGEVIDEO, L"🎬 Video Değiştir");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_MONITOR, L"🖥️ Monitör Seçimi");
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SETTINGS, L"⚙️ Ayarlar");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_PERFORMANCE, L"📊 Performans");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, L"❌ Çıkış");
}

void TrayManager::UpdateTrayIcon(bool isPlaying) {
    this->isPlaying = isPlaying;
    HICON icon = isPlaying ? LoadIcon(hInst, MAKEINTRESOURCE(IDI_TRAY_ICON_ACTIVE)) 
                          : LoadIcon(hInst, MAKEINTRESOURCE(IDI_TRAY_ICON));
    
    nid.hIcon = icon;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void TrayManager::HandleMenuItem(WORD menuItemId) {
    switch (menuItemId) {
        case ID_TRAY_PLAYPAUSE:
            TogglePlayback();
            break;

        case ID_TRAY_SETTINGS:
            ShowSettingsWindow();
            break;

        case ID_TRAY_PERFORMANCE:
            ShowPerformance();
            break;

        case ID_TRAY_EXIT:
            ExitApplication();
            break;
    }
}

void TrayManager::TogglePlayback() {
    isPlaying = !isPlaying;
    UpdateTrayIcon(isPlaying);
    // Video oynatma mantığı burada eklenecek
}

void TrayManager::ShowSettingsWindow() {
    // Ayarlar penceresi gösterme mantığı burada eklenecek
}

void TrayManager::ShowPerformance() {
    // Performans bilgisi gösterme mantığı burada eklenecek
}

void TrayManager::ExitApplication() {
    PostMessage(hWndMain, WM_CLOSE, 0, 0);
}