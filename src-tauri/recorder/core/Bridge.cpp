#include "Bridge.h"
#include "Recorder.h"
#include <memory>

static std::unique_ptr<Recorder> g_recorder;

HRESULT recorder_start(const wchar_t* filename, int format) {
    if (!g_recorder) {
        g_recorder = std::make_unique<Recorder>();
    }
    
    if (g_recorder->IsRecording()) {
        return S_FALSE;
    }

    Log("Bridge: Starting recorder...");
    return g_recorder->Start(filename, (OutputFormat)format);
}

void recorder_stop() {
    if (g_recorder) {
        Log("Bridge: Stopping recorder...");
        g_recorder->Stop();
    }
}

bool recorder_is_recording() {
    return g_recorder ? g_recorder->IsRecording() : false;
}

int recorder_get_elapsed() {
    return g_recorder ? g_recorder->GetElapsedSeconds() : 0;
}
