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
#include "core/modules/webaudio/ChannelMergerNode.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

namespace Starfish {

ChannelMergerNode::ChannelMergerNode(ExecutionContext* executionContext,
                                     BaseAudioContext* context,
                                     ChannelMergerOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    const uint32_t inputs = options.numberOfInputs();
    if (inputs < 1 || inputs > 32) {
        throw new DOMException(executionContext, DOMException::INDEX_SIZE_ERR,
                               "Unsupported channel merger input count");
    }
    m_numberOfInputs = inputs;
    m_numberOfOutputs = 1;
    m_channelCount = 1;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_handler = context->graph()->addHandler(
        std::unique_ptr<AudioHandler>(new ChannelMergerHandler(inputs)));
    applyOptions(options);
    configureInputs();
}

ScriptBindingInstance* ChannelMergerNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void ChannelMergerNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value != 1) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Channel merger requires channelCount 1");
    }
    AudioNode::setChannelCount(value);
}

void ChannelMergerNode::setChannelCountModeStr(String* mode)
{
    AudioGraphLock graphLock(context()->graph());
    if (!mode->equals("explicit")) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Channel merger requires explicit mode");
    }
    AudioNode::setChannelCountModeStr(mode);
}

} // namespace Starfish
#endif
