// Source/ImageProcessor.cpp
#include "../Headers/ImageProcessor.h"

ImageProcessor::ImageProcessor() : pIWICFactory(nullptr), pD2DFactory(nullptr) {
}

ImageProcessor::~ImageProcessor() {
    Cleanup();
}

bool ImageProcessor::Initialize() {
    HRESULT hr = S_OK;
    
    // WIC Factory oluştur
    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pIWICFactory)
    );
    
    if (FAILED(hr)) {
        ErrorHandler::LogError("WIC Factory oluşturulamadı", ErrorLevel::CRITICAL);
        return false;
    }
    
    // D2D Factory oluştur
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
    
    if (FAILED(hr)) {
        ErrorHandler::LogError("D2D Factory oluşturulamadı", ErrorLevel::CRITICAL);
        return false;
    }
    
    return true;
}

void ImageProcessor::Cleanup() {
    if (pIWICFactory) {
        pIWICFactory->Release();
        pIWICFactory = nullptr;
    }
    
    if (pD2DFactory) {
        pD2DFactory->Release();
        pD2DFactory = nullptr;
    }
}

bool ImageProcessor::LoadImageFromFile(const std::wstring& filePath, ID2D1Bitmap** ppBitmap, ID2D1RenderTarget* pRenderTarget) {
    if (!pIWICFactory || !pRenderTarget) {
        return false;
    }
    
    HRESULT hr = S_OK;
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pFrame = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    
    try {
        // Dosyadan decoder oluştur
        hr = pIWICFactory->CreateDecoderFromFilename(
            filePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
        );
        
        if (FAILED(hr)) {
            throw std::runtime_error("Decoder oluşturulamadı");
        }
        
        // İlk frame'i al
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr)) {
            throw std::runtime_error("Frame alınamadı");
        }
        
        // Format converter oluştur
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr)) {
            throw std::runtime_error("Format converter oluşturulamadı");
        }
        
        // 32bppPBGRA formatına çevir
        hr = pConverter->Initialize(
            pFrame,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeMedianCut
        );
        
        if (FAILED(hr)) {
            throw std::runtime_error("Format dönüştürme başarısız");
        }
        
        // D2D Bitmap oluştur
        hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, ppBitmap);
        if (FAILED(hr)) {
            throw std::runtime_error("D2D Bitmap oluşturulamadı");
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Görüntü yükleme hatası: " + std::string(e.what()), ErrorLevel::ERROR);
    }
    
    // Temizlik
    if (pConverter) pConverter->Release();
    if (pFrame) pFrame->Release();
    if (pDecoder) pDecoder->Release();
    
    return SUCCEEDED(hr);
}

bool ImageProcessor::SaveImageToFile(const std::wstring& filePath, ID2D1Bitmap* pBitmap) {
    if (!pIWICFactory || !pBitmap) {
        return false;
    }
    
    HRESULT hr = S_OK;
    IWICBitmapEncoder* pEncoder = nullptr;
    IWICBitmapFrameEncode* pFrameEncode = nullptr;
    IWICStream* pStream = nullptr;
    
    try {
        // Stream oluştur
        hr = pIWICFactory->CreateStream(&pStream);
        if (FAILED(hr)) {
            throw std::runtime_error("Stream oluşturulamadı");
        }
        
        // Dosya için stream başlat
        hr = pStream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE);
        if (FAILED(hr)) {
            throw std::runtime_error("Dosya stream başlatılamadı");
        }
        
        // PNG encoder oluştur
        hr = pIWICFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEncoder);
        if (FAILED(hr)) {
            throw std::runtime_error("PNG encoder oluşturulamadı");
        }
        
        hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
        if (FAILED(hr)) {
            throw std::runtime_error("Encoder başlatılamadı");
        }
        
        // Frame oluştur
        hr = pEncoder->CreateNewFrame(&pFrameEncode, nullptr);
        if (FAILED(hr)) {
            throw std::runtime_error("Frame oluşturulamadı");
        }
        
        hr = pFrameEncode->Initialize(nullptr);
        if (FAILED(hr)) {
            throw std::runtime_error("Frame başlatılamadı");
        }
        
        // Bitmap boyutlarını al
        D2D1_SIZE_U size = pBitmap->GetPixelSize();
        hr = pFrameEncode->SetSize(size.width, size.height);
        if (FAILED(hr)) {
            throw std::runtime_error("Frame boyutu ayarlanamadı");
        }
        
        // Pixel format ayarla
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
        hr = pFrameEncode->SetPixelFormat(&format);
        if (FAILED(hr)) {
            throw std::runtime_error("Pixel format ayarlanamadı");
        }
        
        // Bitmap verilerini kopyala
        const UINT stride = size.width * 4;
        const UINT bufferSize = stride * size.height;
        std::unique_ptr<BYTE[]> buffer(new BYTE[bufferSize]);
        
        D2D1_RECT_U rect = D2D1::RectU(0, 0, size.width, size.height);
        hr = pBitmap->CopyFromMemory(&rect, buffer.get(), stride);
        if (FAILED(hr)) {
            throw std::runtime_error("Bitmap verisi kopyalanamadı");
        }
        
        hr = pFrameEncode->WritePixels(size.height, stride, bufferSize, buffer.get());
        if (FAILED(hr)) {
            throw std::runtime_error("Pixel verileri yazılamadı");
        }
        
        hr = pFrameEncode->Commit();
        if (FAILED(hr)) {
            throw std::runtime_error("Frame commit edilemedi");
        }
        
        hr = pEncoder->Commit();
        if (FAILED(hr)) {
            throw std::runtime_error("Encoder commit edilemedi");
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Görüntü kaydetme hatası: " + std::string(e.what()), ErrorLevel::ERROR);
    }
    
    // Temizlik
    if (pFrameEncode) pFrameEncode->Release();
    if (pEncoder) pEncoder->Release();
    if (pStream) pStream->Release();
    
    return SUCCEEDED(hr);
}

