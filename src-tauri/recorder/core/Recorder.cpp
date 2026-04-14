#include "Recorder.h"
#include <ctime>
#include <iomanip>
#include <sstream>

Recorder::Recorder() {}

Recorder::~Recorder() {
    Stop();
}

HRESULT Recorder::Start(const std::wstring& filename, OutputFormat format) {
    if (m_isRecording) return S_FALSE;

    // Ensure COM is initialized for the calling thread (needed for WIC/MF)
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Initialize Media Foundation Platform
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        Log("Recorder: MFStartup failed. HRESULT: " + std::to_string(hr));
        return hr;
    }

    if (!m_capture.Initialize()) {
        Log("Recorder: CaptureEngine failed to initialize.");
        MFShutdown();
        return E_FAIL;
    }

    m_isRecording = true;
    m_elapsedSeconds = 0;

    // Orchestrate encoder initialization
    hr = S_OK;
    if (format == OutputFormat::MP4) {
        hr = m_mp4Encoder.Initialize(filename, m_capture.GetWidth(), m_capture.GetHeight(), m_capture.GetDevice());
    } else {
        hr = m_gifEncoder.Initialize(filename, m_capture.GetWidth(), m_capture.GetHeight());
    }

    if (FAILED(hr)) {
        Log("Recorder: Failed to initialize encoder. HRESULT: " + std::to_string(hr));
        m_isRecording = false;
        m_capture.Reset();
        MFShutdown();
        return hr;
    }
    
    // Clear queue
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while(!m_encodeQueue.empty()) m_encodeQueue.pop();
    }

    m_thread = std::thread(&Recorder::RecordingThread, this, filename, format);
    m_encodingThread = std::thread(&Recorder::EncodingThread, this, format);
    
    return S_OK;
}

void Recorder::Stop() {
    if (!m_isRecording) return;
    
    m_isRecording = false;
    m_queueCv.notify_all();

    if (m_thread.joinable()) m_thread.join();
    if (m_encodingThread.joinable()) m_encodingThread.join();
    
    m_capture.Reset();
    MFShutdown();
    Log("Recorder: Recording stopped and threads joined.");
}

void Recorder::RecordingThread(std::wstring filename, OutputFormat format) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    auto startTime = std::chrono::steady_clock::now();
    auto lastRetryTime = startTime;
    bool hasCapturedAnyFrame = false;
    int retryCount = 0;
    UINT64 frameCount = 0;

    Log("RecordingThread started. Format: " + std::to_string((int)format));

    while (m_isRecording) {
        auto frameStart = std::chrono::steady_clock::now();
        ComPtr<ID3D11Texture2D> tex;
        bool captured = m_capture.AcquireFrame(tex);

        if (captured) {
            UINT width = m_capture.GetWidth();
            UINT height = m_capture.GetHeight();
            UINT stride = width * 4;
            
            if (m_frameBuffer.size() != stride * height) {
                m_frameBuffer.resize(stride * height);
            }

            if (m_capture.CopyFrameToBuffer(m_frameBuffer.data(), stride)) {
                if (!hasCapturedAnyFrame) {
                    hasCapturedAnyFrame = true;
                    startTime = std::chrono::steady_clock::now();
                    Log("Recording logic: First frame captured successfully.");
                }
            }
        } else {
            auto now = std::chrono::steady_clock::now();
            auto timeSinceLastFrame = std::chrono::duration_cast<std::chrono::seconds>(now - (hasCapturedAnyFrame ? startTime : lastRetryTime)).count();
            
            if (!hasCapturedAnyFrame && timeSinceLastFrame >= 2) {
                Log("Recording logic: Timeout. Resetting CaptureEngine...");
                m_capture.Reset();
                lastRetryTime = now;
                retryCount++;
            }
        }

        if (hasCapturedAnyFrame) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
            LONGLONG ts = elapsed.count() * 10000;

            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_encodeQueue.size() < 60) {
                    m_encodeQueue.push({ m_frameBuffer, ts });
                    m_queueCv.notify_one();
                }
            }
            m_elapsedSeconds = (int)elapsed.count() / 1000;
        }

        frameCount++;
        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsedLoop = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (elapsedLoop < std::chrono::milliseconds(33)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(33) - elapsedLoop);
        }
    }
    
    Log("RecordingThread stopped. Total loops: " + std::to_string(frameCount));
    CoUninitialize();
}

void Recorder::EncodingThread(OutputFormat format) {
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    UINT width = m_capture.GetWidth();
    UINT stride = width * 4;
    UINT64 encodedCount = 0;

    Log("EncodingThread consumer loop started.");

    while (m_isRecording || !m_encodeQueue.empty()) {
        FrameData frame;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return !m_encodeQueue.empty();
            });

            if (m_encodeQueue.empty()) {
                if (!m_isRecording) break;
                continue;
            }

            frame = std::move(m_encodeQueue.front());
            m_encodeQueue.pop();
        }

        if (format == OutputFormat::MP4) {
            if (m_mp4Encoder.WriteFrame(frame.data.data(), stride, frame.ts)) {
                encodedCount++;
            }
        } else {
            m_gifEncoder.WriteFrame(frame.data.data(), stride);
            encodedCount++;
        }
    }

    Log("EncodingThread draining complete. Total processed: " + std::to_string(encodedCount));

    if (encodedCount > 0) {
        if (format == OutputFormat::MP4) {
            m_mp4Encoder.Finalize();
        } else {
            m_gifEncoder.Finalize();
        }
        Log("Video finalized successfully.");
    }

    CoUninitialize();
}
