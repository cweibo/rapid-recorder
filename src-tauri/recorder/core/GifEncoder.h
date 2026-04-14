#pragma once
#include "Common.h"

class GifEncoder {
public:
    GifEncoder();
    ~GifEncoder();

    HRESULT Initialize(const std::wstring& filename, UINT width, UINT height);
    bool WriteFrame(const BYTE* pData, UINT stride);
    void Finalize();

private:
    ComPtr<IWICImagingFactory> m_factory;
    ComPtr<IWICBitmapEncoder> m_encoder;
    ComPtr<IWICStream> m_stream; // Prevent stream from being closed during writing
    UINT m_width = 0;
    UINT m_height = 0;
    UINT m_origWidth = 0;
    UINT m_origHeight = 0;
    bool m_isInitialized = false;

    std::vector<BYTE> m_flippedBuffer;
};
