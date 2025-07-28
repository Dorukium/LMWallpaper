// Source/SettingsWindow.cpp
#include "SettingsWindow.h"
#include "framework.h"

SettingsWindow::SettingsWindow() : isDragging(false) {
    CreateModernWindow();
    InitializeD2D();
    InitializeControls();
    SetupWindowShadow();
}

void SettingsWindow::CreateModernWindow() {
    // Modern pencere stili
    DWORD exStyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW;
    DWORD style = WS_OVERLAPPEDWINDOW | WS_MINIMIZEBOX | WS_SYSMENU;

    // Pencere oluştur
    hWnd = CreateWindowEx(
        exStyle,
        L"LMWallpaperSettingsClass",
        L"LMWallpaper Ayarlar",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInst,
        nullptr
    );

    // Modern başlık çubuğu
    SetWindowLongPtr(hWnd, GWL_STYLE, 
        GetWindowLongPtr(hWnd, GWL_STYLE) & ~WS_CAPTION);
}

void SettingsWindow::InitializeD2D() {
    // D2D1 için gerekli nesneleri oluştur
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                      __uuidof(ID2D1Factory1),
                      &factory);
    
    RECT rc;
    GetClientRect(hWnd, &rc);
    
    D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);
    factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hWnd, size),
        &renderTarget
    );
    
    renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
        &brush
    );
}

void SettingsWindow::SetupWindowShadow() {
    // Mavidewa efekti için gölge ayarı
    MARGINS margins = { 0, 0, 0, 0 };
    DwmExtendFrameIntoClientArea(hWnd, &margins);
}

LRESULT SettingsWindow::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_NCCREATE:
            // Modern pencere davranışları
            return DefWindowProc(hWnd, msg, wParam, lParam);

        case WM_NCHITTEST:
            // Sürükle-bırak işlemleri
            if (isDragging) {
                return HTCAPTION;
            }
            break;

        case WM_DROPFILES:
            // Video dosyası sürükle-bırak
            HDROP hDrop = (HDROP)wParam;
            DROPFILES dropFiles;
            DragQueryFile(hDrop, 0, nullptr, 0);
            char fileName[MAX_PATH];
            DragQueryFile(hDrop, 0, fileName, MAX_PATH);
            DragFinish(hDrop);
            videoPreview.GenerateThumbnail(fileName);
            break;

        case WM_DESTROY:
            // Kaynakları temizle
            renderTarget.reset();
            brush.reset();
            PostQuitMessage(0);
            break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}