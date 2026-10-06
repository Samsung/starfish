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
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"
#include "platform/webaudio/AudioOutputDevice.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <system_error>
#include <thread>

namespace Starfish {

// Native ownership only: neither the worker nor its stop/join path can touch
// a DOM wrapper, a JS callback or the GC heap.
class AudioRenderThread {
public:
    AudioRenderThread(AudioGraph* graph, double sampleRate)
        : m_graph(graph)
        , m_sampleRate(sampleRate)
        , m_device(AudioOutputDevice::create(
              static_cast<uint32_t>(std::round(sampleRate))))
    {
        m_thread = std::thread([this]() { run(); });
    }

    ~AudioRenderThread()
    {
        {
            std::lock_guard<std::mutex> lock(m_waitMutex);
            m_stopping.store(true, std::memory_order_release);
        }
        m_wake.notify_one();
        m_thread.join();
    }

    double outputLatency() const
    {
        return m_device ? m_device->outputLatency() : 0;
    }

    bool outputTimestamp(double& contextTime, uint64_t& wallClockUs) const
    {
        return m_device && m_device->outputTimestamp(contextTime, wallClockUs);
    }

private:
    void run()
    {
        using Clock = std::chrono::steady_clock;
        const auto quantum = std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<double>(AudioBus::RenderQuantumFrames /
                                          m_sampleRate));
        auto deadline = Clock::now() + quantum;
        while (true) {
            {
                std::unique_lock<std::mutex> lock(m_waitMutex);
                if (m_wake.wait_until(lock, deadline, [this]() {
                        return m_stopping.load(std::memory_order_acquire);
                    })) {
                    return;
                }
            }
            {
                std::lock_guard<std::recursive_mutex> lock(m_graph->mutex());
                if (m_stopping.load(std::memory_order_acquire)) {
                    return;
                }
                const uint64_t firstFrame = m_graph->renderedFrames();
                const AudioBus& bus = m_graph->renderQuantum();
                if (m_device) {
                    m_device->submit(bus, firstFrame);
                }
            }
            deadline += quantum;
            // Bound catch-up work after an overloaded DSP graph or a long
            // control operation; do not accumulate an unbounded output queue.
            if (Clock::now() > deadline + quantum * 16) {
                deadline = Clock::now() + quantum;
            }
        }
    }

    AudioGraph* m_graph;
    double m_sampleRate;
    std::unique_ptr<AudioOutputDevice> m_device;
    std::mutex m_waitMutex;
    std::condition_variable m_wake;
    std::atomic<bool> m_stopping{ false };
    std::thread m_thread;
};

void AudioGraphReleaseQueue::release()
{
    if (m_references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

void AudioGraphReleaseQueue::postHandler(AudioHandler* handler)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_attached) {
        m_handlers.push_back(handler);
    }
}

void AudioGraphReleaseQueue::postTimeline(AudioParamTimeline* timeline)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_attached) {
        m_timelines.push_back(timeline);
    }
}

// Reused between topology refreshes so that steady-state rendering does not
// allocate; capacities only grow with the graph.
struct AudioGraph::Scratch {
    struct Edge {
        uint32_t from;
        uint32_t to;
        bool param;
    };
    struct Frame {
        uint32_t node;
        uint32_t next;
    };
    std::vector<Edge> edges;
    std::vector<uint32_t> forwardStart;
    std::vector<uint32_t> forward;
    std::vector<uint32_t> reverseStart;
    std::vector<uint32_t> reverse;
    std::vector<uint32_t> finishOrder;
    std::vector<uint32_t> component;
    std::vector<uint32_t> componentSize;
    std::vector<uint32_t> writerOf;
    std::vector<uint32_t> pending;
    std::vector<uint8_t> flags;
    std::vector<uint8_t> selfLoop;
    std::vector<Frame> walk;

    void buildAdjacency(size_t nodes)
    {
        forwardStart.assign(nodes + 1, 0);
        reverseStart.assign(nodes + 1, 0);
        for (const Edge& edge : edges) {
            forwardStart[edge.from + 1]++;
            reverseStart[edge.to + 1]++;
        }
        for (size_t i = 0; i < nodes; i++) {
            forwardStart[i + 1] += forwardStart[i];
            reverseStart[i + 1] += reverseStart[i];
        }
        forward.resize(edges.size());
        reverse.resize(edges.size());
        pending.assign(forwardStart.begin(), forwardStart.end() - 1);
        for (const Edge& edge : edges) {
            forward[pending[edge.from]++] = edge.to;
        }
        pending.assign(reverseStart.begin(), reverseStart.end() - 1);
        for (const Edge& edge : edges) {
            reverse[pending[edge.to]++] = edge.from;
        }
        pending.clear();
    }

