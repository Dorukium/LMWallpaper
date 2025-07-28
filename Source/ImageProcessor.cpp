#include "../Headers/ImageProcessor.h"

ImageProcessor::ImageProcessor() : m_pWICFactory(NULL), m_pRenderTarget(NULL) {
    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        // Create WIC factory
        hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&m_pWICFactory)
        );
    }
    if (FAILED(hr)) {
        // Handle error
    }
}

ImageProcessor::~ImageProcessor() {
    if (m_pWICFactory) m_pWICFactory->Release();
    if (m_pRenderTarget) m_pRenderTarget->Release();
    CoUninitialize();
}

ID2D1Bitmap* ImageProcessor::LoadImage(const std::wstring& filePath) {
    if (!m_pWICFactory || !m_pRenderTarget) return NULL;

    IWICBitmapDecoder* pDecoder = NULL;
    IWICBitmapFrameDecode* pSource = NULL;
    IWICFormatConverter* pConverter = NULL;
    ID2D1Bitmap* pBitmap = NULL;

    HRESULT hr = m_pWICFactory->CreateDecoderFromFilename(
        filePath.c_str(),
        NULL,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }

    if (SUCCEEDED(hr)) {
        hr = m_pWICFactory->CreateFormatConverter(&pConverter);
    }

    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(
            pSource,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            NULL,
            0.f,
            WICBitmapPaletteTypeMedianCut
        );
    }

    if (SUCCEEDED(hr)) {
        hr = m_pRenderTarget->CreateBitmapFromWicBitmap(
            pConverter,
            NULL,
            &pBitmap
        );
    }

    if (pDecoder) pDecoder->Release();
    if (pSource) pSource->Release();
    if (pConverter) pConverter->Release();

    return pBitmap;
}

void ImageProcessor::SetRenderTarget(ID2D1HwndRenderTarget* pRenderTarget) {
    m_pRenderTarget = pRenderTarget;
}
