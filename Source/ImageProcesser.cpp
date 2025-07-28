// Source/ImageProcessor.cpp
#include "../Headers/ImageProcessor.h"

ImageProcessor::ImageProcessor() 
    : pIWICFactory(nullptr)
    , pD2DFactory(nullptr) {
}

ImageProcessor::~ImageProcessor() {
    Cleanup();
}

bool ImageProcessor::Initialize() {
    HRESULT hr = S_OK;
    
    try {
        // WIC Factory oluştur
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                             IID_IWICImagingFactory, (void**)&pIWICFactory);
        if (FAILED(hr)) {
            ErrorHandler::LogError("WIC Factory oluşturulamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
            return false;
        }
        
        // D2D Factory oluştur
        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
        if (FAILED(hr)) {
            ErrorHandler::LogError("D2D Factory oluşturulamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
            return false;
        }
        
        ErrorHandler::LogInfo("ImageProcessor başarıyla başlatıldı", InfoLevel::INFO);
        return true;
        
    } catch (const std::exception& e) {
        ErrorHandler::LogError("ImageProcessor başlatma hatası: " + std::string(e.what()), ErrorLevel::ERROR);
        Cleanup();
        return false;
    }
}

void ImageProcessor::Cleanup() {
    if (pD2DFactory) {
        pD2DFactory->Release();
        pD2DFactory = nullptr;
    }
    
    if (pIWICFactory) {
        pIWICFactory->Release();
        pIWICFactory = nullptr;
    }
    
    ErrorHandler::LogInfo("ImageProcessor temizlendi", InfoLevel::DEBUG);
}

bool ImageProcessor::LoadImageFromFile(const std::wstring& filePath, ID2D1Bitmap** ppBitmap, ID2D1RenderTarget* pRenderTarget) {
    if (!pIWICFactory || !pRenderTarget || !ppBitmap) {
        ErrorHandler::LogError("ImageProcessor başlatılmamış veya geçersiz parametreler", ErrorLevel::ERROR);
        return false;
    }
    
    *ppBitmap = nullptr;
    IWICBitmap* pWICBitmap = nullptr;
    
    try {
        HRESULT hr = CreateWICBitmapFromFile(filePath, &pWICBitmap);
        if (FAILED(hr)) {
            ErrorHandler::LogError("WIC Bitmap oluşturulamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
            return false;
        }
        
        hr = CreateD2DBitmapFromWICBitmap(pWICBitmap, ppBitmap, pRenderTarget);
        if (FAILED(hr)) {
            ErrorHandler::LogError("D2D Bitmap oluşturulamadı: " + ErrorHandler::HRESULTToString(hr), ErrorLevel::ERROR);
            pWICBitmap->Release();
            return false;
        }
        
        pWICBitmap->Release();
        
        std::string filePathStr(filePath.begin(), filePath.end());
        ErrorHandler::LogInfo("Görüntü başarıyla yüklendi: " + filePathStr, InfoLevel::DEBUG);
        return true;
        
    } catch (const std::exception& e) {
        if (pWICBitmap) pWICBitmap->Release();
        ErrorHandler::LogError("Görüntü yükleme hatası: " + std::string(e.what()), ErrorLevel::ERROR);
        return false;
    }
}

bool ImageProcessor::SaveImageToFile(const std::wstring& filePath, ID2D1Bitmap* pBitmap) {
    if (!pIWICFactory || !pBitmap) {
        ErrorHandler::LogError("Geçersiz parametreler", ErrorLevel::ERROR);
        return false;
    }
    
    // Bu fonksiyon gelecekte implement edilecek
    ErrorHandler::LogInfo("SaveImageToFile henüz implement edilmedi", InfoLevel::WARNING);
    return false;
}

bool ImageProcessor::ResizeImage(ID2D1Bitmap* pSource, UINT width, UINT height, ID2D1Bitmap** ppResized, ID2D1RenderTarget* pRenderTarget) {
    if (!pSource || !ppResized || !pRenderTarget) {
        ErrorHandler::LogError("Geçersiz parametreler", ErrorLevel::ERROR);
        return false;
    }
    
    // Bu fonksiyon gelecekte implement edilecek
    ErrorHandler::LogInfo("ResizeImage henüz implement edilmedi", InfoLevel::WARNING);
    return false;
}

bool ImageProcessor::CreateThumbnail(ID2D1Bitmap* pSource, UINT size, ID2D1Bitmap** ppThumbnail, ID2D1RenderTarget* pRenderTarget) {
    return ResizeImage(pSource, size, size, ppThumbnail, pRenderTarget);
}

bool ImageProcessor::ExtractFrameFromVideo(const std::wstring& videoPath, DWORD timeInMS, ID2D1Bitmap** ppFrame, ID2D1RenderTarget* pRenderTarget) {
    if (!ppFrame || !pRenderTarget) {
        ErrorHandler::LogError("Geçersiz parametreler", ErrorLevel::ERROR);
        return false;
    }
    
    // Bu fonksiyon gelecekte video frame extraction için implement edilecek
    ErrorHandler::LogInfo("ExtractFrameFromVideo henüz implement edilmedi", InfoLevel::WARNING);
    return false;
}

bool ImageProcessor::IsSupportedImageFormat(const std::wstring& filePath) {
    if (filePath.empty()) return false;
    
    // Dosya uzantısını kontrol et
    size_t lastDot = filePath.find_last_of(L'.');
    if (lastDot == std::wstring::npos) return false;
    
    std::wstring extension = filePath.substr(lastDot + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    // Desteklenen formatlar
    std::vector<std::wstring> supportedFormats = {
        L"jpg", L"jpeg", L"png", L"bmp", L"gif", L"tiff", L"webp"
    };
    
    return std::find(supportedFormats.begin(), supportedFormats.end(), extension) != supportedFormats.end();
}

bool ImageProcessor::IsSupportedVideoFormat(const std::wstring& filePath) {
    if (filePath.empty()) return false;
    
    // Dosya uzantısını kontrol et
    size_t lastDot = filePath.find_last_of(L'.');
    if (lastDot == std::wstring::npos) return false;
    
    std::wstring extension = filePath.substr(lastDot + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    
    // Desteklenen video formatları
    std::vector<std::wstring> supportedFormats = {
        L"mp4", L"avi", L"mov", L"wmv", L"mkv", L"webm", L"flv", L"m4v"
    };
    
    return std::find(supportedFormats.begin(), supportedFormats.end(), extension) != supportedFormats.end();
}

HRESULT ImageProcessor::CreateWICBitmapFromFile(const std::wstring& filePath, IWICBitmap** ppBitmap) {
    if (!pIWICFactory || !ppBitmap) return E_INVALIDARG;
    
    *ppBitmap = nullptr;
    
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pFrame = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    
    HRESULT hr = S_OK;
    
    try {
        // Decoder oluştur
        hr = pIWICFactory->CreateDecoderFromFilename(
            filePath.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
        );
        if (FAILED(hr)) return hr;
        
        // İlk frame'i al
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr)) return hr;
        
        // Format converter oluştur
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr)) return hr;
        
        // 32bppPBGRA formatına çevir
        hr = pConverter->Initialize(
            pFrame,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeMedianCut
        );
        if (FAILED(hr)) return hr;
        
        // WIC Bitmap oluştur
        hr = pIWICFactory->CreateBitmapFromSource(pConverter, WICBitmapCacheOnLoad, ppBitmap);
        
    } catch (...) {
        hr = E_FAIL;
    }
    
    // Cleanup
    if (pConverter) pConverter->Release();
    if (pFrame) pFrame->Release();
    if (pDecoder) pDecoder->Release();
    
    return hr;
}

HRESULT ImageProcessor::CreateD2DBitmapFromWICBitmap(IWICBitmap* pWICBitmap, ID2D1Bitmap** ppD2DBitmap, ID2D1RenderTarget* pRenderTarget) {
    if (!pWICBitmap || !ppD2DBitmap || !pRenderTarget) return E_INVALIDARG;
    
    *ppD2DBitmap = nullptr;
    
    return pRenderTarget->CreateBitmapFromWicBitmap(pWICBitmap, nullptr, ppD2DBitmap);
}