    // Kosaraju: |component| numbers strongly connected components in a
    // topological order of the condensed graph (sources first).
    void computeComponents(size_t nodes)
    {
        flags.assign(nodes, 0);
        finishOrder.clear();
        for (uint32_t start = 0; start < nodes; start++) {
            if (flags[start]) {
                continue;
            }
            flags[start] = 1;
            walk.push_back({ start, forwardStart[start] });
            while (!walk.empty()) {
                Frame& frame = walk.back();
                if (frame.next < forwardStart[frame.node + 1]) {
                    const uint32_t next = forward[frame.next++];
                    if (!flags[next]) {
                        flags[next] = 1;
                        walk.push_back({ next, forwardStart[next] });
                    }
                } else {
                    finishOrder.push_back(frame.node);
                    walk.pop_back();
                }
            }
        }
        const uint32_t unassigned = static_cast<uint32_t>(-1);
        component.assign(nodes, unassigned);
        componentSize.clear();
        for (auto it = finishOrder.rbegin(); it != finishOrder.rend(); ++it) {
            if (component[*it] != unassigned) {
                continue;
            }
            const uint32_t id = static_cast<uint32_t>(componentSize.size());
            componentSize.push_back(0);
            component[*it] = id;
            pending.push_back(*it);
            while (!pending.empty()) {
                const uint32_t node = pending.back();
                pending.pop_back();
                componentSize[id]++;
                for (uint32_t i = reverseStart[node];
                     i < reverseStart[node + 1]; i++) {
                    const uint32_t previous = reverse[i];
                    if (component[previous] == unassigned) {
                        component[previous] = id;
                        pending.push_back(previous);
                    }
                }
            }
        }
        selfLoop.assign(nodes, 0);
        for (const Edge& edge : edges) {
            if (edge.from == edge.to) {
                selfLoop[edge.from] = 1;
            }
        }
    }

    bool inCycle(uint32_t node) const
    {
        return componentSize[component[node]] > 1 || selfLoop[node];
    }
};

AudioGraph::AudioGraph(double sampleRate)
    : m_scratch(new Scratch)
    , m_releaseQueue(new AudioGraphReleaseQueue)
    , m_sampleRate(sampleRate)
{
    STARFISH_ASSERT(sampleRate >= 3000 && sampleRate <= 768000);
}

AudioGraph::~AudioGraph()
{
    stopRealtime();
    {
        std::lock_guard<std::mutex> lock(m_releaseQueue->m_mutex);
        m_releaseQueue->m_attached = false;
        m_releaseQueue->m_handlers.clear();
        m_releaseQueue->m_timelines.clear();
    }
    m_releaseQueue->release();
    // Every handler and owned timeline dies here, so drop the connection
    // lists first: a destructor must never reach a peer that was already
    // destroyed earlier in this loop.
    for (const auto& handler : m_handlers) {
        handler->detachForTeardown();
    }
    for (const auto& timeline : m_ownedTimelines) {
        timeline->clearSourcesForTeardown();
    }
    m_handlers.clear();
}

bool AudioGraph::startRealtime(size_t channels)
{
    STARFISH_ASSERT(!m_renderThread);
    prepareOutput(channels);
    try {
        m_renderThread.reset(new AudioRenderThread(this, m_sampleRate));
    } catch (const std::system_error& error) {
        // Surface resource exhaustion instead of leaving a running context
        // whose rendering thread was never created.
        STARFISH_LOG_ERROR("Cannot start audio render thread: %s",
                           error.what());
        return false;
    }
    return true;
}

void AudioGraph::stopRealtime()
{
    // Never hold m_mutex while joining: the worker may be waiting for it.
    m_renderThread.reset();
}

double AudioGraph::outputLatency() const
{
    return m_renderThread ? m_renderThread->outputLatency() : 0;
}

bool AudioGraph::outputTimestamp(double& contextTime,
                                 uint64_t& wallClockUs) const
{
    return m_renderThread &&
           m_renderThread->outputTimestamp(contextTime, wallClockUs);
}

