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

#if defined(STARFISH_LINUX)
#include "platform/multimedia/PulseSimple.h"

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

#if defined(STARFISH_LINUX)
namespace {

    constexpr size_t outputChannels = 2;
    constexpr size_t quantumSamples =
        AudioBus::RenderQuantumFrames * outputChannels;
    constexpr size_t queuedQuanta = 8;
    // Each device is a PulseAudio client plus a writer thread. Writers that
    // are still closing after their owner went away count too, so a stalled
    // server cannot accumulate threads; further devices fall back to the
    // silent wall clock.
    constexpr unsigned maxOutputDevices = 8;
    std::atomic<unsigned> g_outputDevices{ 0 };

    // Shared by the owning device and its detached writer thread, so either
    // may go away first.
    struct PulseOutputState {
        PulseOutputState(PulseSimpleApi* api, uint32_t sampleRate)
            : api(api)
            , sampleRate(sampleRate)
        {
        }

        PulseSimpleApi* api;
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

    pa_simple* openSink(PulseOutputState& state)
    {
        pa_sample_spec spec{ PA_SAMPLE_S16LE, state.sampleRate, 2 };
        // PulseAudio's default prebuffer can hold about two seconds, delaying
        // short media and interactive Web Audio. Request 20 ms, with room for
        // four render quanta at the lowest supported sample rates.
        const uint32_t targetFrames =
            std::max(state.sampleRate / 50,
                     static_cast<uint32_t>(AudioBus::RenderQuantumFrames * 4));
        pa_buffer_attr buffer{ UINT32_MAX, targetFrames * 4, UINT32_MAX,
                               UINT32_MAX, UINT32_MAX };
        int error = 0;
        return state.api->pa_simple_new(nullptr, "Starfish", PA_STREAM_PLAYBACK,
                                        nullptr, "Web Audio", &spec, nullptr,
                                        &buffer, &error);
    }

    void updateTimestamp(PulseOutputState& state, pa_simple* sink,
                         uint64_t firstFrame)
    {
        if (!state.api->pa_simple_get_latency) {
            return;
        }
        int error = 0;
        const uint64_t latencyUs =
            state.api->pa_simple_get_latency(sink, &error);
        if (error != 0 || latencyUs == UINT64_MAX) {
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

    // Connecting and writing both block on the PulseAudio server, so neither
    // runs on the thread that owns the device.
    void writeLoop(std::shared_ptr<PulseOutputState> state)
    {
        pa_simple* sink = openSink(*state);
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
            int error = 0;
            if (state->api->pa_simple_write(sink, state->queue[slot].data(),
                                            quantumSamples * sizeof(int16_t),
                                            &error) < 0) {
                state->failed.store(true, std::memory_order_relaxed);
                break;
            }
            const uint64_t firstFrame = state->firstFrames[slot];
            state->tail.store(tail + 1, std::memory_order_release);
            updateTimestamp(*state, sink, firstFrame);
        }
        if (sink) {
            int error = 0;
            // The owner is gone; discard queued output instead of draining.
            state->api->pa_simple_flush(sink, &error);
            state->api->pa_simple_free(sink);
        }
        g_outputDevices.fetch_sub(1);
    }

    class PulseAudioOutputDevice final : public AudioOutputDevice {
    public:
        explicit PulseAudioOutputDevice(std::shared_ptr<PulseOutputState> state)
            : m_state(std::move(state))
        {
        }

        ~PulseAudioOutputDevice() override
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
            PulseOutputState& state = *m_state;
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
        std::shared_ptr<PulseOutputState> m_state;
    };

} // namespace
#endif

std::unique_ptr<AudioOutputDevice> AudioOutputDevice::create(
    uint32_t sampleRate)
{
#if defined(STARFISH_LINUX)
    void* handle = nullptr;
    PulseSimpleApi* api = loadPulseSimple(handle);
    if (!api) {
        return nullptr;
    }
    if (g_outputDevices.fetch_add(1) >= maxOutputDevices) {
        g_outputDevices.fetch_sub(1);
        return nullptr;
    }
    try {
        auto state = std::make_shared<PulseOutputState>(api, sampleRate);
        std::unique_ptr<AudioOutputDevice> device(
            new PulseAudioOutputDevice(state));
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
