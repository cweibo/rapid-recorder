#include "Common.h"
#include "Recorder.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

Recorder g_recorder;
HWND hStatus, hStartBtn, hFormatMp4, hFormatGif;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"输出格式:", WS_VISIBLE | WS_CHILD, 20, 20, 100, 20, hwnd, NULL, NULL, NULL);
        hFormatMp4 = CreateWindowW(L"BUTTON", L"MP4 (H.264)", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 120, 20, 100, 20, hwnd, NULL, NULL, NULL);
        hFormatGif = CreateWindowW(L"BUTTON", L"GIF 动画", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 230, 20, 100, 20, hwnd, NULL, NULL, NULL);
        SendMessage(hFormatMp4, BM_SETCHECK, BST_CHECKED, 0);

        hStartBtn = CreateWindowW(L"BUTTON", L"开始录制", WS_VISIBLE | WS_CHILD, 20, 60, 310, 40, hwnd, (HMENU)1, NULL, NULL);
        hStatus = CreateWindowW(L"STATIC", L"等候中...", WS_VISIBLE | WS_CHILD | SS_CENTER, 20, 110, 310, 20, hwnd, NULL, NULL, NULL);
        
        SetTimer(hwnd, 1, 1000, NULL);
        return 0;
    }
    case WM_TIMER: {
        if (g_recorder.IsRecording()) {
            wchar_t buf[64];
            swprintf_s(buf, L"正在录制: %d 秒", g_recorder.GetElapsedSeconds());
            SetWindowTextW(hStatus, buf);
        }
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1) { // 按钮点击
            if (!g_recorder.IsRecording()) {
                OutputFormat fmt = (SendMessage(hFormatMp4, BM_GETCHECK, 0, 0) == BST_CHECKED) ? OutputFormat::MP4 : OutputFormat::GIF;
                std::wstring ext = (fmt == OutputFormat::MP4) ? L".mp4" : L".gif";
                std::wstring filename = L"Recording_" + std::to_wstring(time(NULL)) + ext;
                
                g_recorder.Start(filename, fmt);
                if (g_recorder.IsRecording()) {
                    SetWindowTextW(hStartBtn, L"停止录制 (正在录制...)");
                }
            } else {
                g_recorder.Stop();
                SetWindowTextW(hStartBtn, L"开始录制");
                SetWindowTextW(hStatus, L"录制已完成");
            }
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    // 启用初始化
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    // 启用 DPI 感知，确保全屏录制分辨率正确
    SetProcessDPIAware();

    const wchar_t CLASS_NAME[] = L"ScreenRecorderWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Antigravity 极速录屏", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 365, 180, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