void AudioGraph::registerScheduledHandler(AudioHandler* handler)
{
    m_scheduledHandlers.push_back(handler);
    m_topologyDirty = true;
}

void AudioGraph::queueConnection(AudioNodeOutput* source,
                                 AudioNodeInput* destination, bool connect)
{
    ConnectionCommand command;
    command.source = source;
    command.destination.input = destination;
    command.isParam = false;
    command.connect = connect;
    queueConnection(command);
}

void AudioGraph::queueConnection(AudioNodeOutput* source,
                                 AudioParamTimeline* destination, bool connect)
{
    ConnectionCommand command;
    command.source = source;
    command.destination.param = destination;
    command.isParam = true;
    command.connect = connect;
    queueConnection(command);
}

void AudioGraph::queueConnection(const ConnectionCommand& command)
{
    // https://webaudio.github.io/web-audio-api/#control-thread-and-rendering-thread
    // Commands hold native pointers that stay valid until the command runs:
    // a handler or timeline is only removed after its wrapper was collected,
    // and processPendingConnections() collects those reports before it
    // drains the commands queued ahead of them.
    // Bound backlog even for suspended/offline contexts. On saturation, apply
    // the backlog between quanta instead of dropping commands or growing it.
    constexpr size_t MaxPendingConnections = 256;
    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_connectionMutex);
            if (m_pendingConnections.size() < MaxPendingConnections) {
                m_pendingConnections.push_back(command);
                return;
            }
        }
        // Lock ordering is graph -> queue, never queue -> graph.
        AudioGraphLock lock(this);
    }
}

void AudioGraph::processPendingConnections()
{
    // Take collection reports before the commands: every command that names
    // a collected wrapper's handler was queued before that report.
    {
        std::lock_guard<std::mutex> lock(m_releaseQueue->m_mutex);
        m_releaseQueue->m_handlers.swap(m_takenHandlers);
        m_releaseQueue->m_timelines.swap(m_takenTimelines);
    }
    {
        std::lock_guard<std::mutex> lock(m_connectionMutex);
        m_pendingConnections.swap(m_processingConnections);
    }
    for (const auto& command : m_processingConnections) {
        if (command.isParam) {
            if (command.connect) {
                command.destination.param->connect(command.source);
            } else {
                command.destination.param->disconnect(command.source);
            }
        } else if (command.connect) {
            command.destination.input->connect(command.source);
        } else {
            command.destination.input->disconnect(command.source);
        }
    }
    // Retain the two bounded buffers to avoid allocations on every quantum.
    m_processingConnections.clear();
    if (!m_takenTimelines.empty()) {
        for (AudioParamTimeline* timeline : m_takenTimelines) {
            timeline->releaseWrapper();
        }
        m_takenTimelines.clear();
        m_releaseDirty = true;
        m_timelineSweepNeeded = true;
    }
    if (!m_takenHandlers.empty()) {
        for (AudioHandler* handler : m_takenHandlers) {
            if (!handler->releaseRequested() &&
                (!m_destination || handler != m_destination->owner())) {
                handler->setReleaseRequested();
                m_releaseCandidates.push_back(handler);
            }
        }
        m_takenHandlers.clear();
        m_releaseDirty = true;
    }
}

AudioHandler* AudioGraph::addHandler(std::unique_ptr<AudioHandler> handler)
{
    AudioHandler* raw = handler.get();
    raw->setGraph(this);
    m_handlers.push_back(std::move(handler));
    m_topologyDirty = true;
    return raw;
}

