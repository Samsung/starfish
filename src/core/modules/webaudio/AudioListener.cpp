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
#include "core/modules/webaudio/AudioListener.h"

#include "core/modules/webaudio/AudioParam.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/dom/DOMException.h"
#include "core/modules/webaudio/render/AudioGraph.h"

#include <cmath>
#include <limits>

namespace Starfish {

AudioListener::AudioListener(ExecutionContext* executionContext,
                             BaseAudioContext* context)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
    , m_context(context)
    , m_positionX(new AudioParam(context, context->graph()->createTimeline(0)))
    , m_positionY(new AudioParam(context, context->graph()->createTimeline(0)))
    , m_positionZ(new AudioParam(context, context->graph()->createTimeline(0)))
    , m_forwardX(new AudioParam(context, context->graph()->createTimeline(0)))
    , m_forwardY(new AudioParam(context, context->graph()->createTimeline(0)))
    , m_forwardZ(new AudioParam(context, context->graph()->createTimeline(-1)))
    , m_upX(new AudioParam(context, context->graph()->createTimeline(0)))
    , m_upY(new AudioParam(context, context->graph()->createTimeline(1)))
    , m_upZ(new AudioParam(context, context->graph()->createTimeline(0)))
{
    AudioGraphLock graphLock(context->graph());
}

ScriptBindingInstance* AudioListener::scriptBindingInstance()
{
    return m_context->scriptBindingInstance();
}

void AudioListener::setPosition(double x, double y, double z)
{
    AudioGraphLock graphLock(m_context->graph());
    // https://webaudio.github.io/web-audio-api/#dom-audiolistener-setposition
    const double limit = std::numeric_limits<float>::max();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit) {
        throw new DOMException(m_executionContext,
                               DOMException::SCRIPT_TYPE_ERR,
                               "Position exceeds the float range");
    }
    m_positionX->setValue(static_cast<float>(x));
    m_positionY->setValue(static_cast<float>(y));
    m_positionZ->setValue(static_cast<float>(z));
}

void AudioListener::setOrientation(double x, double y, double z, double xUp,
                                   double yUp, double zUp)
{
    AudioGraphLock graphLock(m_context->graph());
    // https://webaudio.github.io/web-audio-api/#dom-audiolistener-setorientation
    const double limit = std::numeric_limits<float>::max();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
        !std::isfinite(xUp) || !std::isfinite(yUp) || !std::isfinite(zUp) ||
        std::abs(x) > limit || std::abs(y) > limit || std::abs(z) > limit ||
        std::abs(xUp) > limit || std::abs(yUp) > limit ||
        std::abs(zUp) > limit) {
        throw new DOMException(m_executionContext,
                               DOMException::SCRIPT_TYPE_ERR,
                               "Orientation exceeds the float range");
    }
    m_forwardX->setValue(static_cast<float>(x));
    m_forwardY->setValue(static_cast<float>(y));
    m_forwardZ->setValue(static_cast<float>(z));
    m_upX->setValue(static_cast<float>(xUp));
    m_upY->setValue(static_cast<float>(yUp));
    m_upZ->setValue(static_cast<float>(zUp));
}

} // namespace Starfish
#endif
