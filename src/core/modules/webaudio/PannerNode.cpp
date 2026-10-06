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
#include "core/modules/webaudio/PannerNode.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/webaudio/AudioListener.h"
#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/PannerHandler.h"

#include <cmath>
#include <limits>

namespace Starfish {

PannerNode::PannerNode(ExecutionContext* executionContext,
                       BaseAudioContext* context, PannerOptions options)
    : AudioNode(executionContext, context)
{
    AudioGraphLock graphLock(context->graph());
    m_numberOfInputs = 1;
    m_numberOfOutputs = 1;
    m_channelCountMode = ChannelCountMode::ClampedMax;
    AudioGraph* graph = context->graph();
    m_positionX =
        new AudioParam(context, graph->createTimeline(options.positionX()));
    m_positionY =
        new AudioParam(context, graph->createTimeline(options.positionY()));
    m_positionZ =
        new AudioParam(context, graph->createTimeline(options.positionZ()));
    m_orientationX =
        new AudioParam(context, graph->createTimeline(options.orientationX()));
    m_orientationY =
        new AudioParam(context, graph->createTimeline(options.orientationY()));
    m_orientationZ =
        new AudioParam(context, graph->createTimeline(options.orientationZ()));
    const std::array<AudioParamTimeline*, 6> source = {
        m_positionX->timeline(),    m_positionY->timeline(),
        m_positionZ->timeline(),    m_orientationX->timeline(),
        m_orientationY->timeline(), m_orientationZ->timeline()
    };
    AudioListener* listener = context->listener();
    const std::array<AudioParamTimeline*, 9> listenerParams = {
        listener->positionX()->timeline(), listener->positionY()->timeline(),
        listener->positionZ()->timeline(), listener->forwardX()->timeline(),
        listener->forwardY()->timeline(),  listener->forwardZ()->timeline(),
        listener->upX()->timeline(),       listener->upY()->timeline(),
        listener->upZ()->timeline()
    };
    m_pannerHandler = static_cast<PannerHandler*>(
        graph->addHandler(std::unique_ptr<AudioHandler>(
            new PannerHandler(context->sampleRate(), source, listenerParams))));
    m_handler = m_pannerHandler;
    setPanningModel(options.panningModel());
    setDistanceModel(options.distanceModel());
    setRefDistance(options.refDistance());
    setMaxDistance(options.maxDistance());
    setRolloffFactor(options.rolloffFactor());
    setConeInnerAngle(options.coneInnerAngle());
    setConeOuterAngle(options.coneOuterAngle());
    setConeOuterGain(options.coneOuterGain());
    configureInputs();
    applyOptions(options);
}

ScriptBindingInstance* PannerNode::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

void PannerNode::setPanningModel(String* model)
{
    AudioGraphLock graphLock(context()->graph());
    if (!model->equals("equalpower") && !model->equals("HRTF")) {
        return;
    }
    // https://webaudio.github.io/web-audio-api/#enumdef-panningmodeltype
    // HRTF impulse responses are intentionally omitted to keep the engine's
    // memory footprint low; equalpower remains the rendering fallback.
    m_panningModel = model;
}

void PannerNode::setDistanceModel(String* model)
{
    AudioGraphLock graphLock(context()->graph());
    if (!model->equals("linear") && !model->equals("inverse") &&
        !model->equals("exponential")) {
        return;
    }
    m_distanceModel = model;
    PannerHandler::DistanceModel value = PannerHandler::DistanceModel::Inverse;
    if (model->equals("linear")) {
        value = PannerHandler::DistanceModel::Linear;
    } else if (model->equals("exponential")) {
        value = PannerHandler::DistanceModel::Exponential;
    }
    m_pannerHandler->setDistanceModel(value);
}

void PannerNode::setRefDistance(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!std::isfinite(value) || value < 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "refDistance must be nonnegative");
    }
    m_refDistance = value;
    m_pannerHandler->setRefDistance(value);
}

void PannerNode::setMaxDistance(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!std::isfinite(value) || value <= 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "maxDistance must be positive");
    }
    m_maxDistance = value;
    m_pannerHandler->setMaxDistance(value);
}

void PannerNode::setRolloffFactor(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!std::isfinite(value) || value < 0) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "rolloffFactor must be nonnegative");
    }
    m_rolloffFactor = value;
    m_pannerHandler->setRolloffFactor(value);
}

void PannerNode::setConeInnerAngle(double value)
{
    AudioGraphLock graphLock(context()->graph());
    m_coneInnerAngle = value;
    m_pannerHandler->setConeInnerAngle(value);
}

void PannerNode::setConeOuterAngle(double value)
{
    AudioGraphLock graphLock(context()->graph());
    m_coneOuterAngle = value;
    m_pannerHandler->setConeOuterAngle(value);
}

void PannerNode::setConeOuterGain(double value)
{
    AudioGraphLock graphLock(context()->graph());
    if (!std::isfinite(value) || value < 0 || value > 1) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "coneOuterGain must be in [0, 1]");
    }
    m_coneOuterGain = value;
    m_pannerHandler->setConeOuterGain(value);
}

void PannerNode::setPosition(double x, double y, double z)
{
    AudioGraphLock graphLock(context()->graph());
    const double limit = std::numeric_limits<float>::max();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "Position exceeds the float range");
    }
    m_positionX->setValue(static_cast<float>(x));
    m_positionY->setValue(static_cast<float>(y));
    m_positionZ->setValue(static_cast<float>(z));
}

void PannerNode::setOrientation(double x, double y, double z)
{
    AudioGraphLock graphLock(context()->graph());
    const double limit = std::numeric_limits<float>::max();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit) {
        throw new DOMException(executionContext(),
                               DOMException::SCRIPT_TYPE_ERR,
                               "Orientation exceeds the float range");
    }
    m_orientationX->setValue(static_cast<float>(x));
    m_orientationY->setValue(static_cast<float>(y));
    m_orientationZ->setValue(static_cast<float>(z));
}

void PannerNode::setChannelCount(uint32_t value)
{
    AudioGraphLock graphLock(context()->graph());
    if (value > 2) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Panner accepts at most two channels");
    }
    AudioNode::setChannelCount(value);
}

void PannerNode::setChannelCountModeStr(String* mode)
{
    AudioGraphLock graphLock(context()->graph());
    if (mode->equals("max")) {
        throw new DOMException(executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "Panner does not support max mode");
    }
    AudioNode::setChannelCountModeStr(mode);
}

} // namespace Starfish
#endif
