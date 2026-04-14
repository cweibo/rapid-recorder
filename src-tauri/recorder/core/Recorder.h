#pragma once
#include "Common.h"
#include "CaptureEngine.h"
#include "VideoEncoder.h"
#include "GifEncoder.h"
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

enum class OutputFormat { MP4, GIF };

struct FrameData {
    std::vector<BYTE> data;
    LONGLONG ts;
};

class Recorder {
public:
    Recorder();
    ~Recorder();

    HRESULT Start(const std::wstring& filename, OutputFormat format);
    void Stop();
    bool IsRecording() const { return m_isRecording; }
    int GetElapsedSeconds() const { return m_elapsedSeconds; }

private:
    void RecordingThread(std::wstring filename, OutputFormat format);
    void EncodingThread(OutputFormat format);

    std::atomic<bool> m_isRecording{false};
    std::atomic<int> m_elapsedSeconds{0};
    
    std::thread m_thread;
    std::thread m_encodingThread;
    
    std::vector<BYTE> m_frameBuffer;
    CaptureEngine m_capture;
    VideoEncoder m_mp4Encoder;
    GifEncoder m_gifEncoder;

    // Async encoding queue
    std::queue<FrameData> m_encodeQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCv;
};
