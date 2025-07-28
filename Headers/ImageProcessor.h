// Headers/ImageProcessor.h
#pragma once

#include "framework.h"
#include "ErrorHandler.h"

class ImageProcessor {
private:
    IWICImagingFactory* pIWICFactory;
    ID2D1Factory* pD2DFactory;
    
public:
    ImageProcessor();
    ~ImageProcessor();
    
    bool Initialize();
    void Cleanup();
    
    // Görüntü yükleme ve kaydetme
    bool LoadImageFromFile(const std::wstring& filePath, ID2D1Bitmap** ppBitmap, ID2D1RenderTarget* pRenderTarget);
    bool SaveImageToFile(const std::wstring& filePath, ID2D1Bitmap* pBitmap);
    
    // Görüntü işleme fonksiyonları
    bool ResizeImage(ID2D1Bitmap* pSource, UINT width, UINT height, ID2D1Bitmap** ppResized, ID2D1RenderTarget* pRenderTarget);
    bool CreateThumbnail(ID2D1Bitmap* pSource, UINT size, ID2D1Bitmap** ppThumbnail, ID2D1RenderTarget* pRenderTarget);
    
    // Video frame'den görüntü çıkarma
    bool ExtractFrameFromVideo(const std::wstring& videoPath, DWORD timeInMS, ID2D1Bitmap** ppFrame, ID2D1RenderTarget* pRenderTarget);
    
    // Destek kontrolü
    bool IsSupportedImageFormat(const std::wstring& filePath);
    bool IsSupportedVideoFormat(const std::wstring& filePath);
    
private:
    HRESULT CreateWICBitmapFromFile(const std::wstring& filePath, IWICBitmap** ppBitmap);
    HRESULT CreateD2DBitmapFromWICBitmap(IWICBitmap* pWICBitmap, ID2D1Bitmap** ppD2DBitmap, ID2D1RenderTarget* pRenderTarget);
};