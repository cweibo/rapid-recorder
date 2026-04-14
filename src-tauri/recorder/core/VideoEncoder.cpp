#include "VideoEncoder.h"
#include <codecapi.h>

VideoEncoder::VideoEncoder() {}

VideoEncoder::~VideoEncoder() {
    Finalize();
}

HRESULT VideoEncoder::Initialize(const std::wstring& filename, UINT width, UINT height, ID3D11Device* device) {
    // MediaFoundation H.264 requires 16-pixel alignment
    m_width = width & ~15;
    m_height = height & ~15;

    Log("VideoEncoder Initializing: " + std::to_string(m_width) + "x" + std::to_string(m_height));

    ComPtr<IMFAttributes> attributes;
    MFCreateAttributes(&attributes, 2);
    attributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, 1);
    attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, 1);

    HRESULT hr = MFCreateSinkWriterFromURL(filename.c_str(), nullptr, attributes.Get(), &m_sinkWriter);
    if (FAILED(hr)) return hr;

    // Out Media Type (H.264)
    ComPtr<IMFMediaType> outType;
    MFCreateMediaType(&outType);
    outType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outType->SetUINT32(MF_MT_AVG_BITRATE, 8000000); // 8 Mbps
    outType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    outType->SetUINT32(MF_MT_VIDEO_PROFILE, eAVEncH264VProfile_High);
    outType->SetUINT32(MF_MT_VIDEO_LEVEL, eAVEncH264VLevel5_1);

    MFSetAttributeSize(outType.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
    MFSetAttributeRatio(outType.Get(), MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(outType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = m_sinkWriter->AddStream(outType.Get(), &m_streamIndex);
    if (FAILED(hr)) return hr;

    // In Media Type (RGB32)
    ComPtr<IMFMediaType> inType;
    MFCreateMediaType(&inType);
    inType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    inType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    MFSetAttributeSize(inType.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
    MFSetAttributeRatio(inType.Get(), MF_MT_FRAME_RATE, 30, 1);
    MFSetAttributeRatio(inType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    hr = m_sinkWriter->SetInputMediaType(m_streamIndex, inType.Get(), nullptr);
    if (FAILED(hr)) return hr;

    hr = m_sinkWriter->BeginWriting();
    if (FAILED(hr)) return hr;

    m_isInitialized = true;
    Log("VideoEncoder Initialized (H.264 High Profile 5.1)");
    return S_OK;
}

bool VideoEncoder::WriteFrame(const BYTE* pData, UINT stride, LONGLONG timestampTicks) {
    if (!m_isInitialized) return false;

    DWORD bufferSize = m_width * m_height * 4;
    ComPtr<IMFMediaBuffer> buffer;
    HRESULT hr = MFCreateMemoryBuffer(bufferSize, &buffer);
    
    BYTE* pDstData = nullptr;
    hr = buffer->Lock(&pDstData, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
        // CPU Copy with Flip
        for (UINT i = 0; i < m_height; ++i) {
            memcpy(pDstData + ((m_height - 1 - i) * m_width * 4), pData + (i * stride), m_width * 4);
        }
        buffer->Unlock();
        buffer->SetCurrentLength(bufferSize);
    }

    ComPtr<IMFSample> sample;
    hr = MFCreateSample(&sample);
    hr = sample->AddBuffer(buffer.Get());
    hr = sample->SetSampleTime(timestampTicks);
    hr = sample->SetSampleDuration(333333);

    hr = m_sinkWriter->WriteSample(m_streamIndex, sample.Get());
    if (FAILED(hr)) return false;

    return true;
}

void VideoEncoder::Finalize() {
    if (m_isInitialized && m_sinkWriter) {
        m_sinkWriter->Finalize();
        m_sinkWriter.Reset();
        m_isInitialized = false;
        Log("VideoEncoder Finalized");
    }
}
