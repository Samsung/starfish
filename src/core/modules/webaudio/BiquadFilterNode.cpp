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
#include "core/modules/webaudio/BiquadFilterNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <cmath>

#include "EscargotPublic.h"

namespace Starfish {

BiquadFilterNode::BiquadFilterNode(ExecutionContext* executionContext,
                                   BaseAudioContext* context,
                                   BiquadFilterOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    if (!std::isfinite(options.frequency()) ||
        !std::isfinite(options.detune()) || !std::isfinite(options.Q()) ||
        !std::isfinite(options.gain())) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "Biquad options must be finite");
    }
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_filterHandler = static_cast<BiquadFilterHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new BiquadFilterHandler(context->sampleRate()))));
    m_handler = m_filterHandler;
    m_frequency = new AudioParam(context, m_filterHandler->frequencyTimeline());
    m_detune = new AudioParam(context, m_filterHandler->detuneTimeline());
    m_Q = new AudioParam(context, m_filterHandler->QTimeline());
    m_gain = new AudioParam(context, m_filterHandler->gainTimeline());
    context->graph()->registerTimeline(m_filterHandler->frequencyTimeline());
    context->graph()->registerTimeline(m_filterHandler->detuneTimeline());
    context->graph()->registerTimeline(m_filterHandler->QTimeline());
    context->graph()->registerTimeline(m_filterHandler->gainTimeline());
    m_filterHandler->frequencyTimeline()->setInitialValue(options.frequency());
    m_filterHandler->detuneTimeline()->setInitialValue(options.detune());
    m_filterHandler->QTimeline()->setInitialValue(options.Q());
    m_filterHandler->gainTimeline()->setInitialValue(options.gain());
    setType(options.type());
    configureInputs();
    applyOptions(options);
}

void BiquadFilterNode::setType(String* type)
{
    AudioGraphLock graphLock(context()->graph());
    BiquadFilterHandler::Type filterType;
    if (type->equals("lowpass")) {
        filterType = BiquadFilterHandler::Type::Lowpass;
    } else if (type->equals("highpass")) {
        filterType = BiquadFilterHandler::Type::Highpass;
    } else if (type->equals("bandpass")) {
        filterType = BiquadFilterHandler::Type::Bandpass;
    } else if (type->equals("lowshelf")) {
        filterType = BiquadFilterHandler::Type::Lowshelf;
    } else if (type->equals("highshelf")) {
        filterType = BiquadFilterHandler::Type::Highshelf;
    } else if (type->equals("peaking")) {
        filterType = BiquadFilterHandler::Type::Peaking;
    } else if (type->equals("notch")) {
        filterType = BiquadFilterHandler::Type::Notch;
    } else {
        filterType = BiquadFilterHandler::Type::Allpass;
    }
    m_type = type;
    m_filterHandler->setType(filterType);
}

void BiquadFilterNode::getFrequencyResponse(ScriptFloat32Array frequencyHz,
                                            ScriptFloat32Array magResponse,
                                            ScriptFloat32Array phaseResponse)
{
    AudioGraphLock graphLock(context()->graph());
    const size_t length = frequencyHz->arrayLength();
    if (magResponse->arrayLength() != length ||
        phaseResponse->arrayLength() != length) {
        throw new DOMException(
            executionContext(), DOMException::INVALID_ACCESS_ERR,
            "Frequency response arrays must have equal lengths");
    }
    const float* frequency = reinterpret_cast<const float*>(
        frequencyHz->rawBuffer() + frequencyHz->byteOffset());
    float* magnitude = reinterpret_cast<float*>(magResponse->rawBuffer() +
                                                magResponse->byteOffset());
    float* phase = reinterpret_cast<float*>(phaseResponse->rawBuffer() +
                                            phaseResponse->byteOffset());
    m_filterHandler->getFrequencyResponse(frequency, magnitude, phase, length,
                                          context()->currentTime());
}

ScriptBindingInstance* BiquadFilterNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
#endif
