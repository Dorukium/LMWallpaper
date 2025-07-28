// Source/SettingsWindow.cpp
#include "../Headers/SettingsWindow.h"

SettingsWindow::SettingsWindow() : hWnd(nullptr), isDragging(false), dragOffset{0, 0} {
    CreateModernWindow();
    InitializeD2D();
    InitializeControls();
    SetupWindowShadow();
}

SettingsWindow::~SettingsWindow() {
    if (brush) {
        brush.reset();
    }
    if (renderTarget) {
        renderTarget.reset();
    }
    if (hWnd) {
        DestroyWindow(hWnd);
    }
}

void SettingsWindow::CreateModernWindow() {
    // Pencere sınıfını kaydet
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEX wc = {0};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = [](HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
            SettingsWindow* pThis = nullptr;
            
            if (msg == WM_NCCREATE) {
                LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
                pThis = (SettingsWindow*)pcs->lpCreateParams;
                SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            } else {
                pThis = (SettingsWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            }
            
            if (pThis) {
                return pThis->MessageHandler(hWnd, msg, wParam, lParam);
            }
            
            return DefWindowProc(hWnd, msg, wParam, lParam);
        };
        wc.hInstance = hInst;
        wc.lpszClassName = L"LMWallpaperSettingsClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr; // D2D kullanacağız
        wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LMWALLPAPER));
        wc.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
        
        RegisterClassEx(&wc);
        classRegistered = true;
    }

    // Modern pencere stili
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW;
    DWORD style = WS_POPUP | WS_VISIBLE;

    // Pencere oluştur
    hWnd = CreateWindowEx(
        exStyle,
        L"LMWallpaperSettingsClass",
        L"LMWallpaper Ayarları",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInst,
        this  // this pointer'ı WM_NCCREATE'de kullanılacak
    );

    if (!hWnd) {
        ErrorHandler::LogError("Ayarlar penceresi oluşturulamadı", ErrorLevel::CRITICAL);
        return;
    }

    // Layered window özelliklerini ayarla
    SetLayeredWindowAttributes(hWnd, 0, 240, LWA_ALPHA); // %94 opaklık
}

void SettingsWindow::InitializeD2D() {
    if (!hWnd) return;

    HRESULT hr = S_OK;
    ID2D1Factory* pFactory = nullptr;
    
    try {
        // D2D1 Factory oluştur
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
        if (FAILED(hr)) {
            throw std::runtime_error("D2D Factory oluşturulamadı");
        }
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        
        D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
        
        // Render target oluştur
        ID2D1HwndRenderTarget* pRenderTarget = nullptr;
        hr = pFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hWnd, size),
            &pRenderTarget
        );
        
        if (FAILED(hr)) {
            throw std::runtime_error("Render Target oluşturulamadı");
        }
        
        renderTarget.reset(pRenderTarget);
        
        // Brush oluştur
        ID2D1SolidColorBrush* pBrush = nullptr;
        hr = renderTarget->CreateSolidColorBrush(
            D2D1::ColorF(0.2f, 0.2f, 0.2f, 1.0f), // Koyu gri
            &pBrush
        );
        
        if (FAILED(hr)) {
            throw std::runtime_error("Brush oluşturulamadı");
        }
        
        brush.reset(pBrush);
        
        if (pFactory) {
            pFactory->Release();
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("D2D başlatma hatası: " + std::string(e.what()), ErrorLevel::ERROR);
        if (pFactory) pFactory->Release();
    }
}

void SettingsWindow::SetupWindowShadow() {
    if (!hWnd) return;

    // DWM kompozisyon etkinse gölge efekti ekle
    BOOL dwmEnabled = FALSE;
    if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled) {
        MARGINS margins = {0};
        DwmExtendFrameIntoClientArea(hWnd, &margins);
        
        // Gölge efekti için window attribute ayarla
        DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
        DwmSetWindowAttribute(hWnd, DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    }
}

void SettingsWindow::InitializeControls() {
    if (!hWnd) return;

    // Video dosyası seçimi
    CreateWindow(L"STATIC", L"Video Dosyası:", WS_VISIBLE | WS_CHILD,
                20, 20, 100, 20, hWnd, nullptr, hInst, nullptr);
    
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
                20, 45, 600, 25, hWnd, (HMENU)IDC_VIDEO_PATH, hInst, nullptr);
    
    CreateWindow(L"BUTTON", L"Gözat...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                630, 45, 80, 25, hWnd, (HMENU)IDC_BROWSE_VIDEO, hInst, nullptr);

    // Monitör seçimi
    CreateWindow(L"STATIC", L"Monitör:", WS_VISIBLE | WS_CHILD,
                20, 90, 100, 20, hWnd, nullptr, hInst, nullptr);
    
    CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                20, 115, 200, 100, hWnd, (HMENU)IDC_MONITOR_LIST, hInst, nullptr);

    // Ses seviyesi
    CreateWindow(L"STATIC", L"Ses Seviyesi:", WS_VISIBLE | WS_CHILD,
                20, 160, 100, 20, hWnd, nullptr, hInst, nullptr);
    
    CreateWindow(L"msctls_trackbar32", L"", WS_VISIBLE | WS_CHILD | TBS_HORZ | TBS_AUTOTICKS,
                20, 185, 300, 30, hWnd, (HMENU)IDC_VOLUME_SLIDER, hInst, nullptr);

    // Otomatik başlatma
    CreateWindow(L"BUTTON", L"Windows ile otomatik başlat", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                20, 235, 250, 20, hWnd, (HMENU)IDC_AUTOSTART, hInst, nullptr);

    // Butonlar
    CreateWindow(L"BUTTON", L"Tamam", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                620, 520, 80, 30, hWnd, (HMENU)IDC_OK, hInst, nullptr);
    
    CreateWindow(L"BUTTON", L"İptal", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                710, 520, 80, 30, hWnd, (HMENU)IDC_CANCEL, hInst, nullptr);

    // Monitör listesini doldur
    PopulateMonitorList();
    
    // Ayarları yükle
    LoadSettings();
}

