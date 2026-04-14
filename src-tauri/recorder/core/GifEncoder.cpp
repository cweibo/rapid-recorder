#include "GifEncoder.h"

GifEncoder::GifEncoder() {}

GifEncoder::~GifEncoder() {
    Finalize();
}

HRESULT GifEncoder::Initialize(const std::wstring& filename, UINT width, UINT height) {
    m_origWidth = width;
    m_origHeight = height;
    
    // Performance: Downscale to 800 base width if source is larger (Optimized for size)
    if (width > 800) {
        float ratio = (float)height / width;
        m_width = 800;
        m_height = (UINT)(800 * ratio);
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

    // Set Global Metadata: LoopCount (0 = infinite)
    ComPtr<IWICMetadataQueryWriter> metaWriter;
    if (SUCCEEDED(m_encoder->GetMetadataQueryWriter(&metaWriter))) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        pv.vt = VT_UI2;
        pv.uiVal = 0;
        metaWriter->SetMetadataByName(L"/appext/Application", &pv); // May fail on some systems, continue anyway
        
        // Loop count specifically
        PropVariantInit(&pv);
        pv.vt = VT_UI1 | VT_ARRAY;
        // NETSCAPE2.0 loop block
        SAFEARRAY* sa = SafeArrayCreateVector(VT_UI1, 0, 11);
        if (sa) {
            void* pData;
            SafeArrayAccessData(sa, &pData);
            memcpy(pData, "NETSCAPE2.0", 11);
            SafeArrayUnaccessData(sa);
            pv.parray = sa;
            metaWriter->SetMetadataByName(L"/appext/Data", &pv);
            SafeArrayDestroy(sa);
        }
    }

    m_flippedBuffer.resize(m_origWidth * m_origHeight * 4);
    m_isInitialized = true;
    m_hasPalette = false;
    Log("GifEncoder Optimized: " + std::to_string(m_width) + "x" + std::to_string(m_height) + " (Source: " + std::to_string(m_origWidth) + "x" + std::to_string(m_origHeight) + ")");
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

    // 5. Manage Palette (Crucial for compression)
    if (!m_hasPalette) {
        hr = m_factory->CreatePalette(&m_palette);
        if (SUCCEEDED(hr)) {
            // Generate palette from the first frame
            hr = m_palette->InitializeFromBitmap(source.Get(), 256, FALSE);
            if (SUCCEEDED(hr)) {
                m_hasPalette = true;
                m_encoder->SetPalette(m_palette.Get());
            }
        }
    }

    if (m_hasPalette) {
        frameEncode->SetPalette(m_palette.Get());
    }

    // 6. Color Quantization: Use Format Converter (Crucial to avoid black screen)
    ComPtr<IWICFormatConverter> converter;
    hr = m_factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr)) {
        hr = converter->Initialize(
            source.Get(), 
            GUID_WICPixelFormat8bppIndexed, 
            WICBitmapDitherTypeNone, 
            m_palette.Get(), 
            0.0f, 
            WICBitmapPaletteTypeMedianCut
        );
        if (SUCCEEDED(hr)) {
            source = converter; // Use converted source
        }
    }

    // 7. Set Frame Metadata: Delay
    ComPtr<IWICMetadataQueryWriter> frameMeta;
    if (SUCCEEDED(frameEncode->GetMetadataQueryWriter(&frameMeta))) {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        pv.vt = VT_UI2;
        pv.uiVal = 10; // 10 centiseconds = 100ms = 10 FPS
        frameMeta->SetMetadataByName(L"/grctlext/Delay", &pv);
    }

    WICPixelFormatGUID format = GUID_WICPixelFormat8bppIndexed; 
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
