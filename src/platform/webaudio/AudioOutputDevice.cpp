/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "platform/webaudio/AudioOutputDevice.h"

#include "core/modules/webaudio/render/AudioBus.h"
#include "core/modules/profiling/Profiling.h"

#if defined(STARFISH_LINUX) || defined(STARFISH_TIZEN)
#if defined(STARFISH_LINUX)
#include "platform/multimedia/PulseSimple.h"
#else
#include <audio_io.h>
#include <sound_manager.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <new>
#include <system_error>
#include <thread>
#endif

namespace Starfish {

#if defined(STARFISH_LINUX) || defined(STARFISH_TIZEN)
namespace {

    constexpr size_t outputChannels = 2;
    constexpr size_t quantumSamples =
        AudioBus::RenderQuantumFrames * outputChannels;
    constexpr size_t queuedQuanta = 8;
    // Each device is an audio server client plus a writer thread. Writers that
    // are still closing after their owner went away count too, so a stalled
    // server cannot accumulate threads; further devices fall back to the
    // silent wall clock.
    constexpr unsigned maxOutputDevices = 8;
    std::atomic<unsigned> g_outputDevices{ 0 };

    // The platform connection. Only the writer thread uses it, and every
    // call may block on the audio server.
    class OutputSink {
    public:
        // Discards queued output instead of draining it.
        virtual ~OutputSink() = default;
        virtual bool write(const int16_t* pcm, size_t bytes) = 0;
        // Microseconds of audio queued ahead of the speaker.
        virtual bool latency(uint64_t& latencyUs) = 0;
    };

#if defined(STARFISH_LINUX)
    class PulseSink final : public OutputSink {
    public:
        static std::unique_ptr<OutputSink> open(PulseSimpleApi* api,
                                                uint32_t sampleRate)
        {
            pa_sample_spec spec{ PA_SAMPLE_S16LE, sampleRate, 2 };
            // PulseAudio's default prebuffer can hold about two seconds,
            // delaying short media and interactive Web Audio. Request 20 ms,
            // with room for four render quanta at the lowest supported sample
            // rates.
            const uint32_t targetFrames = std::max(
                sampleRate / 50,
                static_cast<uint32_t>(AudioBus::RenderQuantumFrames * 4));
            pa_buffer_attr buffer{ UINT32_MAX, targetFrames * 4, UINT32_MAX,
                                   UINT32_MAX, UINT32_MAX };
            int error = 0;
            pa_simple* sink = api->pa_simple_new(
                nullptr, "Starfish", PA_STREAM_PLAYBACK, nullptr, "Web Audio",
                &spec, nullptr, &buffer, &error);
            if (!sink) {
                return nullptr;
            }
            return std::unique_ptr<OutputSink>(new PulseSink(api, sink));
        }

        ~PulseSink() override
        {
            int error = 0;
            m_api->pa_simple_flush(m_sink, &error);
            m_api->pa_simple_free(m_sink);
        }

        bool write(const int16_t* pcm, size_t bytes) override
        {
            int error = 0;
            return m_api->pa_simple_write(m_sink, pcm, bytes, &error) >= 0;
        }

        bool latency(uint64_t& latencyUs) override
        {
            if (!m_api->pa_simple_get_latency) {
                return false;
            }
            int error = 0;
            latencyUs = m_api->pa_simple_get_latency(m_sink, &error);
            return error == 0 && latencyUs != UINT64_MAX;
        }

    private:
        PulseSink(PulseSimpleApi* api, pa_simple* sink)
            : m_api(api)
            , m_sink(sink)
        {
        }

        PulseSimpleApi* m_api;
        pa_simple* m_sink;
    };
#else
    class AudioOutSink final : public OutputSink {
    public:
        static std::unique_ptr<OutputSink> open(uint32_t sampleRate)
        {
            audio_out_h output = nullptr;
            if (audio_out_create_new(
                    static_cast<int>(sampleRate), AUDIO_CHANNEL_STEREO,
                    AUDIO_SAMPLE_TYPE_S16_LE, &output) != AUDIO_IO_ERROR_NONE) {
                return nullptr;
            }
            // Web Audio is media playback: it follows the media volume and
            // routing policy, like an <audio> element.
            sound_stream_info_h stream = nullptr;
            if (sound_manager_create_stream_information(
                    SOUND_STREAM_TYPE_MEDIA, nullptr, nullptr, &stream) !=
                    SOUND_MANAGER_ERROR_NONE ||
                audio_out_set_sound_stream_info(output, stream) !=
                    AUDIO_IO_ERROR_NONE ||
                audio_out_prepare(output) != AUDIO_IO_ERROR_NONE) {
                audio_out_destroy(output);
                if (stream) {
                    sound_manager_destroy_stream_information(stream);
                }
                return nullptr;
            }
            return std::unique_ptr<OutputSink>(
                new AudioOutSink(output, stream));
        }