// https://webaudio.github.io/web-audio-api/#rendering-an-audio-graph
// Orders the nodes once per topology change: DelayNodes in cycles are split
// into a reader (a source) and a writer, the remaining cycles are muted, and
// everything else is rendered in topological order. Connections into an
// AudioParam are edges into the node owning that param.
void AudioGraph::refreshTopology()
{
    m_topologyDirty = false;
    Scratch& scratch = *m_scratch;
    const uint32_t count = static_cast<uint32_t>(m_handlers.size());
    for (uint32_t i = 0; i < count; i++) {
        m_handlers[i]->setGraphIndex(i);
        m_handlers[i]->setCycleMuted(false);
    }

    scratch.edges.clear();
    for (uint32_t i = 0; i < count; i++) {
        AudioHandler* handler = m_handlers[i].get();
        for (size_t output = 0; output < handler->outputCount(); output++) {
            for (AudioNodeInput* destination :
                 handler->output(output).destinations()) {
                scratch.edges.push_back(
                    { i, destination->owner()->graphIndex(), false });
            }
        }
        for (size_t index = 0; index < handler->timelineCount(); index++) {
            for (AudioNodeOutput* source :
                 handler->timeline(index)->sources()) {
                scratch.edges.push_back(
                    { source->owner()->graphIndex(), i, true });
            }
        }
    }

    // Find the DelayNodes that are part of a cycle.
    scratch.buildAdjacency(count);
    scratch.computeComponents(count);
    const uint32_t none = static_cast<uint32_t>(-1);
    scratch.writerOf.assign(count, none);
    uint32_t nodes = count;
    for (uint32_t i = 0; i < count; i++) {
        AudioHandler* handler = m_handlers[i].get();
        if (!handler->isDelayNode()) {
            continue;
        }
        const bool inCycle = scratch.inCycle(i);
        handler->setFeedbackCycle(inCycle);
        if (inCycle) {
            scratch.writerOf[i] = nodes++;
        }
    }

    // Split them: audio inputs feed the writer; the reader keeps the outputs
    // and the delayTime param, which is read when producing output.
    if (nodes != count) {
        for (Scratch::Edge& edge : scratch.edges) {
            if (!edge.param && scratch.writerOf[edge.to] != none) {
                edge.to = scratch.writerOf[edge.to];
            }
        }
        scratch.buildAdjacency(nodes);
        scratch.computeComponents(nodes);
    }

    // Nodes only matter if something consumes them: the destination, an
    // analyser, a scheduled source that must reach its end, or a delay line.
    std::vector<uint8_t>& needed = scratch.flags;
    needed.assign(nodes, 0);
    std::vector<uint32_t>& pending = scratch.pending;
    auto require = [&](uint32_t node) {
        if (!needed[node]) {
            needed[node] = 1;
            pending.push_back(node);
        }
    };
    if (m_destination) {
        require(m_destination->owner()->graphIndex());
    }
    for (AudioHandler* handler : m_analysisHandlers) {
        require(handler->graphIndex());
    }
    for (AudioHandler* handler : m_scheduledHandlers) {
        require(handler->graphIndex());
    }
    for (uint32_t i = count; i < nodes; i++) {
        require(i);
    }
    while (!pending.empty()) {
        const uint32_t node = pending.back();
        pending.pop_back();
        for (uint32_t i = scratch.reverseStart[node];
             i < scratch.reverseStart[node + 1]; i++) {
            require(scratch.reverse[i]);
        }
    }

    // Emit the needed nodes ordered by component, which is topological.
    std::vector<uint32_t>& order = scratch.finishOrder;
    order.clear();
    for (uint32_t node = 0; node < nodes; node++) {
        if (needed[node]) {
            order.push_back(node);
        }
    }
    // std::sort works in place; std::stable_sort may allocate.
    std::sort(order.begin(), order.end(), [&scratch](uint32_t a, uint32_t b) {
        return scratch.component[a] != scratch.component[b]
                   ? scratch.component[a] < scratch.component[b]
                   : a < b;
    });
    // Delay writers are created after their readers, so map them back.
    std::vector<uint32_t>& delayOf = scratch.pending;
    delayOf.assign(nodes - count, 0);
    for (uint32_t i = 0; i < count; i++) {
        if (scratch.writerOf[i] != none) {
            delayOf[scratch.writerOf[i] - count] = i;
        }
    }
    m_renderSteps.clear();
    for (uint32_t node : order) {
        if (node >= count) {
            m_renderSteps.push_back({ m_handlers[delayOf[node - count]].get(),
                                      StepKind::DelayWrite });
            continue;
        }
        AudioHandler* handler = m_handlers[node].get();
        if (scratch.writerOf[node] != none) {
            m_renderSteps.push_back({ handler, StepKind::DelayRead });
        } else if (scratch.inCycle(node)) {
            handler->setCycleMuted(true);
            m_renderSteps.push_back({ handler, StepKind::Muted });
        } else {
            m_renderSteps.push_back({ handler, StepKind::Process });
        }
    }
    delayOf.clear();
}

