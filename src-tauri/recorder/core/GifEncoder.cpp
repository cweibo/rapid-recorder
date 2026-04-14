#include "GifEncoder.h"

GifEncoder::GifEncoder() {}

GifEncoder::~GifEncoder() {
    Finalize();
}

HRESULT GifEncoder::Initialize(const std::wstring& filename, UINT width, UINT height) {
    m_origWidth = width;
    m_origHeight = height;
    
    // Performance: Downscale to 1280 base width if source is larger
    if (width > 1280) {
        float ratio = (float)height / width;
        m_width = 1280;
        m_height = (UINT)(1280 * ratio);
    } else {
        m_width = width;
        m_height = height;
    }

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr)) return hr;

    hr = m_factory->CreateStream(&m_stream);
    if (FAILED(hr)) return hr;

    hr = m_stream->InitializeFromFilename(filename.c_str(), GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    hr = m_factory->CreateEncoder(GUID_ContainerFormatGif, NULL, &m_encoder);
    if (FAILED(hr)) return hr;

    hr = m_encoder->Initialize(m_stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) return hr;

    m_flippedBuffer.resize(m_origWidth * m_origHeight * 4);
    m_isInitialized = true;
    Log("GifEncoder Initialized: " + std::to_string(m_width) + "x" + std::to_string(m_height) + " (Source: " + std::to_string(m_origWidth) + "x" + std::to_string(m_origHeight) + ")");
    return S_OK;
}

bool GifEncoder::WriteFrame(const BYTE* pData, UINT stride) {
    if (!m_isInitialized) return false;

    // 1. Direct linear copy (remove flip as report says it's inverted)
    memcpy(m_flippedBuffer.data(), pData, m_origWidth * m_origHeight * 4);

    // 2. Create WIC bitmap from the flipped memory
    ComPtr<IWICBitmap> bitmap;
    HRESULT hr = m_factory->CreateBitmapFromMemory(
        m_origWidth, m_origHeight, 
        GUID_WICPixelFormat32bppBGRA, 
        m_origWidth * 4, 
        (UINT)m_flippedBuffer.size(), 
        m_flippedBuffer.data(), 
        &bitmap
    );
    if (FAILED(hr)) return false;

    // 3. Create Scaler if needed
    ComPtr<IWICBitmapSource> source;
    if (m_origWidth != m_width) {
        ComPtr<IWICBitmapScaler> scaler;
        hr = m_factory->CreateBitmapScaler(&scaler);
        if (FAILED(hr)) return false;
        
        hr = scaler->Initialize(bitmap.Get(), m_width, m_height, WICBitmapInterpolationModeFant);
        if (FAILED(hr)) return false;
        
        source = scaler;
    } else {
        source = bitmap;
    }

    // 4. Encode Frame
    ComPtr<IWICBitmapFrameEncode> frameEncode;
    hr = m_encoder->CreateNewFrame(&frameEncode, NULL);
    if (FAILED(hr)) return false;

    hr = frameEncode->Initialize(NULL);
    if (FAILED(hr)) return false;

    hr = frameEncode->SetSize(m_width, m_height);
    if (FAILED(hr)) return false;

    WICPixelFormatGUID format = GUID_WICPixelFormat8bppIndexed; // GIF standard
    hr = frameEncode->SetPixelFormat(&format);
    if (FAILED(hr)) return false;

    hr = frameEncode->WriteSource(source.Get(), NULL);
    if (FAILED(hr)) return false;

    hr = frameEncode->Commit();
    if (FAILED(hr)) return false;

    return true;
}

void GifEncoder::Finalize() {
    if (m_isInitialized && m_encoder) {
        m_encoder->Commit();
        m_encoder.Reset();
        m_stream.Reset();
        m_factory.Reset(); // Clear factory
        m_isInitialized = false;
        Log("GifEncoder Finalized");
    }
}
