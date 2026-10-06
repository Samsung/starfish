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
#include "core/modules/webaudio/ConstantSourceNode.h"

#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

namespace Starfish {

ConstantSourceNode::ConstantSourceNode(ExecutionContext* executionContext,
                                       BaseAudioContext* context,
                                       ConstantSourceOptions options)
    : AudioScheduledSourceNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    m_numberOfInputs = 0;
    m_numberOfOutputs = 1;
    m_sourceHandler = static_cast<ConstantSourceHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new ConstantSourceHandler(context->sampleRate()))));
    m_handler = m_sourceHandler;
    m_offset = new AudioParam(context, m_sourceHandler->offsetTimeline());
    context->graph()->registerTimeline(m_sourceHandler->offsetTimeline());
    m_sourceHandler->offsetTimeline()->setInitialValue(options.offset());
    applyOptions(options);
}

ScriptBindingInstance* ConstantSourceNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void ConstantSourceNode::start(double when)
{
    AudioGraphLock graphLock(context()->graph());
    AudioScheduledSourceNode::start(when);
    m_sourceHandler->start(when);
    context()->registerScheduledSource(this);
}

void ConstantSourceNode::stop(double when)
{
    AudioGraphLock graphLock(context()->graph());
    AudioScheduledSourceNode::stop(when);
    m_sourceHandler->stop(when);
}

bool ConstantSourceNode::renderScheduledQuantum()
{
    AudioGraphLock graphLock(context()->graph());
    return m_sourceHandler->finished();
}

} // namespace Starfish
#endif
