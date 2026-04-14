#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// Export macro (empty for static linking)
#define EXPORT 

// Format mapping
enum BridgeFormat {
    FORMAT_MP4 = 0,
    FORMAT_GIF = 1
};

// Bridge Interfaces
EXPORT HRESULT recorder_start(const wchar_t* filename, int format);
EXPORT void recorder_stop();
EXPORT bool recorder_is_recording();
EXPORT int recorder_get_elapsed();

#ifdef __cplusplus
}
#endif
