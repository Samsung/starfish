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
#include "core/modules/webaudio/GainNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <cmath>

namespace Starfish {

GainNode::GainNode(ExecutionContext* executionContext,
                   BaseAudioContext* context, GainOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    if (!std::isfinite(options.gain())) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Gain must be finite");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    auto* handler = static_cast<GainHandler*>(context->graph()->addHandler(
        std::unique_ptr<AudioHandler>(new GainHandler(context->sampleRate()))));
    m_handler = handler;
    m_gain = new AudioParam(context, handler->gainTimeline());
    context->graph()->registerTimeline(handler->gainTimeline());
    handler->gainTimeline()->setInitialValue(options.gain());
    applyOptions(options);
}

ScriptBindingInstance* GainNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
