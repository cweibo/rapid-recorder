#pragma once
#include "Common.h"

class CaptureEngine {
public:
    CaptureEngine();
    ~CaptureEngine();

    bool Initialize();
    void Reset();
    bool AcquireFrame(ComPtr<ID3D11Texture2D>& outTexture);
    void ReleaseFrame();

    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    ID3D11Device* GetDevice() { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() { return m_context.Get(); }

    // Copy current frame to system memory buffer
    bool CopyFrameToBuffer(BYTE* pDst, UINT stride);

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGIOutputDuplication> m_deskDupl;
    ComPtr<ID3D11Texture2D> m_copyTexture; 
    ComPtr<ID3D11Texture2D> m_stagingTexture; 
    
    UINT m_width = 0;
    UINT m_height = 0;
    bool m_frameAcquired = false;
};
