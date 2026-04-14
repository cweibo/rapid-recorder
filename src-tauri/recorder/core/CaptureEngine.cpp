#include "CaptureEngine.h"

CaptureEngine::CaptureEngine() {}

CaptureEngine::~CaptureEngine() {
    ReleaseFrame();
}

bool CaptureEngine::Initialize() {
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
        D3D11_CREATE_DEVICE_VIDEO_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, 
        &m_device, &featureLevel, &m_context);
    if (!CheckHR(hr, "Failed to create D3D11 Device")) return false;

    // Multi-threaded protection
    ComPtr<ID3D10Multithread> mt;
    if (SUCCEEDED(m_device.As(&mt))) {
        mt->SetMultithreadProtected(TRUE);
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_device.As(&dxgiDevice);
    
    ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter));
    
    // Adapter Info
    DXGI_ADAPTER_DESC adapterDesc;
    dxgiAdapter->GetDesc(&adapterDesc);
    std::wstring adapterName = adapterDesc.Description;
    std::string name;
    for (wchar_t c : adapterName) if (c > 0 && c < 128) name += (char)c;
    Log("Capture Device: " + name);

    ComPtr<IDXGIOutput> dxgiOutput;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIOutput1> dxgiOutput1;
    hr = dxgiOutput.As(&dxgiOutput1);
    if (FAILED(hr)) return false;

    hr = dxgiOutput1->DuplicateOutput(m_device.Get(), &m_deskDupl);
    if (!CheckHR(hr, "DuplicateOutput Failed")) return false;

    DXGI_OUTDUPL_DESC desc;
    m_deskDupl->GetDesc(&desc);
    m_width = desc.ModeDesc.Width; 
    m_height = desc.ModeDesc.Height; 

    // Create copy texture
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    hr = m_device->CreateTexture2D(&texDesc, nullptr, &m_copyTexture);
    if (!CheckHR(hr, "Failed to create copy texture")) return false;

    // Create staging texture
    D3D11_TEXTURE2D_DESC stagingDesc = texDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.BindFlags = 0;
    stagingDesc.MiscFlags = 0;
    hr = m_device->CreateTexture2D(&stagingDesc, nullptr, &m_stagingTexture);
    if (!CheckHR(hr, "Failed to create staging texture")) return false;

    Log("CaptureEngine Initialized: " + std::to_string(m_width) + "x" + std::to_string(m_height));
    return true;
}

void CaptureEngine::Reset() {
    Log("CaptureEngine resetting Duplication...");
    m_deskDupl.Reset();
    
    ComPtr<IDXGIDevice> dxgiDevice;
    m_device.As(&dxgiDevice);
    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter));
    ComPtr<IDXGIOutput> dxgiOutput;
    dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    ComPtr<IDXGIOutput1> dxgiOutput1;
    dxgiOutput.As(&dxgiOutput1);
    
    HRESULT hr = dxgiOutput1->DuplicateOutput(m_device.Get(), &m_deskDupl);
    if (SUCCEEDED(hr)) {
        Log("CaptureEngine DuplicateOutput reset successful.");
    } else {
        Log("CaptureEngine DuplicateOutput reset failed.");
    }
}

bool CaptureEngine::CopyFrameToBuffer(BYTE* pDst, UINT stride) {
    if (!m_stagingTexture || !m_copyTexture) return false;

    m_context->CopyResource(m_stagingTexture.Get(), m_copyTexture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = m_context->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) return false;

    // CPU Copy
    for (UINT i = 0; i < m_height; ++i) {
        memcpy(pDst + (i * stride), (BYTE*)mapped.pData + (i * mapped.RowPitch), m_width * 4);
    }

    m_context->Unmap(m_stagingTexture.Get(), 0);
    return true;
}

bool CaptureEngine::AcquireFrame(ComPtr<ID3D11Texture2D>& outTexture) {
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    ComPtr<IDXGIResource> desktopResource;
    HRESULT hr = m_deskDupl->AcquireNextFrame(30, &frameInfo, &desktopResource);

    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            outTexture = m_copyTexture;
            return false;
        }
        
        Log("AcquireFrame Error: HRESULT 0x" + std::to_string(hr));

        if (hr == DXGI_ERROR_ACCESS_LOST) {
            Log("Desktop capture access lost.");
        }
        
        outTexture = m_copyTexture;
        return false;
    }

    ComPtr<ID3D11Texture2D> tex;
    hr = desktopResource.As(&tex);
    if (FAILED(hr)) return false;

    // Calibration
    D3D11_TEXTURE2D_DESC actualDesc;
    tex->GetDesc(&actualDesc);
    if (actualDesc.Width != m_width || actualDesc.Height != m_height) {
        m_width = actualDesc.Width;
        m_height = actualDesc.Height;
        Log("Resolution Calibrated: " + std::to_string(m_width) + "x" + std::to_string(m_height));
        
        m_copyTexture.Reset();
        m_stagingTexture.Reset();
        
        D3D11_TEXTURE2D_DESC copyDesc = actualDesc;
        copyDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        m_device->CreateTexture2D(&copyDesc, NULL, &m_copyTexture);
        
        copyDesc.Usage = D3D11_USAGE_STAGING;
        copyDesc.BindFlags = 0;
        copyDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        m_device->CreateTexture2D(&copyDesc, NULL, &m_stagingTexture);
    }

    m_context->CopyResource(m_copyTexture.Get(), tex.Get());
    m_deskDupl->ReleaseFrame();

    m_context->CopyResource(m_stagingTexture.Get(), m_copyTexture.Get());
    return true;
}

void CaptureEngine::ReleaseFrame() {
    // Handled in AcquireFrame
}