        ~AudioOutSink() override
        {
            audio_out_flush(m_output);
            audio_out_unprepare(m_output);
            audio_out_destroy(m_output);
            sound_manager_destroy_stream_information(m_stream);
        }

        bool write(const int16_t* pcm, size_t bytes) override
        {
            auto* data =
                const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pcm));
            while (bytes) {
                int written = audio_out_write(m_output, data,
                                              static_cast<unsigned>(bytes));
                if (written <= 0) {
                    return false;
                }
                data += written;
                bytes -= static_cast<size_t>(written);
            }
            return true;
        }

        // audio_io reports no latency; the context falls back to its wall
        // clock for outputLatency and getOutputTimestamp().
        bool latency(uint64_t&) override
        {
            return false;
        }

    private:
        AudioOutSink(audio_out_h output, sound_stream_info_h stream)
            : m_output(output)
            , m_stream(stream)
        {
        }

        audio_out_h m_output;
        sound_stream_info_h m_stream;
    };
#endif

    // Shared by the owning device and its detached writer thread, so either
    // may go away first.
    struct OutputState {
        explicit OutputState(uint32_t sampleRate)
            : sampleRate(sampleRate)
        {
        }

#if defined(STARFISH_LINUX)
        PulseSimpleApi* api{ nullptr };
#endif
        uint32_t sampleRate;
        // Single-producer/single-consumer ring: submit() fills slots and
        // advances head; the writer drains them and advances tail.
        std::array<std::array<int16_t, quantumSamples>, queuedQuanta> queue;
        std::array<uint64_t, queuedQuanta> firstFrames{};
        std::atomic<size_t> head{ 0 };
        std::atomic<size_t> tail{ 0 };
        std::atomic<bool> stop{ false };
        std::atomic<bool> writerWaiting{ false };
        std::atomic<bool> failed{ false };
        std::atomic<double> outputLatency{ 0 };
        std::mutex wakeMutex;
        std::condition_variable wake;
        mutable std::mutex timestampMutex;
        double timestampContextTime{ 0 };
        uint64_t timestampWallClockUs{ 0 };
    };

    std::unique_ptr<OutputSink> openSink(OutputState& state)
    {
#if defined(STARFISH_LINUX)
        return PulseSink::open(state.api, state.sampleRate);
#else
        return AudioOutSink::open(state.sampleRate);
#endif
    }

    void updateTimestamp(OutputState& state, OutputSink& sink,
                         uint64_t firstFrame)
    {
        uint64_t latencyUs = 0;
        if (!sink.latency(latencyUs)) {
            return;
        }
        // One host query gives both the audible frame and the delay ahead of
        // the block just submitted.
        const double blockSeconds =
            static_cast<double>(AudioBus::RenderQuantumFrames) /
            state.sampleRate;
        state.outputLatency.store(
            std::max(0.0, latencyUs / 1000000.0 - blockSeconds),
            std::memory_order_relaxed);
        const uint64_t endFrame = firstFrame + AudioBus::RenderQuantumFrames;
        const double queuedFrames =
            static_cast<double>(latencyUs) * state.sampleRate / 1000000.0;
        const uint64_t audibleFrame =
            queuedFrames < endFrame
                ? endFrame - static_cast<uint64_t>(queuedFrames)
                : 0;
        const uint64_t nowUs = longTickCount();
        std::lock_guard<std::mutex> lock(state.timestampMutex);
        state.timestampContextTime =
            static_cast<double>(audibleFrame) / state.sampleRate;
        state.timestampWallClockUs = nowUs;
    }

    // Connecting and writing both block on the audio server, so neither
    // runs on the thread that owns the device.
    void writeLoop(std::shared_ptr<OutputState> state)
    {
        std::unique_ptr<OutputSink> sink = openSink(*state);
        if (!sink) {
            state->failed.store(true, std::memory_order_relaxed);
        }
        while (sink && !state->stop.load()) {
            const size_t tail = state->tail.load(std::memory_order_relaxed);
            if (tail == state->head.load()) {
                std::unique_lock<std::mutex> lock(state->wakeMutex);
                state->writerWaiting.store(true);
                if (!state->stop.load() && tail == state->head.load()) {
                    // The timeout only bounds a missed wake-up; submit()
                    // notifies whenever the writer is waiting.
                    state->wake.wait_for(lock, std::chrono::milliseconds(100));
                }
                state->writerWaiting.store(false);
                continue;
            }
            const size_t slot = tail % queuedQuanta;
            if (!sink->write(state->queue[slot].data(),
                             quantumSamples * sizeof(int16_t))) {
                state->failed.store(true, std::memory_order_relaxed);
                break;
            }
            const uint64_t firstFrame = state->firstFrames[slot];
            state->tail.store(tail + 1, std::memory_order_release);
            updateTimestamp(*state, *sink, firstFrame);
        }
        // The owner is gone; the sink discards queued output.
        sink.reset();
        g_outputDevices.fetch_sub(1);
    }

    class QueuedAudioOutputDevice final : public AudioOutputDevice {
    public:
        explicit QueuedAudioOutputDevice(std::shared_ptr<OutputState> state)
            : m_state(std::move(state))
        {
        }

        ~QueuedAudioOutputDevice() override
        {
            // Never join: the writer may be blocked in the server. It exits
            // after its current call and owns the connection until then.
            m_state->stop.store(true);
            std::lock_guard<std::mutex> lock(m_state->wakeMutex);
            m_state->wake.notify_one();
        }

        // Called by a single producer thread.
        void submit(const AudioBus& bus, uint64_t firstFrame) override
        {
            OutputState& state = *m_state;
            if (state.failed.load(std::memory_order_relaxed)) {
                return;
            }
            const size_t head = state.head.load(std::memory_order_relaxed);
            if (head - state.tail.load(std::memory_order_acquire) ==
                queuedQuanta) {
                // The writer is a full queue behind the server; blocking here
                // would stall rendering, so the newest quantum is dropped.
                return;
            }
            const size_t slot = head % queuedQuanta;
            std::array<int16_t, quantumSamples>& pcm = state.queue[slot];
            for (size_t frame = 0; frame < AudioBus::RenderQuantumFrames;
                 frame++) {
                for (size_t channel = 0; channel < outputChannels; channel++) {
                    const size_t sourceChannel =
                        std::min(channel, bus.channels() - 1);
                    const float sample = bus.channel(sourceChannel)[frame];
                    const float clamped =
                        std::isfinite(sample)
                            ? std::max(-1.0f, std::min(1.0f, sample))
                            : 0.0f;
                    pcm[frame * outputChannels + channel] =
                        static_cast<int16_t>(std::lrint(clamped * 32767.0f));
                }
            }
            state.firstFrames[slot] = firstFrame;
            state.head.store(head + 1);
            // Only take the lock when the writer sleeps on an empty queue.
            if (state.writerWaiting.load()) {
                std::lock_guard<std::mutex> lock(state.wakeMutex);
                state.wake.notify_one();
            }
        }

        bool failed() const override
        {
            return m_state->failed.load(std::memory_order_relaxed);
        }

        double outputLatency() const override
        {
            return m_state->outputLatency.load(std::memory_order_relaxed);
        }

        bool outputTimestamp(double& contextTime,
                             uint64_t& wallClockUs) const override
        {
            std::lock_guard<std::mutex> lock(m_state->timestampMutex);
            if (!m_state->timestampWallClockUs) {
                return false;
            }
            contextTime = m_state->timestampContextTime;
            wallClockUs = m_state->timestampWallClockUs;
            return true;
        }

    private:
        std::shared_ptr<OutputState> m_state;
    };

} // namespace
#endif

std::unique_ptr<AudioOutputDevice> AudioOutputDevice::create(
    uint32_t sampleRate)
{
#if defined(STARFISH_LINUX) || defined(STARFISH_TIZEN)
#if defined(STARFISH_LINUX)
    void* handle = nullptr;
    PulseSimpleApi* api = loadPulseSimple(handle);
    if (!api) {
        return nullptr;
    }
#endif
    if (g_outputDevices.fetch_add(1) >= maxOutputDevices) {
        g_outputDevices.fetch_sub(1);
        return nullptr;
    }
    try {
        auto state = std::make_shared<OutputState>(sampleRate);
#if defined(STARFISH_LINUX)
        state->api = api;
#endif
        std::unique_ptr<AudioOutputDevice> device(
            new QueuedAudioOutputDevice(state));
        std::thread(writeLoop, std::move(state)).detach();
        return device;
    } catch (const std::bad_alloc&) {
    } catch (const std::system_error&) {
    }
    g_outputDevices.fetch_sub(1);
    return nullptr;
#else
    return nullptr;
#endif
}

} // namespace Starfish

#endif
