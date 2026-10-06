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
#include "core/modules/webaudio/StereoPannerNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

namespace Starfish {

StereoPannerNode::StereoPannerNode(ExecutionContext* executionContext,
                                   BaseAudioContext* context,
                                   StereoPannerOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_channelCountMode = ChannelCountMode::ClampedMax;
    auto* handler = static_cast<StereoPannerHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new StereoPannerHandler(context->sampleRate()))));
    m_handler = handler;
    m_pan = new AudioParam(context, handler->panTimeline());
    context->graph()->registerTimeline(handler->panTimeline());
    handler->panTimeline()->setInitialValue(options.pan());
    configureInputs();
    applyOptions(options);
}

ScriptBindingInstance* StereoPannerNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void StereoPannerNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value > 2) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Stereo panner accepts at most two channels");
    }
    AudioNode::setChannelCount(value);
}

void StereoPannerNode::setChannelCountModeStr(String* mode)
{
    AudioGraphLock graphLock(context()->graph());
    if (mode->equals("max")) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Stereo panner does not support max mode");
    }
    AudioNode::setChannelCountModeStr(mode);
}

} // namespace Starfish
#endif