bool ImageProcessor::ResizeImage(ID2D1Bitmap* pSource, UINT width, UINT height, ID2D1Bitmap** ppResized, ID2D1RenderTarget* pRenderTarget) {
    if (!pSource || !pRenderTarget) {
        return false;
    }
    
    HRESULT hr = S_OK;
    ID2D1BitmapRenderTarget* pBitmapRenderTarget = nullptr;
    
    try {
        // Bitmap render target oluştur
        hr = pRenderTarget->CreateCompatibleRenderTarget(
            D2D1::SizeF(static_cast<float>(width), static_cast<float>(height)),
            &pBitmapRenderTarget
        );
        
        if (FAILED(hr)) {
            throw std::runtime_error("Bitmap render target oluşturulamadı");
        }
        
        pBitmapRenderTarget->BeginDraw();
        
        // Yeniden boyutlandırarak çiz
        D2D1_RECT_F destRect = D2D1::RectF(0, 0, static_cast<float>(width), static_cast<float>(height));
        pBitmapRenderTarget->DrawBitmap(pSource, destRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        
        hr = pBitmapRenderTarget->EndDraw();
        if (FAILED(hr)) {
            throw std::runtime_error("Render target end draw başarısız");
        }
        
        // Sonuç bitmap'ini al
        hr = pBitmapRenderTarget->GetBitmap(ppResized);
        if (FAILED(hr)) {
            throw std::runtime_error("Sonuç bitmap'i alınamadı");
        }
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("Görüntü yeniden boyutlandırma hatası: " + std::string(e.what()), ErrorLevel::ERROR);
    }
    
    if (pBitmapRenderTarget) {
        pBitmapRenderTarget->Release();
    }
    
    return SUCCEEDED(hr);
}

bool ImageProcessor::CreateThumbnail(ID2D1Bitmap* pSource, UINT size, ID2D1Bitmap** ppThumbnail, ID2D1RenderTarget* pRenderTarget) {
    return ResizeImage(pSource, size, size, ppThumbnail, pRenderTarget);
}

bool ImageProcessor::ExtractFrameFromVideo(const std::wstring& videoPath, DWORD timeInMS, ID2D1Bitmap** ppFrame, ID2D1RenderTarget* pRenderTarget) {
    // Bu fonksiyon Media Foundation kullanarak video frame çıkarımı yapacak
    // Şimdilik basit bir implementasyon
    ErrorHandler::LogInfo("Video frame çıkarımı henüz implementasyonu yapılmadı", InfoLevel::DEBUG);
    return false;
}

bool ImageProcessor::IsSupportedImageFormat(const std::wstring& filePath) {
    std::wstring extension = std::filesystem::path(filePath).extension();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    return (extension == L".jpg" || extension == L".jpeg" || 
            extension == L".png" || extension == L".bmp" || 
            extension == L".gif" || extension == L".tiff");
}

bool ImageProcessor::IsSupportedVideoFormat(const std::wstring& filePath) {
    std::wstring extension = std::filesystem::path(filePath).extension();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    return (extension == L".mp4" || extension == L".avi" || 
            extension == L".mov" || extension == L".wmv" || 
            extension == L".mkv" || extension == L".webm");
}