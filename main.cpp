#include "framework.h"
#include "main.h"
#include "Logger.h"
#include "Headers/ErrorHandler.h"
#include "Headers/TrayManager.h"
#include "Headers/SettingsWindow.h"
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h>

// NOTE: Added required headers for GDI+ and COM
#include <gdiplus.h>
#include <objbase.h>

#pragma comment(lib, "gdiplus.lib")

// NOTE: Added namespace for GDI+
using namespace Gdiplus;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hMainWnd;
TrayManager* trayManager;
ErrorHandler errorHandler;
SettingsWindow* settingsWindow;
ULONG_PTR gdiplusToken;

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddToStartup(HWND hWnd);
void RemoveFromStartup(HWND hWnd);
bool IsInStartup();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Prevent multiple instances
    HANDLE hMutex = CreateMutex(NULL, TRUE, L"LMWallpaper_Mutex_Unique_Instance_Check");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, L"LMWallpaper is already running.", L"LMWallpaper", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LMWALLPAPER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        // NOTE: Corrected ErrorHandler call
        errorHandler.HandleError(hr, "CoInitializeEx failed");
        return FALSE;
    }

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        // NOTE: Corrected ErrorHandler call
        errorHandler.HandleError("Failed to initialize instance.", ErrorLevel::CRITICAL);
        return FALSE;
    }

    trayManager = new TrayManager(hMainWnd);
    settingsWindow = new SettingsWindow(hInst, hMainWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    delete trayManager;
    delete settingsWindow;

    // Shutdown GDI+
    GdiplusShutdown(gdiplusToken);

    // Uninitialize COM
    CoUninitialize();

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LMWALLPAPER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_LMWALLPAPER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    hMainWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hMainWnd)
    {
        return FALSE;
    }

    // Hide the main window, as this is a tray application
    // ShowWindow(hMainWnd, nCmdShow);
    // UpdateWindow(hMainWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // Add to startup if not already there
        if (!IsInStartup()) {
            AddToStartup(hWnd);
        }
        break;
    case WM_TRAY_ICON:
        if (lParam == WM_RBUTTONUP)
        {
            trayManager->ShowContextMenu(hWnd);
        }
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_SETTINGS:
            settingsWindow->Show();
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_DESTROY:
        // Remove from startup on exit
        RemoveFromStartup(hWnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void AddToStartup(HWND hWnd) {
    HKEY hKey = NULL;
    const wchar_t* czStartName = L"LMWallpaper";
    wchar_t szPath[MAX_PATH];
    DWORD dwSize = MAX_PATH;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        GetModuleFileName(NULL, szPath, dwSize);
        if (RegSetValueEx(hKey, czStartName, 0, REG_SZ, (BYTE*)szPath, (wcslen(szPath) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS) {
            errorHandler.HandleError("Failed to add to startup.", ErrorLevel::WARNING);
        }
        RegCloseKey(hKey);
    }
}

void RemoveFromStartup(HWND hWnd) {
    HKEY hKey = NULL;
    const wchar_t* czStartName = L"LMWallpaper";

    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegDeleteValue(hKey, czStartName) != ERROR_SUCCESS) {
            // It might not exist, which is fine.
        }
        RegCloseKey(hKey);
    }
}

bool IsInStartup() {
    HKEY hKey = NULL;
    bool exists = false;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, L"LMWallpaper", NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            exists = true;
        }
        RegCloseKey(hKey);
    }
    return exists;
}
