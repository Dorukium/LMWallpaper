// main.cpp
#include "framework.h"
#include "Headers/resource.h"
#include "Headers/TrayManager.h"
#include "Headers/ErrorHandler.h"
#include "Headers/MemoryOptimizer.h"

// Global değişkenler
HINSTANCE hInst = nullptr;
HWND hWndMain = nullptr;
static std::unique_ptr<TrayManager> trayManager = nullptr;
static std::unique_ptr<MemoryOptimizer> memoryOptimizer = nullptr;

// Forward declarations
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool CheckInstanceRunning();
void InitializeApplication();
void CleanupApplication();

// Windows ana mesaj döngüsü işleyicisi
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (lParam == 0 && trayManager)
        {
            // Menü öğesi seçildiğinde
            WORD menuItemId = LOWORD(wParam);
            trayManager->HandleMenuItem(menuItemId);
        }
        break;

    case WM_TRAY_MESSAGE:
        if (trayManager)
        {
            return trayManager->HandleTrayMessage(wParam, lParam);
        }
        break;

    case WM_DESTROY:
        CleanupApplication();
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        // Pencereyi kapatma yerine gizle
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Tek instance kontrolü için mutex
bool CheckInstanceRunning()
{
    static wchar_t szUniqueName[] = L"{LMWALLPAPER_UNIQUE_INSTANCE_2025}";
    HANDLE hMutex = CreateMutex(nullptr, TRUE, szUniqueName);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex) CloseHandle(hMutex);
        return true;
    }
    return false;
}

void InitializeApplication()
{
    // COM başlatma
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        ErrorHandler::LogError("COM başlatılamadı", ErrorLevel::CRITICAL);
        return;
    }

    // GDI+ başlatma
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    if (status != Gdiplus::Ok) {
        ErrorHandler::LogError("GDI+ başlatılamadı", ErrorLevel::ERROR);
    }

    // Bellek optimizatörü başlat
    try {
        memoryOptimizer = std::make_unique<MemoryOptimizer>();
    } catch (const std::exception& e) {
        ErrorHandler::LogError("MemoryOptimizer başlatılamadı: " + std::string(e.what()), ErrorLevel::ERROR);
    }

    ErrorHandler::LogInfo("Uygulama başlatıldı", InfoLevel::INFO);
}

void CleanupApplication()
{
    ErrorHandler::LogInfo("Uygulama kapatılıyor", InfoLevel::INFO);

    // Kaynakları temizle
    trayManager.reset();
    memoryOptimizer.reset();

    // GDI+ kapatma
    Gdiplus::GdiplusShutdown(0); // Token burada sabit 0 kullanılıyor

    // COM kapatma
    CoUninitialize();
    
    // ErrorHandler cleanup
    ErrorHandler::Cleanup();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Komut satırı argümanlarını kontrol et
    std::string cmdLine(lpCmdLine);
    if (cmdLine.find("--uninstall") != std::string::npos)
    {
        // Uninstall işlemleri
        HKEY hKey;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            RegDeleteValue(hKey, L"LMWallpaper");
            RegCloseKey(hKey);
        }
        return 0;
    }

    // Tek instance kontrolü
    if (CheckInstanceRunning())
    {
        MessageBox(nullptr, L"LMWallpaper zaten çalışıyor.", L"Bilgi", MB_ICONINFORMATION);
        return 0;
    }

    // Uygulama başlatma
    InitializeApplication();

    // Windows kayıt defteri kontrolü (autostart)
    bool autoStart = false;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dataSize = 0;
        if (RegQueryValueEx(hKey, L"LMWallpaper", nullptr, nullptr, nullptr, &dataSize) == ERROR_SUCCESS && dataSize > 0)
        {
            autoStart = true;
        }
        RegCloseKey(hKey);
    }

    // Ana pencere sınıfı kaydı
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LMWallpaperClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LMWALLPAPER));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    if (!RegisterClassEx(&wc))
    {
        ErrorHandler::LogError("Pencere sınıfı kayıt edilemedi", ErrorLevel::CRITICAL);
        CleanupApplication();
        return 1;
    }

    // Ana pencere oluşturma (gizli)
    hInst = hInstance;
    hWndMain = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"LMWallpaperClass",
        L"LMWallpaper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        300, 200,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hWndMain)
    {
        ErrorHandler::LogError("Ana pencere oluşturulamadı", ErrorLevel::CRITICAL);
        CleanupApplication();
        return 1;
    }

    // Pencereyi gizle (sadece sistem tepsisinde çalış)
    ShowWindow(hWndMain, SW_HIDE);

    // Tray manager başlatma
    try {
        trayManager = std::make_unique<TrayManager>(hWndMain);
        if (!trayManager->Initialize(autoStart))
        {
            MessageBox(hWndMain, L"Sistem tepsisinde başlatma hatası!", L"Hata", MB_ICONERROR);
            CleanupApplication();
            return 1;
        }
    } catch (const std::exception& e) {
        ErrorHandler::LogError("TrayManager başlatılamadı: " + std::string(e.what()), ErrorLevel::CRITICAL);
        CleanupApplication();
        return 1;
    }

    // Install parametresi varsa autostart ayarla
    if (cmdLine.find("--install") != std::string::npos || cmdLine.find("--autostart") != std::string::npos)
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            wchar_t exePath[MAX_PATH];
            GetModuleFileName(nullptr, exePath, MAX_PATH);
            std::wstring startupValue = std::wstring(exePath) + L" --autostart";
            
            RegSetValueEx(hKey, L"LMWallpaper", 0, REG_SZ, 
                         (BYTE*)startupValue.c_str(), 
                         (DWORD)(startupValue.length() + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
        }
    }

    ErrorHandler::LogInfo("LMWallpaper başlatıldı", InfoLevel::INFO);

    // Ana mesaj döngüsü
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Temizlik
    CleanupApplication();
    UnregisterClass(L"LMWallpaperClass", hInstance);
    
    return static_cast<int>(msg.wParam);
}