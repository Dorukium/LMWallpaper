// Source/TrayManager.cpp
#include "../Headers/TrayManager.h"
#include "../Headers/SettingsWindow.h"

TrayManager::TrayManager(HWND hWnd) {
    iconData = std::make_unique<TrayIconData>();
    iconData->hWndMain = hWnd;
    CreateTrayMenu();
}

TrayManager::~TrayManager() {
    if (iconData) {
        // Sistem tepsisi ikonunu kaldır
        Shell_NotifyIcon(NIM_DELETE, &iconData->nid);
        
        if (iconData->hMenu) {
            DestroyMenu(iconData->hMenu);
        }
    }
}

bool TrayManager::Initialize(bool autoStart) {
    try {
        if (!iconData) {
            return false;
        }

        // Sistem tepsisi ikonu ayarları
        iconData->nid.cbSize = sizeof(NOTIFYICONDATA);
        iconData->nid.hWnd = iconData->hWndMain;
        iconData->nid.uID = ID_TRAY_ICON;
        iconData->nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        iconData->nid.uCallbackMessage = WM_TRAY_MESSAGE;
        
        // Icon yükleme
        iconData->nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LMWALLPAPER));
        if (!iconData->nid.hIcon) {
            // Varsayılan application icon kullan
            iconData->nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        }
        
        wcscpy_s(iconData->nid.szTip, L"LMWallpaper - Duraklatıldı");

        // Sistem tepsisine ikonu ekle
        if (!Shell_NotifyIcon(NIM_ADD, &iconData->nid)) {
            ErrorHandler::LogError("Sistem tepsisi ikonu eklenemedi", ErrorLevel::CRITICAL);
            return false;
        }

        // Başlangıç durumunu ayarla
        UpdateTrayIcon(autoStart);
        iconData->isPlaying = autoStart;

        return true;
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("TrayManager başlatma hatası: " + std::string(e.what()), ErrorLevel::CRITICAL);
        return false;
    }
}

