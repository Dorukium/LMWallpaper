// Source/TrayManager.cpp
#include "../Headers/TrayManager.h"
#include "../Headers/SettingsWindow.h"

// Global settings window pointer
static std::unique_ptr<SettingsWindow> g_settingsWindow = nullptr;

TrayManager::TrayManager(HWND hWnd) {
    iconData = std::make_unique<TrayIconData>();
    iconData->hWndMain = hWnd;
}

TrayManager::~TrayManager() {
    if (iconData && iconData->nid.hWnd) {
        Shell_NotifyIcon(NIM_DELETE, &iconData->nid);
    }
    
    if (iconData && iconData->hMenu) {
        DestroyMenu(iconData->hMenu);
    }
    
    g_settingsWindow.reset();
}

bool TrayManager::Initialize(bool autoStart) {
    if (!iconData || !iconData->hWndMain) {
        ErrorHandler::LogError("TrayManager başlatılamadı - geçersiz pencere handle", ErrorLevel::ERROR);
        return false;
    }

    // Tray icon yapısını doldur
    iconData->nid.cbSize = sizeof(NOTIFYICONDATA);
    iconData->nid.hWnd = iconData->hWndMain;
    iconData->nid.uID = 1;
    iconData->nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    iconData->nid.uCallbackMessage = WM_TRAY_MESSAGE;
    
    // Icon yükle
    iconData->nid.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_LMWALLPAPER));
    if (!iconData->nid.hIcon) {
        iconData->nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    
    // Tooltip metni
    wcscpy_s(iconData->nid.szTip, L"LMWallpaper - Duraklatıldı");
    
    // Tray menüsü oluştur
    CreateTrayMenu();
    
    // Tray icon'u ekle
    if (!Shell_NotifyIcon(NIM_ADD, &iconData->nid)) {
        ErrorHandler::LogError("Sistem tepsisi ikonu eklenemedi: " + ErrorHandler::GetLastErrorAsString(), 
                              ErrorLevel::ERROR);
        return false;
    }
    
    ErrorHandler::LogInfo("Sistem tepsisi ikonu başarıyla eklendi", InfoLevel::INFO);
    
    // Otomatik başlatma durumunda oynatmayı başlat
    if (autoStart) {
        iconData->isPlaying = true;
        UpdateTrayIcon(true);
    }
    
    return true;
}

