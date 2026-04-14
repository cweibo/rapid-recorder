#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <shlwapi.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdio>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using Microsoft::WRL::ComPtr;

// Enhanced Log: ensure real-time visibility in Tauri dev terminal
inline void Log(const std::string& msg) {
    printf("[C++ Core] %s\n", msg.c_str());
    fflush(stdout);
    OutputDebugStringA((msg + "\n").c_str());
}

// Error handling helper
inline bool CheckHR(HRESULT hr, const std::string& msg) {
    if (FAILED(hr)) {
        char buf[256];
        sprintf_s(buf, "Error: %s HRESULT: 0x%08X\n", msg.c_str(), hr);
        Log(buf);
        return false;
    }
    return true;
}

// Resolution alignment
inline void AlignResolution(DXGI_OUTDUPL_DESC& desc, UINT& width, UINT& height) {
    width = desc.ModeDesc.Width & ~15;  // 16-pixel alignment for H.264
    height = desc.ModeDesc.Height & ~15;
}