void TrayManager::CreateTrayMenu() {
    if (!iconData) return;

    iconData->hMenu = CreatePopupMenu();
    if (!iconData->hMenu) {
        ErrorHandler::LogError("Sistem tepsisi menüsü oluşturulamadı", ErrorLevel::CRITICAL);
        return;
    }

    // Menü öğelerini ekle
    AppendMenu(iconData->hMenu, MF_STRING, ID_TRAY_PLAYPAUSE, L"▶ Wallpaper Başlat/Durdur");
    AppendMenu(iconData->hMenu, MF_STRING, ID_TRAY_CHANGEVIDEO, L"🎬 Video Değiştir");
    AppendMenu(iconData->hMenu, MF_STRING, ID_TRAY_MONITOR, L"🖥️ Monitör Seçimi");
    AppendMenu(iconData->hMenu, MF_STRING, ID_TRAY_SETTINGS, L"⚙️ Ayarlar");
    AppendMenu(iconData->hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(iconData->hMenu, MF_STRING, ID_TRAY_PERFORMANCE, L"📊 Performans");
    AppendMenu(iconData->hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(iconData->hMenu, MF_STRING, ID_TRAY_EXIT, L"❌ Çıkış");
}

void TrayManager::UpdateTrayIcon(bool isPlaying) {
    if (!iconData) return;

    iconData->isPlaying = isPlaying;
    
    // Icon değiştir
    HICON newIcon = isPlaying ? 
        LoadIcon(hInst, MAKEINTRESOURCE(IDI_TRAY_ICON_ACTIVE)) :
        LoadIcon(hInst, MAKEINTRESOURCE(IDI_TRAY_ICON));
    
    if (!newIcon) {
        newIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    iconData->nid.hIcon = newIcon;
    
    // Tooltip güncelle
    wcscpy_s(iconData->nid.szTip, isPlaying ? L"LMWallpaper - Çalışıyor" : L"LMWallpaper - Duraklatıldı");
    
    Shell_NotifyIcon(NIM_MODIFY, &iconData->nid);
    
    // Menü öğesini güncelle
    if (iconData->hMenu) {
        ModifyMenu(iconData->hMenu, ID_TRAY_PLAYPAUSE, MF_BYCOMMAND | MF_STRING, 
                  ID_TRAY_PLAYPAUSE, isPlaying ? L"⏸ Wallpaper Durdur" : L"▶ Wallpaper Başlat");
    }
}

void TrayManager::HandleMenuItem(WORD menuItemId) {
    switch (menuItemId) {
        case ID_TRAY_PLAYPAUSE:
            TogglePlayback();
            break;

        case ID_TRAY_CHANGEVIDEO:
            {
                // Dosya seçme diyalogu
                OPENFILENAME ofn = {0};
                wchar_t szFile[MAX_PATH] = {0};
                
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = iconData->hWndMain;
                ofn.lpstrFile = szFile;
                ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
                ofn.lpstrFilter = L"Video Dosyaları\0*.mp4;*.avi;*.mov;*.wmv;*.mkv;*.webm\0Tüm Dosyalar\0*.*\0";
                ofn.nFilterIndex = 1;
                ofn.lpstrFileTitle = nullptr;
                ofn.nMaxFileTitle = 0;
                ofn.lpstrInitialDir = nullptr;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                
                if (GetOpenFileName(&ofn)) {
                    // Video değiştirme işlemi burada yapılacak
                    ErrorHandler::LogInfo("Video seçildi: " + std::string((char*)szFile), InfoLevel::INFO);
                }
            }
            break;

        case ID_TRAY_MONITOR:
            // Monitör seçimi penceresi
            MessageBox(iconData->hWndMain, L"Monitör seçimi özelliği henüz hazır değil.", L"Bilgi", MB_ICONINFORMATION);
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

void TrayManager::UpdatePlayState(bool isPlaying) {
    UpdateTrayIcon(isPlaying);
}

LRESULT TrayManager::HandleTrayMessage(WPARAM wParam, LPARAM lParam) {
    if (wParam != ID_TRAY_ICON || !iconData) {
        return 0;
    }

    switch (lParam) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            {
                // Sağ tık menüsü göster
                POINT cursor;
                GetCursorPos(&cursor);
                
                // Menüyü ön plana getir
                SetForegroundWindow(iconData->hWndMain);
                
                TrackPopupMenu(iconData->hMenu, 
                              TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                              cursor.x, cursor.y, 0, iconData->hWndMain, nullptr);
                
                // Menü kapandıktan sonra focus'u geri al
                PostMessage(iconData->hWndMain, WM_NULL, 0, 0);
            }
            break;

        case WM_LBUTTONDBLCLK:
            // Çift tık ile ayarlar penceresi
            ShowSettingsWindow();
            break;

        case WM_LBUTTONUP:
            // Tek tık ile play/pause toggle
            TogglePlayback();
            break;
    }

    return 0;
}

void TrayManager::TogglePlayback() {
    iconData->isPlaying = !iconData->isPlaying;
    UpdateTrayIcon(iconData->isPlaying);
    
    // Video oynatma/durdurma mantığı burada eklenecek
    ErrorHandler::LogInfo(iconData->isPlaying ? "Wallpaper başlatıldı" : "Wallpaper durduruldu", InfoLevel::INFO);
}

void TrayManager::ShowSettingsWindow() {
    try {
        // Ayarlar penceresi göster
        static std::unique_ptr<SettingsWindow> settingsWindow = nullptr;
        
        if (!settingsWindow) {
            settingsWindow = std::make_unique<SettingsWindow>();
        }
        
        settingsWindow->Show();
    }
    catch (const std::exception& e) {
        ErrorHandler::LogError("Ayarlar penceresi açma hatası: " + std::string(e.what()), ErrorLevel::ERROR);
        MessageBox(iconData->hWndMain, L"Ayarlar penceresi açılamadı.", L"Hata", MB_ICONERROR);
    }
}

void TrayManager::ShowPerformance() {
    // Performans bilgilerini göster
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    
    std::wostringstream perfInfo;
    perfInfo << L"LMWallpaper Performans Bilgileri\n\n";
    perfInfo << L"Bellek Kullanımı: " << (pmc.WorkingSetSize / 1024 / 1024) << L" MB\n";
    perfInfo << L"Sistem Bellek: " << (memInfo.ullAvailPhys / 1024 / 1024) << L" MB Boş / ";
    perfInfo << (memInfo.ullTotalPhys / 1024 / 1024) << L" MB Toplam\n";
    perfInfo << L"CPU Kullanımı: Hesaplanıyor...\n";
    
    MessageBox(iconData->hWndMain, perfInfo.str().c_str(), L"Performans", MB_ICONINFORMATION);
}

void TrayManager::ExitApplication() {
    int result = MessageBox(iconData->hWndMain, 
                           L"LMWallpaper'dan çıkmak istediğinizden emin misiniz?", 
                           L"Çıkış", 
                           MB_YESNO | MB_ICONQUESTION);
    
    if (result == IDYES) {
        PostMessage(iconData->hWndMain, WM_DESTROY, 0, 0);
    }
}