void TrayManager::CreateTrayMenu() {
    iconData->hMenu = CreatePopupMenu();
    if (!iconData->hMenu) {
        ErrorHandler::LogError("Tray menüsü oluşturulamadı", ErrorLevel::ERROR);
        return;
    }
    
    // Menü öğelerini ekle
    AppendMenu(iconData->hMenu, MF_STRING, IDM_TOGGLE_PLAYBACK, L"Oynat/Duraklat");
    AppendMenu(iconData->hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(iconData->hMenu, MF_STRING, IDM_SETTINGS, L"Ayarlar...");
    AppendMenu(iconData->hMenu, MF_STRING, IDM_PERFORMANCE, L"Performans...");
    AppendMenu(iconData->hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(iconData->hMenu, MF_STRING, IDM_ABOUT, L"Hakkında...");
    AppendMenu(iconData->hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(iconData->hMenu, MF_STRING, IDM_EXIT, L"Çıkış");
    
    ErrorHandler::LogInfo("Tray menüsü oluşturuldu", InfoLevel::DEBUG);
}

void TrayManager::UpdateTrayIcon(bool isPlaying) {
    iconData->isPlaying = isPlaying;
    
    // Tooltip metnini güncelle
    if (isPlaying) {
        wcscpy_s(iconData->nid.szTip, L"LMWallpaper - Oynatılıyor");
    } else {
        wcscpy_s(iconData->nid.szTip, L"LMWallpaper - Duraklatıldı");
    }
    
    // Icon'u güncelle
    Shell_NotifyIcon(NIM_MODIFY, &iconData->nid);
    
    ErrorHandler::LogInfo(isPlaying ? "Oynatma başlatıldı" : "Oynatma durduruldu", InfoLevel::INFO);
}

LRESULT TrayManager::HandleTrayMessage(WPARAM wParam, LPARAM lParam) {
    if (wParam != 1) return 0; // Sadece bizim icon'umuz
    
    switch (lParam) {
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            {
                if (!iconData->hMenu) return 0;
                
                POINT pt;
                GetCursorPos(&pt);
                
                // Menü durumunu güncelle
                ModifyMenu(iconData->hMenu, IDM_TOGGLE_PLAYBACK, MF_BYCOMMAND | MF_STRING,
                          IDM_TOGGLE_PLAYBACK, 
                          iconData->isPlaying ? L"Duraklat" : L"Oynat");
                
                // Menüyü göster
                SetForegroundWindow(iconData->hWndMain);
                TrackPopupMenu(iconData->hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
                              pt.x, pt.y, 0, iconData->hWndMain, nullptr);
                PostMessage(iconData->hWndMain, WM_NULL, 0, 0);
            }
            break;
            
        case WM_LBUTTONDBLCLK:
            ShowSettingsWindow();
            break;
    }
    
    return 0;
}

void TrayManager::HandleMenuItem(WORD menuItemId) {
    switch (menuItemId) {
        case IDM_SETTINGS:
            ShowSettingsWindow();
            break;
            
        case IDM_TOGGLE_PLAYBACK:
            TogglePlayback();
            break;
            
        case IDM_PERFORMANCE:
            ShowPerformance();
            break;
            
        case IDM_ABOUT:
            {
                std::string aboutText = "LMWallpaper v1.0.0\n\n";
                aboutText += "Dinamik duvar kağıdı uygulaması\n";
                aboutText += "Windows 10/11 uyumlu\n\n";
                aboutText += "© 2025 LMWallpaper Team";
                ErrorHandler::ShowInfoDialog(aboutText, "Hakkında");
            }
            break;
            
        case IDM_EXIT:
            ExitApplication();
            break;
            
        default:
            ErrorHandler::LogInfo("Bilinmeyen menü öğesi: " + std::to_string(menuItemId), InfoLevel::DEBUG);
            break;
    }
}

void TrayManager::ShowSettingsWindow() {
    try {
        if (!g_settingsWindow) {
            g_settingsWindow = std::make_unique<SettingsWindow>();
        }
        g_settingsWindow->Show();
        ErrorHandler::LogInfo("Ayarlar penceresi açıldı", InfoLevel::DEBUG);
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Ayarlar penceresi açılamadı: " + std::string(e.what()), ErrorLevel::ERROR);
        ErrorHandler::ShowErrorDialog("Ayarlar penceresi açılamadı!", "Hata");
    }
}

void TrayManager::TogglePlayback() {
    iconData->isPlaying = !iconData->isPlaying;
    UpdateTrayIcon(iconData->isPlaying);
    
    // TODO: Gerçek video oynatma/durdurma işlemi
    // VideoPlayer instance'ı burada kontrol edilecek
}

void TrayManager::ShowPerformance() {
    std::string perfInfo = "Performans Bilgileri:\n\n";
    
    // Bellek kullanımı
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        double memoryMB = pmc.WorkingSetSize / (1024.0 * 1024.0);
        perfInfo += "Bellek Kullanımı: " + std::to_string(static_cast<int>(memoryMB)) + " MB\n";
    }
    
    // CPU kullanımı (basit)
    perfInfo += "CPU Kullanımı: ~5%\n"; // Placeholder
    perfInfo += "Aktif Frame'ler: 0\n";
    perfInfo += "Video Durumu: " + std::string(iconData->isPlaying ? "Oynatılıyor" : "Duraklatıldı");
    
    ErrorHandler::ShowInfoDialog(perfInfo, "Performans");
}

void TrayManager::ExitApplication() {
    int result = MessageBox(iconData->hWndMain, 
                           L"LMWallpaper'ı kapatmak istediğinizden emin misiniz?",
                           L"Çıkış Onayı", 
                           MB_YESNO | MB_ICONQUESTION);
                           
    if (result == IDYES) {
        ErrorHandler::LogInfo("Uygulama kullanıcı tarafından kapatıldı", InfoLevel::INFO);
        PostMessage(iconData->hWndMain, WM_DESTROY, 0, 0);
    }
}

void TrayManager::UpdatePlayState(bool isPlaying) {
    UpdateTrayIcon(isPlaying);
}