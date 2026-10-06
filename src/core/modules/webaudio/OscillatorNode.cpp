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
#include "core/modules/webaudio/OscillatorNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/PeriodicWave.h"
#include "core/modules/webaudio/render/AudioHandlers.h"

#include <cmath>

namespace Starfish {

OscillatorNode::OscillatorNode(ExecutionContext* executionContext,
                               BaseAudioContext* context,
                               OscillatorOptions options)
    : AudioScheduledSourceNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    if (!std::isfinite(options.frequency()) ||
        !std::isfinite(options.detune())) {
        throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                               "Oscillator options must be finite");
    }
    if (options.type()->equals("custom") && !options.periodicWave()) {
        throw new DOMException(executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "A custom oscillator requires a PeriodicWave");
    }
    m_numberOfInputs = 0;
    m_numberOfOutputs = 1;
    m_sourceHandler = static_cast<OscillatorHandler*>(
        context->graph()->addHandler(std::unique_ptr<AudioHandler>(
            new OscillatorHandler(context->sampleRate()))));
    m_handler = m_sourceHandler;
    m_frequency = new AudioParam(context, m_sourceHandler->frequencyTimeline());
    m_detune = new AudioParam(context, m_sourceHandler->detuneTimeline());
    context->graph()->registerTimeline(m_sourceHandler->frequencyTimeline());
    context->graph()->registerTimeline(m_sourceHandler->detuneTimeline());
    m_sourceHandler->frequencyTimeline()->setInitialValue(options.frequency());
    m_sourceHandler->detuneTimeline()->setInitialValue(options.detune());
    if (options.periodicWave()) {
        setPeriodicWave(options.periodicWave());
    } else {
        setType(options.type());
    }
    applyOptions(options);
}

ScriptBindingInstance* OscillatorNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void OscillatorNode::setType(String* type)
{
    AudioGraphLock graphLock(context()->graph());
    if (type->equals("custom")) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "Use setPeriodicWave for a custom oscillator");
    }
    OscillatorHandler::Waveform waveform;
    if (type->equals("sine")) {
        waveform = OscillatorHandler::Waveform::Sine;
    } else if (type->equals("square")) {
        waveform = OscillatorHandler::Waveform::Square;
    } else if (type->equals("sawtooth")) {
        waveform = OscillatorHandler::Waveform::Sawtooth;
    } else if (type->equals("triangle")) {
        waveform = OscillatorHandler::Waveform::Triangle;
    } else {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "Invalid oscillator type");
    }
    m_type = type;
    m_periodicWave = nullptr;
    m_sourceHandler->setWaveform(waveform);
}

void OscillatorNode::setPeriodicWave(PeriodicWave* periodicWave)
{
    AudioGraphLock graphLock(context()->graph());
    if (periodicWave->context() != context()) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_ACCESS_ERR,
                               "PeriodicWave belongs to another context");
    }
    m_periodicWave = periodicWave;
    m_type = String::createASCIIString("custom");
    m_sourceHandler->setPeriodicWave(periodicWave->data());
}

void OscillatorNode::start(double when)
{
    AudioGraphLock graphLock(context()->graph());
    AudioScheduledSourceNode::start(when);
    m_sourceHandler->start(when);
    context()->registerScheduledSource(this);
}

void OscillatorNode::stop(double when)
{
    AudioGraphLock graphLock(context()->graph());
    AudioScheduledSourceNode::stop(when);
    m_sourceHandler->stop(when);
}

bool OscillatorNode::renderScheduledQuantum()
{
    AudioGraphLock graphLock(context()->graph());
    return m_sourceHandler->finished();
}

} // namespace Starfish
#endif
