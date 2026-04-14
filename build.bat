@echo off
echo Building Antigravity Screen Recorder...

cl /EHsc /W3 /O2 /MT /utf-8 ^
    main.cpp CaptureEngine.cpp VideoEncoder.cpp GifEncoder.cpp Recorder.cpp ^
    /Fe:Recorder.exe ^
    /link ^
    d3d11.lib dxgi.lib mfplat.lib mfuuid.lib mfreadwrite.lib ^
    windowscodecs.lib user32.lib gdi32.lib comctl32.lib ^
    ole32.lib shlwapi.lib

if %errorlevel% neq 0 (
    echo.
    echo Build FAILED! Please make sure you are running this in a Visual Studio Developer Command Prompt.
    pause
) else (
    echo.
    echo Build SUCCESSFUL! Created Recorder.exe
)
