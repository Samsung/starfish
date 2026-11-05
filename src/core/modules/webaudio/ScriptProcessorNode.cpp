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
#include "core/modules/webaudio/ScriptProcessorNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/AudioProcessingEvent.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "core/modules/webaudio/render/AudioHandlers.h"
#include "core/modules/webaudio/render/AudioNodeInput.h"

#include <algorithm>
#include <cstring>
#include <new>

namespace Starfish {

ScriptProcessorNode::ScriptProcessorNode(ExecutionContext* executionContext,
                                         BaseAudioContext* context,
                                         uint32_t bufferSize,
                                         uint32_t numberOfInputChannels,
                                         uint32_t numberOfOutputChannels)
    : AudioNode(executionContext, context)
    , m_bufferSize(bufferSize)
    , m_numberOfInputChannels(numberOfInputChannels)
    , m_numberOfOutputChannels(numberOfOutputChannels)
{
    AudioGraphLock graphLock(context->graph());
    // https://webaudio.github.io/web-audio-api/#ScriptProcessorNode
    // One input and one output; channelCount is numberOfInputChannels with
    // mode "explicit", and neither can change.
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_channelCount = numberOfInputChannels;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_processorHandler =
        static_cast<ScriptProcessorHandler*>(context->addHandler(
            std::unique_ptr<AudioHandler>(new ScriptProcessorHandler(
                bufferSize, numberOfInputChannels, numberOfOutputChannels))));
    m_handler = m_processorHandler;
    try {
        m_handler->input(0).configure(
            std::max<uint32_t>(numberOfInputChannels, 1),
            AudioInputChannelMode::Explicit, true);
    } catch (const std::bad_alloc&) {
        context->throwAllocationError();
    }
    // Rendered every quantum, even when nothing downstream pulls it, so an
    // input-only node still gets its events.
    context->graph()->registerAnalysisHandler(m_handler);
    context->registerScriptProcessor(this);
}

ScriptBindingInstance* ScriptProcessorNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

EventListener* ScriptProcessorNode::onaudioprocess()
{
    return attributeEventListener(staticStrings()->m_audioprocess);
}

void ScriptProcessorNode::setOnaudioprocess(EventListener* listener)
{
    QualifiedName attr = staticStrings()->m_audioprocess;
    if (!listener) {
        clearAttributeEventListener(attr);
        return;
    }
    setAttributeEventListener(attr, listener);
    // The context keeps a node with a listener alive and dispatches to it.
    context()->registerScriptProcessor(this);
}

// https://webaudio.github.io/web-audio-api/#dom-audionode-channelcount
void ScriptProcessorNode::setChannelCount(uint32_t value)
{
    if (value != m_channelCount) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "ScriptProcessorNode channelCount is fixed");
    }
}

// https://webaudio.github.io/web-audio-api/#dom-audionode-channelcountmode
void ScriptProcessorNode::setChannelCountModeStr(String* channelCountMode)
{
    if (!channelCountMode->equals("explicit")) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "ScriptProcessorNode channelCountMode is fixed");
    }
}

bool ScriptProcessorNode::dispatchAudioProcessEvents()
{
    String* type = staticStrings()->m_audioprocess.localName();
    AudioGraph* graph = context()->graph();
    // Only this node's own listeners count: an AudioNode has no event path,
    // and hasListenerForTypeOnPath() answers true for non-Node targets.
    auto listeners = getEventListeners(type);
    if (!listeners || listeners->empty()) {
        // Stay registered while connected, so a listener added later with
        // addEventListener() still gets events; blocks that completed with
        // no listener are dropped rather than delivered late.
        AudioGraphLock graphLock(graph);
        for (int slot; (slot = m_processorHandler->pendingSlot()) >= 0;) {
            m_processorHandler->beginEvent(slot);
        }
        return m_processorHandler->isConnected();
    }
    AudioBufferOptions options;
    options.setLength(m_bufferSize);
    options.setSampleRate(context()->sampleRate());
    // At most one event per slot: in a real-time context the render thread
    // keeps completing blocks while script runs.
    for (int events = 0; events < 2; events++) {
        // A channel that script detached stays empty, so a reused input or
        // output buffer with one is replaced.
        if (!m_inputBuffer || m_inputBuffer.value()->hasDetachedChannel()) {
            options.setNumberOfChannels(
                std::max<uint32_t>(m_numberOfInputChannels, 1));
            m_inputBuffer = new AudioBuffer(executionContext(), options);
        }
        int slot;
        uint64_t block;
        uint64_t startFrame;
        AudioBuffer* output;
        {
            AudioGraphLock graphLock(graph);
            slot = m_processorHandler->pendingSlot();
            if (slot < 0) {
                break;
            }
            if (!m_outputBuffers[slot] ||
                m_outputBuffers[slot].value()->hasDetachedChannel()) {
                options.setNumberOfChannels(
                    std::max<uint32_t>(m_numberOfOutputChannels, 1));
                m_outputBuffers[slot] =
                    new AudioBuffer(executionContext(), options);
            }
            output = m_outputBuffers[slot].value();
            startFrame = m_processorHandler->pendingStartFrame(slot);
            block = m_processorHandler->beginEvent(slot);
            for (uint32_t channel = 0; channel < m_numberOfInputChannels;
                 channel++) {
                memcpy(m_inputBuffer.value()->mutableChannel(channel),
                       m_processorHandler->inputBlock(slot, channel),
                       m_bufferSize * sizeof(float));
            }
            for (uint32_t channel = 0; channel < output->numberOfChannels();
                 channel++) {
                memset(output->mutableChannel(channel), 0,
                       m_bufferSize * sizeof(float));
            }
        }
        // https://webaudio.github.io/web-audio-api/#dom-audioprocessingevent-playbacktime
        // "The time when the audio will be played in the same time
        // coordinate system as the AudioContext's currentTime": the output
        // plays two blocks after its input block started.
        AudioProcessingEventInit init;
        init.setPlaybackTime(
            static_cast<double>(startFrame + 2 * m_bufferSize) /
            context()->sampleRate());
        init.setInputBuffer(m_inputBuffer.value());
        init.setOutputBuffer(output);
        dispatchEventByUA(
            new AudioProcessingEvent(executionContext(), type, init));
        // "When the dispatch of an AudioProcessingEvent completes, it
        // acquires the contents of its outputBuffer."
        AudioGraphLock graphLock(graph);
        // A detached channel leaves no output for this event: silence.
        m_processorHandler->setOutput(
            slot, block, output->acquireContents() ? output->data() : nullptr);
    }
    return true;
}

} // namespace Starfish

#endif
