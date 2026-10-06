/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/ConvolverNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/ConvolverHandler.h"

namespace Starfish {

ConvolverNode::ConvolverNode(ExecutionContext* executionContext,
                             BaseAudioContext* context,
                             ConvolverOptions options)
    : AudioNode(executionContext, context)
{
    {
        AudioGraphLock graphLock(context->graph());
        m_numberOfInputs = 1;
        m_numberOfOutputs = 1;
        m_channelCountMode = ChannelCountMode::ClampedMax;
        m_convolverHandler =
            static_cast<ConvolverHandler*>(context->graph()->addHandler(
                std::unique_ptr<AudioHandler>(new ConvolverHandler())));
        m_handler = m_convolverHandler;
        configureInputs();
    }
    m_normalize = !options.disableNormalization();
    if (options.buffer()) {
        setBuffer(options.buffer());
    }
    applyOptions(options);
}

ScriptBindingInstance* ConvolverNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-convolvernode-buffer
void ConvolverNode::setBuffer(Optional<AudioBuffer*> buffer)
{
    std::unique_ptr<ConvolverHandler::Response> response;
    if (buffer) {
        if ((buffer->numberOfChannels() != 1 &&
             buffer->numberOfChannels() != 2 &&
             buffer->numberOfChannels() != 4) ||
            buffer->sampleRate() != context()->sampleRate() ||
            buffer->length() > context()->sampleRate() * 5 ||
            buffer->length() > 131072) {
            // Cap both duration and FFT partition count. At high sample rates
            // a time-only limit would consume too much memory per node.
            throw new DOMException(executionContext(),
                                   DOMException::NOT_SUPPORTED_ERR,
                                   "Unsupported impulse response");
        }
        buffer->acquireContents();
        // The FFTs of a long response would stall rendering if they ran
        // under the graph lock; only the swap needs it.
        response = ConvolverHandler::createResponse(buffer->data(), m_normalize,
                                                    context()->sampleRate());
    }
    {
        AudioGraphLock graphLock(context()->graph());
        response = m_convolverHandler->swapResponse(std::move(response));
    }
    m_buffer = buffer;
}

void ConvolverNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value < 1 || value > 2) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Convolver input must be mono or stereo");
    }
    AudioNode::setChannelCount(value);
}

void ConvolverNode::setChannelCountModeStr(String* mode)
{
    AudioGraphLock graphLock(context()->graph());
    if (mode->equals("max")) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Convolver cannot use max channel mode");
    }
    AudioNode::setChannelCountModeStr(mode);
}

void ConvolverNode::setChannelInterpretationStr(String* interpretation)
{
    AudioGraphLock graphLock(context()->graph());
    AudioNode::setChannelInterpretationStr(interpretation);
    m_convolverHandler->setSpeakers(m_channelInterpretation ==
                                    ChannelInterpretation::Speakers);
}

} // namespace Starfish
#endif
