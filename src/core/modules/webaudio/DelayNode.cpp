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
#include "core/modules/webaudio/DelayNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <cmath>

namespace Starfish {

DelayNode::DelayNode(ExecutionContext* executionContext,
                     BaseAudioContext* context, DelayOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-createdelay
    if (!std::isfinite(options.maxDelayTime()) || options.maxDelayTime() <= 0 ||
        options.maxDelayTime() >= 180) {
        throw new DOMException(
            executionContext, DOMException::NOT_SUPPORTED_ERR,
            "maxDelayTime must be greater than zero and less than 180 seconds");
    }
    if (!std::isfinite(options.delayTime())) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "delayTime must be finite");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    auto* handler = static_cast<DelayHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new DelayHandler(context->sampleRate(), options.maxDelayTime()))));
    m_handler = handler;
    m_delayTime = new AudioParam(context, handler->delayTimeline());
    context->graph()->registerTimeline(handler->delayTimeline());
    handler->delayTimeline()->setInitialValue(options.delayTime());
    configureInputs();
    applyOptions(options);
}

ScriptBindingInstance* DelayNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
