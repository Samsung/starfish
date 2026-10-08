/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)

#include "platform/multimedia/FFmpegAudioOutput.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <cstring>

#if defined(STARFISH_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#else
#include "platform/multimedia/PulseSimple.h"
#endif

namespace Starfish {

#if defined(STARFISH_WINDOWS)
class WasapiAudioOutput final : public FFmpegAudioOutput {
public:
    WasapiAudioOutput()
        : m_ready(CreateEventW(nullptr, FALSE, FALSE, nullptr))
        , m_interrupt(CreateEventW(nullptr, TRUE, FALSE, nullptr))
    {
    }

    ~WasapiAudioOutput() override
    {
        if (m_client) {
            m_client->Stop();
        }
        if (m_render) {
            m_render->Release();
        }
        if (m_client) {
            m_client->Release();
        }
        if (m_ready) {
            CloseHandle(m_ready);
        }
        if (m_interrupt) {
            CloseHandle(m_interrupt);
        }
        if (m_comInitialized) {
            CoUninitialize();
        }
    }

    bool open(int channels, int sampleRate) override
    {
        HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        m_comInitialized = SUCCEEDED(result);
        if (!m_comInitialized || !m_ready || !m_interrupt) {
            return false;
        }
        IMMDeviceEnumerator* enumerator = nullptr;
        result = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumerator));
        if (FAILED(result)) {
            return false;
        }
        IMMDevice* device = nullptr;
        result =
            enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        enumerator->Release();
        if (FAILED(result)) {
            return false;
        }
        result = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void**>(&m_client));
        device->Release();
        if (FAILED(result)) {
            return false;
        }
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = static_cast<WORD>(channels);
        format.nSamplesPerSec = sampleRate;
        format.wBitsPerSample = 16;
        format.nBlockAlign = channels * sizeof(int16_t);
        format.nAvgBytesPerSec = sampleRate * format.nBlockAlign;
        m_frameBytes = format.nBlockAlign;
        // Shared mode lets the Windows engine adapt interleaved S16 PCM
        // to the endpoint's mix format without an exclusive device lock.
        result =
            m_client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                                     AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                     AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                 1000000, 0, &format, nullptr);
        if (FAILED(result) || FAILED(m_client->SetEventHandle(m_ready)) ||
            FAILED(m_client->GetBufferSize(&m_bufferFrames)) ||
            FAILED(m_client->GetService(__uuidof(IAudioRenderClient),
                                        reinterpret_cast<void**>(&m_render)))) {
            return false;
        }
        return true;
    }

    bool write(const uint8_t* data, size_t bytes) override
    {
        if (bytes % m_frameBytes) {
            return false;
        }
        size_t remaining = bytes / m_frameBytes;
        while (remaining) {
            if (WaitForSingleObject(m_interrupt, 0) == WAIT_OBJECT_0) {
                return false;
            }
            UINT32 padding = 0;
            if (FAILED(m_client->GetCurrentPadding(&padding))) {
                return false;
            }
            UINT32 count = static_cast<UINT32>(std::min(
                remaining, static_cast<size_t>(m_bufferFrames - padding)));
            if (count) {
                BYTE* buffer = nullptr;
                if (FAILED(m_render->GetBuffer(count, &buffer))) {
                    return false;
                }
                std::memcpy(buffer, data, count * m_frameBytes);
                if (FAILED(m_render->ReleaseBuffer(count, 0))) {
                    return false;
                }
                data += count * m_frameBytes;
                remaining -= count;
                if (!m_started) {
                    if (FAILED(m_client->Start())) {
                        return false;
                    }
                    m_started = true;
                }
            }
            if (remaining) {
                HANDLE events[] = { m_interrupt, m_ready };
                if (WaitForMultipleObjects(2, events, FALSE, 2000) !=
                    WAIT_OBJECT_0 + 1) {
                    return false;
                }
            }
        }
        return true;
    }

    void flush() override
    {
        m_client->Stop();
        m_client->Reset();
        m_started = false;
        ResetEvent(m_interrupt);
    }

    void interrupt() override
    {
        SetEvent(m_interrupt);
    }

private:
    IAudioClient* m_client{ nullptr };
    IAudioRenderClient* m_render{ nullptr };
    HANDLE m_ready;
    HANDLE m_interrupt;
    UINT32 m_bufferFrames{ 0 };
    size_t m_frameBytes{ 0 };
    bool m_comInitialized{ false };
    bool m_started{ false };
};
#else
class PulseAudioOutput final : public FFmpegAudioOutput {
public:
    ~PulseAudioOutput() override
    {
        if (m_stream) {
            m_api->pa_simple_free(m_stream);
        }
    }

    bool open(int channels, int sampleRate) override
    {
        void* library = nullptr;
        m_api = loadPulseSimple(library);
        if (!m_api) {
            return false;
        }
        pa_sample_spec spec;
        spec.format = PA_SAMPLE_S16LE;
        spec.rate = sampleRate;
        spec.channels = channels;
        int error = 0;
        m_stream = m_api->pa_simple_new(nullptr, "Starfish", PA_STREAM_PLAYBACK,
                                        nullptr, "media", &spec, nullptr,
                                        nullptr, &error);
        return m_stream != nullptr;
    }

    bool write(const uint8_t* data, size_t bytes) override
    {
        int error = 0;
        return m_api->pa_simple_write(m_stream, data, bytes, &error) >= 0;
    }

    void flush() override
    {
        int error = 0;
        m_api->pa_simple_flush(m_stream, &error);
    }

    void interrupt() override
    {
    }

private:
    PulseSimpleApi* m_api{ nullptr };
    pa_simple* m_stream{ nullptr };
};
#endif

#if defined(STARFISH_MEDIA_PLAYBACK_TEST)
// CI hosts need not expose a physical endpoint. This opt-in sink exercises
// the normal PCM writer queue and timing without retaining the audio stream.
class TestAudioOutput final : public FFmpegAudioOutput {
public:
    bool open(int channels, int sampleRate) override
    {
        m_bytesPerSecond = static_cast<uint64_t>(channels) * sampleRate * 2;
        return m_bytesPerSecond != 0;
    }

    bool write(const uint8_t* data, size_t bytes) override
    {
        const int16_t* samples = reinterpret_cast<const int16_t*>(data);
        size_t count = bytes / sizeof(int16_t);
        double sum = 0;
        int peak = 0;
        for (size_t i = 0; i < count; ++i) {
            int value = samples[i];
            peak = std::max(peak, std::abs(value));
            sum += static_cast<double>(value) * value;
        }
        std::unique_lock<std::mutex> lock(m_mutex);
        auto duration =
            std::chrono::nanoseconds(bytes * 1000000000ULL / m_bytesPerSecond);
        if (m_cv.wait_for(lock, duration, [this]() { return m_interrupted; })) {
            return false;
        }
        std::fprintf(stderr, "FFMPEG_CI_AUDIO samples=%zu rms=%f peak=%d\n",
                     count, count ? std::sqrt(sum / count) / 32768.0 : 0, peak);
        std::fflush(stderr);
        return true;
    }

    void flush() override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_interrupted = false;
    }

    void interrupt() override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_interrupted = true;
        m_cv.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    uint64_t m_bytesPerSecond{ 0 };
    bool m_interrupted{ false };
};

#endif

FFmpegAudioOutput* FFmpegAudioOutput::create()
{
#if defined(STARFISH_MEDIA_PLAYBACK_TEST)
    return new TestAudioOutput();
#elif defined(STARFISH_WINDOWS)
    return new WasapiAudioOutput();
#else
    return new PulseAudioOutput();
#endif
}

} // namespace Starfish

#endif
