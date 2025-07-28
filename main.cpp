// main.cpp
#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "TrayManager.h"

// Global değişkenler
static HINSTANCE hInst = nullptr;
static HWND hWndMain = nullptr;
static TrayManager* trayManager = nullptr;

// Windows ana mesaj döngüsü işleyicisi
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        if (lParam == 0)
        {
            // Menü öğesi seçildiğinde
            WORD menuItemId = LOWORD(wParam);
            trayManager->HandleMenuItem(menuItemId);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Tek instance kontrolü için atom
bool CheckInstanceRunning()
{
    static wchar_t szUniqueName[] = L"{LMWALLPAPER_UNIQUE_INSTANCE}";
    HANDLE hMutex = CreateMutex(nullptr, TRUE, szUniqueName);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutex);
        return true;
    }
    return false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Tek instance kontrolü
    if (CheckInstanceRunning())
    {
        MessageBox(nullptr, L"LMWallpaper zaten çalışıyor.", L"Hata", MB_ICONERROR);
        return 0;
    }

    // Windows kayıt defteri kontrolü
    bool autoStart = false;
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dataSize = 0;
        RegQueryValueEx(hKey, L"LMWallpaper", nullptr, nullptr, nullptr, &dataSize);
        RegCloseKey(hKey);
        
        if (dataSize != 0)
            autoStart = true;
    }

    // Ana pencere sınıfı kaydı
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LMWallpaperClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassEx(&wc);

    // Ana pencere oluşturma
    hInst = hInstance;
    hWndMain = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        L"LMWallpaperClass",
        L"LMWallpaper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        200, 150,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    // Tray manager başlatma
    trayManager = new TrayManager(hWndMain);
    if (!trayManager->Initialize(autoStart))
    {
        MessageBox(hWndMain, L"Sistem tepsisinde başlatma hatası!", L"Hata", MB_ICONERROR);
        PostQuitMessage(0);
        return 1;
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Temizlik
    delete trayManager;
    UnregisterClass(L"LMWallpaperClass", hInstance);
    return static_cast<int>(msg.wParams);
}