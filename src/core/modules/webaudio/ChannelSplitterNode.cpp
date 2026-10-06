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
#include "core/modules/webaudio/ChannelSplitterNode.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

namespace Starfish {

ChannelSplitterNode::ChannelSplitterNode(ExecutionContext* executionContext,
                                         BaseAudioContext* context,
                                         ChannelSplitterOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    const uint32_t outputs = options.numberOfOutputs();
    if (outputs < 1 || outputs > 32) {
        throw new DOMException(executionContext, DOMException::INDEX_SIZE_ERR,
                               "Unsupported channel splitter output count");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = outputs;
    m_channelCount = outputs;
    m_channelCountMode = ChannelCountMode::Explicit;
    m_channelInterpretation = ChannelInterpretation::Discrete;
    m_handler = context->graph()->addHandler(
        std::unique_ptr<AudioHandler>(new ChannelSplitterHandler(outputs)));
    applyOptions(options);
    configureInputs();
}

ScriptBindingInstance* ChannelSplitterNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void ChannelSplitterNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value != m_numberOfOutputs) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Channel splitter count matches outputs");
    }
    AudioNode::setChannelCount(value);
}

void ChannelSplitterNode::setChannelCountModeStr(String* mode)
{
    AudioGraphLock graphLock(context()->graph());
    if (!mode->equals("explicit")) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Channel splitter requires explicit mode");
    }
    AudioNode::setChannelCountModeStr(mode);
}

void ChannelSplitterNode::setChannelInterpretationStr(String* interpretation)
{
    AudioGraphLock graphLock(context()->graph());
    if (!interpretation->equals("discrete")) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Channel splitter requires discrete mixing");
    }
    AudioNode::setChannelInterpretationStr(interpretation);
}

} // namespace Starfish
#endif
