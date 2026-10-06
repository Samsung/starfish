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
#include "core/modules/webaudio/AudioParam.h"

#include "core/dom/DOMException.h"
#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"

#include <cmath>
#include <algorithm>
#include <limits>

namespace Starfish {

AudioParam::AudioParam(BaseAudioContext* context, AudioParamTimeline* timeline)
    : ScriptWrappable(this)
    , m_context(context)
    , m_timeline(timeline)
    , m_releaseQueue(context->graph()->releaseQueue())
{
    AudioGraphLock graphLock(context->graph());
    // While this wrapper lives, script may automate the timeline, so the
    // graph keeps it (and the handler embedding it) alive.
    m_timeline->addWrapper();
    m_releaseQueue->retain();
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void*) {
            AudioParam* param = static_cast<AudioParam*>(obj);
            param->m_releaseQueue->postTimeline(param->m_timeline);
            param->m_releaseQueue->release();
        },
        NULL, NULL, NULL);
}

ScriptBindingInstance* AudioParam::scriptBindingInstance()
{
    return m_context->scriptBindingInstance();
}

float AudioParam::value() const
{
    AudioGraphLock graphLock(m_context->graph());
    return m_timeline->currentValue();
}

void AudioParam::setValue(float value)
{
    AudioGraphLock graphLock(m_context->graph());
    setValueAtTime(value, m_context->currentTime());
    m_timeline->setCurrentValue(value);
}

String* AudioParam::automationRateStr() const
{
    AudioGraphLock graphLock(m_context->graph());
    return String::createASCIIString(m_timeline->isARate() ? "a-rate"
                                                           : "k-rate");
}

void AudioParam::setAutomationRateStr(String* rate)
{
    AudioGraphLock graphLock(m_context->graph());
    if (m_timeline->isFixedKRate() && rate->equals("a-rate")) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "AudioParam requires k-rate automation");
    }
    m_timeline->setARate(rate->equals("a-rate"));
}

float AudioParam::defaultValue() const
{
    AudioGraphLock graphLock(m_context->graph());
    return m_timeline->defaultValue();
}

float AudioParam::minValue() const
{
    AudioGraphLock graphLock(m_context->graph());
    return m_timeline->minValue();
}

float AudioParam::maxValue() const
{
    AudioGraphLock graphLock(m_context->graph());
    return m_timeline->maxValue();
}

void AudioParam::ensureEventOutsideValueCurve(double time) const
{
    AudioGraphLock graphLock(m_context->graph());
    if (m_timeline->isInsideValueCurve(time)) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "AudioParam event overlaps a value curve");
    }
}

AudioParam* AudioParam::setValueAtTime(float value, double startTime)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(startTime) || startTime < 0 || !std::isfinite(value)) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam event");
    }
    const double time = std::max(startTime, m_context->currentTime());
    ensureEventOutsideValueCurve(time);
    m_timeline->setValue(value, time);
    return this;
}

AudioParam* AudioParam::linearRampToValueAtTime(float value, double endTime)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(endTime) || endTime < 0 || !std::isfinite(value)) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam ramp");
    }
    const double currentTime = m_context->currentTime();
    const double time = std::max(endTime, currentTime);
    ensureEventOutsideValueCurve(time);
    m_timeline->linearRamp(value, time, currentTime);
    return this;
}

AudioParam* AudioParam::exponentialRampToValueAtTime(float value,
                                                     double endTime)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(endTime) || endTime < 0 || !std::isfinite(value) ||
        value == 0) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam exponential ramp");
    }
    const double currentTime = m_context->currentTime();
    const double time = std::max(endTime, currentTime);
    ensureEventOutsideValueCurve(time);
    m_timeline->exponentialRamp(value, time, currentTime);
    return this;
}

AudioParam* AudioParam::setTargetAtTime(float target, double startTime,
                                        float timeConstant)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(startTime) || startTime < 0 ||
        !std::isfinite(timeConstant) || timeConstant < 0 ||
        !std::isfinite(target)) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam target event");
    }
    const double time = std::max(startTime, m_context->currentTime());
    ensureEventOutsideValueCurve(time);
    m_timeline->setTarget(target, time, timeConstant);
    return this;
}

AudioParam* AudioParam::setValueCurveAtTime(
    const GCAtomicVector<double>& values, double startTime, double duration)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(startTime) || startTime < 0 ||
        !std::isfinite(duration) || duration <= 0) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam value curve time");
    }
    if (values.size() < 2) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "AudioParam value curve requires two values");
    }
    const double time = std::max(startTime, m_context->currentTime());
    std::vector<float> copy;
    copy.reserve(values.size());
    for (double value : values) {
        if (!std::isfinite(value) ||
            std::abs(value) > std::numeric_limits<float>::max()) {
            throw new DOMException(m_context->executionContext(),
                                   DOMException::SCRIPT_TYPE_ERR,
                                   "Invalid AudioParam value curve sample");
        }
        copy.push_back(static_cast<float>(value));
    }
    if (!std::isfinite(time + duration) ||
        m_timeline->valueCurveConflicts(time, duration)) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::NOT_SUPPORTED_ERR,
                               "AudioParam value curve overlaps an event");
    }
    m_timeline->setValueCurve(std::move(copy), time, duration);
    return this;
}

AudioParam* AudioParam::cancelScheduledValues(double cancelTime)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(cancelTime) || cancelTime < 0) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam cancellation time");
    }
    m_timeline->cancelScheduledValues(
        std::max(cancelTime, m_context->currentTime()));
    return this;
}

AudioParam* AudioParam::cancelAndHoldAtTime(double cancelTime)
{
    AudioGraphLock graphLock(m_context->graph());
    if (!std::isfinite(cancelTime) || cancelTime < 0) {
        throw new DOMException(m_context->executionContext(),
                               DOMException::SCRIPT_RANGE_ERR,
                               "Invalid AudioParam hold time");
    }
    m_timeline->cancelAndHoldAtTime(
        std::max(cancelTime, m_context->currentTime()));
    return this;
}

} // namespace Starfish
#endif