bool AudioGraph::isReleasable(AudioHandler* handler) const
{
    if (handler->hasPendingOutput()) {
        // A tail matters only while something consumes this node's output.
        for (size_t index = 0; index < handler->outputCount(); index++) {
            AudioNodeOutput& output = handler->output(index);
            if (!output.destinations().empty()) {
                return false;
            }
            for (AudioParamTimeline* timeline : m_timelines) {
                const auto& sources = timeline->sources();
                if (std::find(sources.begin(), sources.end(), &output) !=
                    sources.end()) {
                    return false;
                }
            }
        }
    }
    for (size_t index = 0; index < handler->timelineCount(); index++) {
        AudioParamTimeline* timeline = handler->timeline(index);
        if (!timeline->isGraphOwned() && timeline->hasWrappers()) {
            return false;
        }
    }
    return true;
}

void AudioGraph::detachHandler(AudioHandler* handler)
{
    for (size_t index = 0; index < handler->inputCount(); index++) {
        handler->input(index).disconnectAll();
    }
    for (size_t index = 0; index < handler->outputCount(); index++) {
        AudioNodeOutput& output = handler->output(index);
        while (!output.destinations().empty()) {
            output.destinations().back()->disconnect(&output);
        }
        for (AudioParamTimeline* timeline : m_timelines) {
            timeline->disconnect(&output);
        }
    }
    for (size_t index = 0; index < handler->timelineCount(); index++) {
        AudioParamTimeline* timeline = handler->timeline(index);
        while (!timeline->sources().empty()) {
            timeline->disconnect(timeline->sources().back());
        }
        if (!timeline->isGraphOwned()) {
            m_timelines.erase(
                std::remove(m_timelines.begin(), m_timelines.end(), timeline),
                m_timelines.end());
        }
    }
    m_scheduledHandlers.erase(std::remove(m_scheduledHandlers.begin(),
                                          m_scheduledHandlers.end(), handler),
                              m_scheduledHandlers.end());
    m_analysisHandlers.erase(std::remove(m_analysisHandlers.begin(),
                                         m_analysisHandlers.end(), handler),
                             m_analysisHandlers.end());
}

// https://webaudio.github.io/web-audio-api/#lifetime-AudioNode
// A node whose wrapper was collected is removed once it cannot affect the
// output any more: it has no pending tail or playback, none of its params is
// still reachable from script, and everything feeding it is removable too.
// While script holds a wrapper, the nodes it connects to stay reachable
// through AudioNode::m_connections, so upstream nodes are always candidates
// as well, except for nodes that are kept alive (the destination).
void AudioGraph::releaseHandlers()
{
    m_releaseDirty = false;
    if (!m_releaseCandidates.empty()) {
        for (AudioHandler* handler : m_releaseCandidates) {
            handler->setReleasable(isReleasable(handler));
        }
        bool changed = true;
        while (changed) {
            changed = false;
            for (AudioHandler* handler : m_releaseCandidates) {
                if (!handler->releasable()) {
                    continue;
                }
                bool blocked = false;
                for (size_t index = 0;
                     !blocked && index < handler->inputCount(); index++) {
                    for (AudioNodeOutput* source :
                         handler->input(index).sources()) {
                        AudioHandler* owner = source->owner();
                        if (!owner->releaseRequested() ||
                            !owner->releasable()) {
                            blocked = true;
                            break;
                        }
                    }
                }
                for (size_t index = 0;
                     !blocked && index < handler->timelineCount(); index++) {
                    AudioParamTimeline* timeline = handler->timeline(index);
                    if (timeline->isGraphOwned()) {
                        continue;
                    }
                    for (AudioNodeOutput* source : timeline->sources()) {
                        AudioHandler* owner = source->owner();
                        if (!owner->releaseRequested() ||
                            !owner->releasable()) {
                            blocked = true;
                            break;
                        }
                    }
                }
                if (blocked) {
                    handler->setReleasable(false);
                    changed = true;
                }
            }
        }
        bool removed = false;
        for (AudioHandler* handler : m_releaseCandidates) {
            if (handler->releasable()) {
                detachHandler(handler);
                removed = true;
            }
        }
        if (removed) {
            m_releaseCandidates.erase(
                std::remove_if(m_releaseCandidates.begin(),
                               m_releaseCandidates.end(),
                               [](AudioHandler* handler) {
                                   return handler->releasable();
                               }),
                m_releaseCandidates.end());
            m_handlers.erase(
                std::remove_if(
                    m_handlers.begin(), m_handlers.end(),
                    [](const std::unique_ptr<AudioHandler>& handler) {
                        return handler->releaseRequested() &&
                               handler->releasable();
                    }),
                m_handlers.end());
            m_topologyDirty = true;
            m_timelineSweepNeeded = true;
        }
    }
    if (m_timelineSweepNeeded) {
        sweepTimelines();
    }
}

