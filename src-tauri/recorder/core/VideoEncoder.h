#pragma once
#include "Common.h"

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    HRESULT Initialize(const std::wstring& filename, UINT width, UINT height, ID3D11Device* device);
    bool WriteFrame(const BYTE* pData, UINT stride, LONGLONG timestampTicks);
    void Finalize();

private:
    ComPtr<IMFSinkWriter> m_sinkWriter;
    DWORD m_streamIndex = 0;
    UINT m_width = 0;
    UINT m_height = 0;
    bool m_isInitialized = false;
};
