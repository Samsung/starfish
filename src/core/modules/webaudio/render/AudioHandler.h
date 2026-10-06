/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioHandler__
#define __StarfishAudioHandler__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Starfish {

class AudioGraph;
class AudioNodeInput;
class AudioNodeOutput;
class AudioParamTimeline;

// Render handlers have no GC references. The graph owns their lifetime, and
// configuration/connection changes are made between render quanta.
class AudioHandler {
public:
    AudioHandler(size_t inputs, size_t outputs, size_t outputChannels);
    // Destructors release only native resources owned by this handler; the
    // graph has detached every connection before deleting a handler.
    virtual ~AudioHandler();

    AudioNodeInput& input(size_t index);
    AudioNodeOutput& output(size_t index);
    size_t inputCount() const
    {
        return m_inputs.size();
    }
    size_t outputCount() const
    {
        return m_outputs.size();
    }
    // Sums this quantum's inputs (already rendered upstream) and processes.
    void render(uint64_t frameStart, size_t frames);
    // https://webaudio.github.io/web-audio-api/#rendering-an-audio-graph
    // Nodes in a delay-free cycle output silence.
    void renderMuted();
    // A DelayNode in a cycle is split into a reader (a source) and a writer.
    virtual void renderDelayRead(uint64_t, size_t)
    {
    }
    virtual void renderDelayWrite(uint64_t, size_t)
    {
    }
    void setGraph(AudioGraph* graph)
    {
        m_graph = graph;
    }
    AudioGraph* graph() const
    {
        return m_graph;
    }
    void setCycleMuted(bool muted)
    {
        m_cycleMuted = muted;
    }
    virtual bool isDelayNode() const
    {
        return false;
    }
    virtual void setFeedbackCycle(bool)
    {
    }
    virtual void inputChannelsChanged(size_t index);
    virtual bool finished() const
    {
        return false;
    }
    // Automation timelines read while processing: the handler's own
    // AudioParams and any shared ones (PannerNode reads the listener's).
    virtual size_t timelineCount() const
    {
        return 0;
    }
    virtual AudioParamTimeline* timeline(size_t index);
    // https://webaudio.github.io/web-audio-api/#AudioNode-actively-processing
    // Whether this node can still produce sound without new input: a
    // playing source, or a tail that has not decayed. Checked before a node
    // whose wrapper was collected is removed from the graph.
    virtual bool hasPendingOutput() const;
    // Graph teardown only: forget every connection without notifying the
    // peers, which are being destroyed too.
    void detachForTeardown();

    // Scratch fields owned by AudioGraph's topology and release passes.
    uint32_t graphIndex() const
    {
        return m_graphIndex;
    }
    void setGraphIndex(uint32_t index)
    {
        m_graphIndex = index;
    }
    bool releaseRequested() const
    {
        return m_releaseRequested;
    }
    void setReleaseRequested()
    {
        m_releaseRequested = true;
    }
    bool releasable() const
    {
        return m_releasable;
    }
    void setReleasable(bool releasable)
    {
        m_releasable = releasable;
    }

protected:
    virtual void process(uint64_t frameStart, size_t frames) = 0;
    void zeroOutputs();

private:
    std::vector<std::unique_ptr<AudioNodeInput>> m_inputs;
    std::vector<std::unique_ptr<AudioNodeOutput>> m_outputs;
    AudioGraph* m_graph{ nullptr };
    uint32_t m_graphIndex{ 0 };
    bool m_cycleMuted{ false };
    bool m_releaseRequested{ false };
    bool m_releasable{ false };
};

} // namespace Starfish

#endif
#endif