void SettingsWindow::PopulateMonitorList() {
    HWND hCombo = GetDlgItem(hWnd, IDC_MONITOR_LIST);
    if (!hCombo) return;

    // Mevcut öğeleri temizle
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    // Monitörleri enumerate et
    EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
        HWND hCombo = (HWND)dwData;
        
        MONITORINFOEX mi;
        mi.cbSize = sizeof(mi);
        
        if (GetMonitorInfo(hMonitor, &mi)) {
            std::wstring monitorName = mi.szDevice;
            monitorName += L" (";
            monitorName += std::to_wstring(lprcMonitor->right - lprcMonitor->left);
            monitorName += L"x";
            monitorName += std::to_wstring(lprcMonitor->bottom - lprcMonitor->top);
            monitorName += L")";
            
            int index = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)monitorName.c_str());
            SendMessage(hCombo, CB_SETITEMDATA, index, (LPARAM)hMonitor);
        }
        
        return TRUE;
    }, (LPARAM)hCombo);

    // İlk monitörü seç
    SendMessage(hCombo, CB_SETCURSEL, 0, 0);
}

void SettingsWindow::LoadSettings() {
    // Registry'den ayarları yükle
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\LMWallpaper", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        // Video dosyası yolu
        wchar_t videoPath[MAX_PATH] = {0};
        DWORD size = sizeof(videoPath);
        if (RegQueryValueEx(hKey, L"VideoPath", nullptr, nullptr, (BYTE*)videoPath, &size) == ERROR_SUCCESS) {
            SetDlgItemText(hWnd, IDC_VIDEO_PATH, videoPath);
        }
        
        // Ses seviyesi
        DWORD volume = 50; // Varsayılan %50
        size = sizeof(volume);
        if (RegQueryValueEx(hKey, L"Volume", nullptr, nullptr, (BYTE*)&volume, &size) == ERROR_SUCCESS) {
            SendDlgItemMessage(hWnd, IDC_VOLUME_SLIDER, TBM_SETPOS, TRUE, volume);
        }
        
        // Otomatik başlatma
        DWORD autoStart = 0;
        size = sizeof(autoStart);
        if (RegQueryValueEx(hKey, L"AutoStart", nullptr, nullptr, (BYTE*)&autoStart, &size) == ERROR_SUCCESS) {
            CheckDlgButton(hWnd, IDC_AUTOSTART, autoStart ? BST_CHECKED : BST_UNCHECKED);
        }
        
        RegCloseKey(hKey);
    } else {
        // Varsayılan değerler
        SendDlgItemMessage(hWnd, IDC_VOLUME_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendDlgItemMessage(hWnd, IDC_VOLUME_SLIDER, TBM_SETPOS, TRUE, 50);
        SendDlgItemMessage(hWnd, IDC_VOLUME_SLIDER, TBM_SETTICFREQ, 10, 0);
    }
}

void SettingsWindow::SaveSettings() {
    // Registry'ye ayarları kaydet
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\LMWallpaper", 0, nullptr, 
                      REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        
        // Video dosyası yolu
        wchar_t videoPath[MAX_PATH];
        GetDlgItemText(hWnd, IDC_VIDEO_PATH, videoPath, MAX_PATH);
        RegSetValueEx(hKey, L"VideoPath", 0, REG_SZ, (BYTE*)videoPath, 
                     (DWORD)(wcslen(videoPath) + 1) * sizeof(wchar_t));
        
        // Ses seviyesi
        DWORD volume = (DWORD)SendDlgItemMessage(hWnd, IDC_VOLUME_SLIDER, TBM_GETPOS, 0, 0);
        RegSetValueEx(hKey, L"Volume", 0, REG_DWORD, (BYTE*)&volume, sizeof(volume));
        
        // Otomatik başlatma
        DWORD autoStart = IsDlgButtonChecked(hWnd, IDC_AUTOSTART) == BST_CHECKED ? 1 : 0;
        RegSetValueEx(hKey, L"AutoStart", 0, REG_DWORD, (BYTE*)&autoStart, sizeof(autoStart));
        
        RegCloseKey(hKey);
        
        // Otomatik başlatma ayarını Windows registry'sinde de güncelle
        HKEY hRunKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                        0, KEY_WRITE, &hRunKey) == ERROR_SUCCESS) {
            if (autoStart) {
                wchar_t exePath[MAX_PATH];
                GetModuleFileName(nullptr, exePath, MAX_PATH);
                std::wstring startupValue = std::wstring(exePath) + L" --autostart";
                RegSetValueEx(hRunKey, L"LMWallpaper", 0, REG_SZ, 
                             (BYTE*)startupValue.c_str(), 
                             (DWORD)(startupValue.length() + 1) * sizeof(wchar_t));
            } else {
                RegDeleteValue(hRunKey, L"LMWallpaper");
            }
            RegCloseKey(hRunKey);
        }
    }
}

