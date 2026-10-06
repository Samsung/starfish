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
#include "core/modules/webaudio/DynamicsCompressorNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <cmath>

namespace Starfish {

DynamicsCompressorNode::DynamicsCompressorNode(
    ExecutionContext* executionContext, BaseAudioContext* context,
    DynamicsCompressorOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    if (!std::isfinite(options.threshold()) || !std::isfinite(options.knee()) ||
        !std::isfinite(options.ratio()) || !std::isfinite(options.attack()) ||
        !std::isfinite(options.release())) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "Compressor parameters must be finite");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_channelCountMode = ChannelCountMode::ClampedMax;
    m_compressorHandler = static_cast<DynamicsCompressorHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new DynamicsCompressorHandler(context->sampleRate()))));
    m_handler = m_compressorHandler;
    m_threshold =
        new AudioParam(context, m_compressorHandler->thresholdTimeline());
    m_knee = new AudioParam(context, m_compressorHandler->kneeTimeline());
    m_ratio = new AudioParam(context, m_compressorHandler->ratioTimeline());
    m_attack = new AudioParam(context, m_compressorHandler->attackTimeline());
    m_release = new AudioParam(context, m_compressorHandler->releaseTimeline());
    context->graph()->registerTimeline(
        m_compressorHandler->thresholdTimeline());
    context->graph()->registerTimeline(m_compressorHandler->kneeTimeline());
    context->graph()->registerTimeline(m_compressorHandler->ratioTimeline());
    context->graph()->registerTimeline(m_compressorHandler->attackTimeline());
    context->graph()->registerTimeline(m_compressorHandler->releaseTimeline());
    m_compressorHandler->thresholdTimeline()->setInitialValue(
        options.threshold());
    m_compressorHandler->kneeTimeline()->setInitialValue(options.knee());
    m_compressorHandler->ratioTimeline()->setInitialValue(options.ratio());
    m_compressorHandler->attackTimeline()->setInitialValue(options.attack());
    m_compressorHandler->releaseTimeline()->setInitialValue(options.release());
    configureInputs();
    applyOptions(options);
}

float DynamicsCompressorNode::reduction() const
{
    AudioGraphLock graphLock(m_context->graph());
    return m_compressorHandler->reduction();
}

void DynamicsCompressorNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value > 2) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Compressor accepts at most two channels");
    }
    AudioNode::setChannelCount(value);
}

void DynamicsCompressorNode::setChannelCountModeStr(String* mode)
{
    AudioGraphLock graphLock(context()->graph());
    if (mode->equals("max")) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Compressor does not support max channel mode");
    }
    AudioNode::setChannelCountModeStr(mode);
}

ScriptBindingInstance* DynamicsCompressorNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