// Frees graph-owned timelines that neither a wrapper nor a handler uses.
void AudioGraph::sweepTimelines()
{
    m_timelineSweepNeeded = false;
    bool candidates = false;
    for (const auto& timeline : m_ownedTimelines) {
        candidates |= timeline->wasWrapped() && !timeline->hasWrappers();
    }
    if (!candidates) {
        return;
    }
    auto referenced = [this](AudioParamTimeline* timeline) {
        for (const auto& handler : m_handlers) {
            for (size_t index = 0; index < handler->timelineCount(); index++) {
                if (handler->timeline(index) == timeline) {
                    return true;
                }
            }
        }
        return false;
    };
    for (auto it = m_ownedTimelines.begin(); it != m_ownedTimelines.end();) {
        AudioParamTimeline* timeline = it->get();
        if (!timeline->wasWrapped() || timeline->hasWrappers() ||
            referenced(timeline)) {
            ++it;
            continue;
        }
        while (!timeline->sources().empty()) {
            timeline->disconnect(timeline->sources().back());
        }
        m_timelines.erase(
            std::remove(m_timelines.begin(), m_timelines.end(), timeline),
            m_timelines.end());
        it = m_ownedTimelines.erase(it);
    }
}

void AudioGraph::registerAnalysisHandler(AudioHandler* handler)
{
    m_analysisHandlers.push_back(handler);
    m_topologyDirty = true;
}

void AudioGraph::registerTimeline(AudioParamTimeline* timeline)
{
    m_timelines.push_back(timeline);
}

AudioParamTimeline* AudioGraph::createTimeline(float initialValue)
{
    AudioGraphLock lock(this);
    auto timeline = std::unique_ptr<AudioParamTimeline>(
        new AudioParamTimeline(initialValue));
    AudioParamTimeline* result = timeline.get();
    result->setGraphOwned();
    m_ownedTimelines.push_back(std::move(timeline));
    registerTimeline(result);
    return result;
}

void AudioGraph::setDestination(AudioNodeOutput* destination)
{
    m_destination = destination;
    m_topologyDirty = true;
}

void AudioGraph::renderQuantum(AudioBus& destination, size_t frames)
{
    processPendingConnections();
    STARFISH_ASSERT(destination.frames() == AudioBus::RenderQuantumFrames);
    STARFISH_ASSERT(frames >= 1 && frames <= AudioBus::RenderQuantumFrames);
    // Handle new collection reports now; re-check the tails of collected
    // nodes that are still sounding only every few quanta.
    constexpr uint64_t ReleaseInterval = 64;
    if (m_releaseDirty ||
        (!m_releaseCandidates.empty() && m_quantum % ReleaseInterval == 0)) {
        releaseHandlers();
    }
    if (m_topologyDirty) {
        refreshTopology();
    }
    const double time = currentTime();
    for (AudioParamTimeline* timeline : m_timelines) {
        timeline->beginQuantum(time);
    }
    ++m_quantum;
    const uint64_t frameStart =
        m_renderedFrames.load(std::memory_order_relaxed);
    for (const RenderStep& step : m_renderSteps) {
        switch (step.kind) {
        case StepKind::Process:
            step.handler->render(frameStart, frames);
            break;
        case StepKind::Muted:
            step.handler->renderMuted();
            break;
        case StepKind::DelayRead:
            step.handler->renderDelayRead(frameStart, frames);
            break;
        case StepKind::DelayWrite:
            step.handler->renderDelayWrite(frameStart, frames);
            break;
        }
    }
    if (m_destination) {
        destination.copyFrom(m_destination->bus(), frames, true);
    } else {
        destination.zero();
    }
    // Disconnected scheduled sources were rendered above so that they still
    // advance and finish. Only their native state crosses back to the
    // control thread.
    m_scheduledHandlers.erase(std::remove_if(m_scheduledHandlers.begin(),
                                             m_scheduledHandlers.end(),
                                             [](AudioHandler* handler) {
                                                 return handler->finished();
                                             }),
                              m_scheduledHandlers.end());
    // Rendering time advances by a full quantum even when an offline
    // destination stores only the final partial quantum.
    m_renderedFrames.fetch_add(AudioBus::RenderQuantumFrames,
                               std::memory_order_release);
}

} // namespace Starfish

#endif
