/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioGraph__
#define __StarfishAudioGraph__

#include "core/modules/webaudio/render/AudioBus.h"
#include "core/modules/webaudio/render/AudioHandler.h"
#include "core/modules/webaudio/render/AudioNodeInput.h"
#include "core/modules/webaudio/render/AudioNodeOutput.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <atomic>
#include <mutex>

namespace Starfish {

class AudioHandler;
class AudioGraph;
class AudioNodeOutput;
class AudioParamTimeline;
class AudioRenderThread;

// Hand-off from GC finalizers of AudioNode/AudioParam wrappers to the graph.
// Finalizers run in no particular order, so the graph may already be gone
// when a wrapper is collected; this refcounted native object outlives both
// and drops posts once the graph has detached.
class AudioGraphReleaseQueue {
public:
    void retain()
    {
        m_references.fetch_add(1, std::memory_order_relaxed);
    }
    void release();
    // The wrapper owning |handler| was collected.
    void postHandler(AudioHandler* handler);
    // An AudioParam wrapping |timeline| was collected.
    void postTimeline(AudioParamTimeline* timeline);

private:
    friend class AudioGraph;
    AudioGraphReleaseQueue() = default;
    ~AudioGraphReleaseQueue() = default;

    std::atomic<unsigned> m_references{ 1 };
    std::mutex m_mutex;
    std::vector<AudioHandler*> m_handlers;
    std::vector<AudioParamTimeline*> m_timelines;
    bool m_attached{ true };
};

class AudioGraph {
public:
    explicit AudioGraph(double sampleRate);
    ~AudioGraph();

    // Control-side native accesses must hold this lock. It is acquired once
    // per render quantum, never across JS tasks, event dispatch or join().
    // Recursive locking allows a node constructor to use its public setters.
    std::recursive_mutex& mutex()
    {
        return m_mutex;
    }
    // Creates the render thread and output device. The thread exists only
    // between startRealtime() and stopRealtime(), so a suspended or closed
    // context has no thread to wake.
    bool startRealtime(size_t channels);
    void stopRealtime();
    double outputLatency() const;
    bool outputTimestamp(double& contextTime, uint64_t& wallClockUs) const;
    void registerScheduledHandler(AudioHandler* handler);
    void queueConnection(AudioNodeOutput* source, AudioNodeInput* destination,
                         bool connect);
    void queueConnection(AudioNodeOutput* source,
                         AudioParamTimeline* destination, bool connect);
    // Caller holds m_mutex, or is the offline renderer on the main thread.
    void processPendingConnections();

    AudioHandler* addHandler(std::unique_ptr<AudioHandler> handler);
    void markTopologyDirty()
    {
        m_topologyDirty = true;
    }
    void registerAnalysisHandler(AudioHandler* handler);
    void registerTimeline(AudioParamTimeline* timeline);
    AudioParamTimeline* createTimeline(float initialValue);
    void setDestination(AudioNodeOutput* destination);
    void renderQuantum(AudioBus& destination, size_t frames);
    void prepareOutput(size_t channels)
    {
        m_renderBus.resize(channels);
    }
    const AudioBus& renderQuantum(size_t frames = AudioBus::RenderQuantumFrames)
    {
        renderQuantum(m_renderBus, frames);
        return m_renderBus;
    }
    double currentTime() const
    {
        return static_cast<double>(renderedFrames()) / m_sampleRate;
    }
    uint64_t renderedFrames() const
    {
        return m_renderedFrames.load(std::memory_order_acquire);
    }
    // Wrappers retain this to report their collection; see the class above.
    AudioGraphReleaseQueue* releaseQueue() const
    {
        return m_releaseQueue;
    }

private:
    struct ConnectionCommand {
        AudioNodeOutput* source;
        union {
            AudioNodeInput* input;
            AudioParamTimeline* param;
        } destination;
        bool isParam;
        bool connect;
    };
    enum class StepKind : uint8_t { Process, Muted, DelayRead, DelayWrite };
    struct RenderStep {
        AudioHandler* handler;
        StepKind kind;
    };
    struct Scratch;

    void queueConnection(const ConnectionCommand& command);
    void refreshTopology();
    void releaseHandlers();
    bool isReleasable(AudioHandler* handler) const;
    void detachHandler(AudioHandler* handler);
    void sweepTimelines();

    std::recursive_mutex m_mutex;
    std::mutex m_connectionMutex;
    std::vector<ConnectionCommand> m_pendingConnections;
    std::vector<ConnectionCommand> m_processingConnections;
    std::unique_ptr<AudioRenderThread> m_renderThread;
    std::vector<std::unique_ptr<AudioHandler>> m_handlers;
    std::vector<AudioHandler*> m_scheduledHandlers;
    std::vector<AudioHandler*> m_analysisHandlers;
    std::vector<AudioParamTimeline*> m_timelines;
    std::vector<std::unique_ptr<AudioParamTimeline>> m_ownedTimelines;
    // Rebuilt only when the topology changes; the render loop walks it
    // without recursion, so graph depth never consumes thread stack.
    std::vector<RenderStep> m_renderSteps;
    std::unique_ptr<Scratch> m_scratch;
    AudioGraphReleaseQueue* m_releaseQueue;
    std::vector<AudioHandler*> m_releaseCandidates;
    std::vector<AudioHandler*> m_takenHandlers;
    std::vector<AudioParamTimeline*> m_takenTimelines;
    AudioNodeOutput* m_destination{ nullptr };
    AudioBus m_renderBus;
    double m_sampleRate;
    uint64_t m_quantum{ 0 };
    std::atomic<uint64_t> m_renderedFrames{ 0 };
    bool m_topologyDirty{ true };
    bool m_releaseDirty{ false };
    bool m_timelineSweepNeeded{ false };
};

class AudioGraphLock {
public:
    explicit AudioGraphLock(AudioGraph* graph)
        : m_lock(graph->mutex())
    {
        // Keep queued topology changes ordered before synchronous controls.
        graph->processPendingConnections();
    }

private:
    std::lock_guard<std::recursive_mutex> m_lock;
};

} // namespace Starfish

#endif
#endif