void SettingsWindow::Show() {
    if (hWnd) {
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
void SettingsWindow::Show() {
    if (hWnd) {
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
        UpdateWindow(hWnd);
    }
}

void SettingsWindow::Hide() {
    if (hWnd) {
        ShowWindow(hWnd, SW_HIDE);
    }
}

void SettingsWindow::Update() {
    if (hWnd && renderTarget) {
        InvalidateRect(hWnd, nullptr, FALSE);
    }
}

LRESULT SettingsWindow::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT:
            {
                if (renderTarget && brush) {
                    renderTarget->BeginDraw();
                    
                    // Arka plan temizle
                    renderTarget->Clear(D2D1::ColorF(0.15f, 0.15f, 0.15f, 1.0f));
                    
                    // Başlık çubuğu çiz
                    D2D1_RECT_F titleRect = D2D1::RectF(0, 0, 800, 40);
                    brush->SetColor(D2D1::ColorF(0.1f, 0.1f, 0.1f, 1.0f));
                    renderTarget->FillRectangle(titleRect, brush.get());
                    
                    // Başlık metni (basit çizim)
                    brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
                    
                    HRESULT hr = renderTarget->EndDraw();
                    if (FAILED(hr)) {
                        ErrorHandler::LogError("Render target draw hatası", ErrorLevel::WARNING);
                    }
                }
                
                // Varsayılan paint işlemini de çağır
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
            break;

        case WM_COMMAND:
            {
                WORD cmdId = LOWORD(wParam);
                switch (cmdId) {
                    case IDC_BROWSE_VIDEO:
                        {
                            OPENFILENAME ofn = {0};
                            wchar_t szFile[MAX_PATH] = {0};
                            
                            ofn.lStructSize = sizeof(ofn);
                            ofn.hwndOwner = hWnd;
                            ofn.lpstrFile = szFile;
                            ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);
                            ofn.lpstrFilter = L"Video Dosyaları\0*.mp4;*.avi;*.mov;*.wmv;*.mkv;*.webm\0Tüm Dosyalar\0*.*\0";
                            ofn.nFilterIndex = 1;
                            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                            
                            if (GetOpenFileName(&ofn)) {
                                SetDlgItemText(hWnd, IDC_VIDEO_PATH, szFile);
                            }
                        }
                        break;
                        
                    case IDC_OK:
                        SaveSettings();
                        Hide();
                        MessageBox(hWnd, L"Ayarlar kaydedildi.", L"Bilgi", MB_ICONINFORMATION);
                        break;
                        
                    case IDC_CANCEL:
                        Hide();
                        break;
                }
            }
            break;

        case WM_CLOSE:
            Hide();
            return 0;

        case WM_NCHITTEST:
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ScreenToClient(hWnd, &pt);
                
                // Başlık çubuğu alanında sürükleme için
                if (pt.y >= 0 && pt.y <= 40) {
                    return HTCAPTION;
                }
                
                return HTCLIENT;
            }
            break;

        case WM_LBUTTONDOWN:
            {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                if (pt.y <= 40) { // Başlık çubuğu alanı
                    isDragging = true;
                    dragOffset.x = pt.x;
                    dragOffset.y = pt.y;
                    SetCapture(hWnd);
                }
            }
            break;

        case WM_LBUTTONUP:
            if (isDragging) {
                isDragging = false;
                ReleaseCapture();
            }
            break;

        case WM_MOUSEMOVE:
            if (isDragging && (wParam & MK_LBUTTON)) {
                POINT cursor;
                GetCursorPos(&cursor);
                
                RECT windowRect;
                GetWindowRect(hWnd, &windowRect);
                
                SetWindowPos(hWnd, nullptr,
                           cursor.x - dragOffset.x,
                           cursor.y - dragOffset.y,
                           0, 0,
                           SWP_NOSIZE | SWP_NOZORDER);
            }
            break;

        case WM_SIZE:
            if (renderTarget) {
                D2D1_SIZE_U size = D2D1::SizeU(LOWORD(lParam), HIWORD(lParam));
                renderTarget->Resize(size);
            }
            break;

        case WM_DESTROY:
            // Pencere kapatılırken kaynakları temizle
            brush.reset();
            renderTarget.reset();
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    
    return 0;
}