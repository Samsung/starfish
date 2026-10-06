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
#include "core/modules/webaudio/render/AudioParamTimeline.h"
#include "core/modules/webaudio/render/AudioGraph.h"

#include <algorithm>
#include <cmath>

namespace Starfish {

AudioParamTimeline::AudioParamTimeline(float initialValue, float minValue,
                                       float maxValue)
    : m_defaultValue(initialValue)
    , m_initialValue(initialValue)
    , m_minValue(minValue)
    , m_maxValue(maxValue)
    , m_currentValue(initialValue)
{
}

AudioParamTimeline::~AudioParamTimeline() = default;

void AudioParamTimeline::connect(AudioNodeOutput* output)
{
    if (std::find(m_sources.begin(), m_sources.end(), output) !=
        m_sources.end()) {
        return;
    }
    m_sources.push_back(output);
    if (!m_modulationBus) {
        m_modulationBus.reset(new AudioBus(1));
    }
    // Param inputs are graph edges for ordering and cycle detection.
    output->owner()->graph()->markTopologyDirty();
}

void AudioParamTimeline::disconnect(AudioNodeOutput* output)
{
    auto position = std::find(m_sources.begin(), m_sources.end(), output);
    if (position == m_sources.end()) {
        return;
    }
    m_sources.erase(position);
    if (m_sources.empty()) {
        m_modulationBus.reset();
    }
    output->owner()->graph()->markTopologyDirty();
}

const AudioBus* AudioParamTimeline::modulation(size_t frames)
{
    if (!m_modulationBus) {
        return nullptr;
    }
    m_modulationBus->zero();
    // https://webaudio.github.io/web-audio-api/#dom-audionode-connect-destinationparam-output
    // Param inputs are converted to mono before they are added to the
    // intrinsic automation value. The graph renders every source before the
    // node owning this param, so their outputs hold this quantum.
    for (AudioNodeOutput* source : m_sources) {
        m_modulationBus->addFrom(source->bus(), frames, true);
    }
    return m_modulationBus.get();
}

void AudioParamTimeline::insert(Event event)
{
    auto position =
        std::upper_bound(m_events.begin(), m_events.end(), event.time,
                         [](double time, const Event& existing) {
                             return time < existing.time;
                         });
    m_events.insert(position, event);
}

void AudioParamTimeline::setValue(float value, double time)
{
    insert({ EventType::SetValue, time, value });
}

void AudioParamTimeline::linearRamp(float value, double endTime,
                                    double currentTime)
{
    // https://webaudio.github.io/web-audio-api/#dom-audioparam-linearramptovalueattime
    // Folded history in the anchor already provides the ramp's start.
    if (m_events.empty() && !m_hasAnchor) {
        setValue(m_initialValue, currentTime);
    }
    insert({ EventType::LinearRamp, endTime, value, 0, currentTime });
}

void AudioParamTimeline::exponentialRamp(float value, double endTime,
                                         double currentTime)
{
    // Folded history in the anchor already provides the ramp's start.
    if (m_events.empty() && !m_hasAnchor) {
        setValue(m_initialValue, currentTime);
    }
    insert({ EventType::ExponentialRamp, endTime, value, 0, currentTime });
}

void AudioParamTimeline::setTarget(float target, double startTime,
                                   float timeConstant)
{
    insert({ EventType::SetTarget, startTime, target, timeConstant });
}

void AudioParamTimeline::setValueCurve(std::vector<float>&& values,
                                       double startTime, double duration)
{
    auto curve = std::make_shared<CurveData>();
    curve->values = std::move(values);
    curve->duration = duration;
    insert({ EventType::SetValueCurve, startTime, curve->values.front(), 0, 0,
             curve });
}

bool AudioParamTimeline::isInsideValueCurve(double time) const
{
    if (m_hasAnchor && m_anchor.type == EventType::SetValueCurve &&
        time >= m_anchor.time &&
        time < m_anchor.time + m_anchor.curve->duration) {
        return true;
    }
    for (const Event& event : m_events) {
        if (event.type == EventType::SetValueCurve && time >= event.time &&
            time < event.time + event.curve->duration) {
            return true;
        }
    }
    return false;
}

bool AudioParamTimeline::valueCurveConflicts(double startTime,
                                             double duration) const
{
    if (isInsideValueCurve(startTime)) {
        return true;
    }
    const double endTime = startTime + duration;
    for (const Event& event : m_events) {
        if (event.time > startTime && event.time < endTime) {
            return true;
        }
    }
    return false;
}

void AudioParamTimeline::cancelScheduledValues(double cancelTime)
{
    auto first = std::lower_bound(
        m_events.begin(), m_events.end(), cancelTime,
        [](const Event& event, double time) { return event.time < time; });
    // Active target and curve automations are cancelled with future events.
    // https://webaudio.github.io/web-audio-api/#dom-audioparam-cancelscheduledvalues
    // beginQuantum() never folds the latest past event into the anchor, so
    // the automation active at cancelTime is still an event here.
    if (first != m_events.begin()) {
        const Event& previous = *(first - 1);
        if (previous.type == EventType::SetTarget ||
            (previous.type == EventType::SetValueCurve &&
             cancelTime < previous.time + previous.curve->duration)) {
            --first;
        }
    }
    m_events.erase(first, m_events.end());
}

void AudioParamTimeline::cancelAndHoldAtTime(double cancelTime)
{
    // https://webaudio.github.io/web-audio-api/#dom-audioparam-cancelandholdattime
    // A curve starting at the cancellation instant has zero retained
    // duration, so it must not replace the value from the preceding event.
    auto firstAt = std::lower_bound(
        m_events.begin(), m_events.end(), cancelTime,
        [](const Event& event, double time) { return event.time < time; });
    if (firstAt != m_events.end() && firstAt->time == cancelTime &&
        firstAt->type == EventType::SetValueCurve) {
        m_events.erase(firstAt);
    }
    const float heldValue = valueAt(cancelTime);
    auto firstAfter = std::upper_bound(
        m_events.begin(), m_events.end(), cancelTime,
        [](double time, const Event& event) { return time < event.time; });
    const EventType nextType =
        firstAfter == m_events.end() ? EventType::SetValue : firstAfter->type;
    m_events.erase(firstAfter, m_events.end());
    if (nextType == EventType::LinearRamp ||
        nextType == EventType::ExponentialRamp) {
        insert({ nextType, cancelTime, heldValue });
    } else {
        setValue(heldValue, cancelTime);
    }
}

float AudioParamTimeline::targetValueAt(const State& state, double time)
{
    if (!state.timeConstant) {
        return state.target;
    }
    return static_cast<float>(
        state.target + (state.value - state.target) *
                           std::exp(-(time - state.time) / state.timeConstant));
}

float AudioParamTimeline::curveValueAt(const State& state, double time)
{
    const std::vector<float>& values = state.curve->values;
    const double position =
        std::min(1.0,
                 std::max(0.0, (time - state.time) / state.curve->duration)) *
        static_cast<double>(values.size() - 1);
    const size_t index = static_cast<size_t>(position);
    if (index >= values.size() - 1) {
        return values.back();
    }
    return static_cast<float>(values[index] +
                              (values[index + 1] - values[index]) *
                                  (position - index));
}

// Applies one event whose time has been reached to the automation state.
void AudioParamTimeline::fold(State& state, const Event& event)
{
    if (event.type == EventType::SetTarget) {
        if (state.type == EventType::SetTarget) {
            state.value = targetValueAt(state, event.time);
        } else if (state.type == EventType::SetValueCurve) {
            state.value = curveValueAt(state, event.time);
        }
        state.target = event.value;
        state.timeConstant = event.timeConstant;
    } else if (event.type == EventType::SetValueCurve) {
        state.curve = event.curve.get();
    } else {
        state.value = event.value;
    }
    state.type = event.type;
    state.time = event.time;
}

void AudioParamTimeline::beginQuantum(double time)
{
    if (m_events.empty()) {
        return;
    }
    // Keep the latest event at or before |time|: it is the automation that
    // is active now, which cancelScheduledValues() may still remove. Every
    // earlier event only contributes the state it hands over, so fold those
    // into the anchor. Later queries are never earlier than |time|.
    size_t reached = 0;
    while (reached < m_events.size() && m_events[reached].time <= time) {
        reached++;
    }
    if (reached > 1) {
        if (!m_hasAnchor) {
            m_anchor = {
                0, m_initialValue, 0, 0, EventType::SetValue, nullptr
            };
            m_hasAnchor = true;
        }
        for (size_t i = 0; i + 1 < reached; i++) {
            fold(m_anchor, m_events[i]);
            if (m_events[i].type == EventType::SetValueCurve) {
                m_anchorCurve = m_events[i].curve;
            }
        }
        if (m_anchor.type != EventType::SetValueCurve) {
            m_anchorCurve.reset();
            m_anchor.curve = nullptr;
        }
        m_events.erase(m_events.begin(), m_events.begin() + (reached - 1));
    }
    m_currentValue.store(valueAt(time), std::memory_order_relaxed);
}

float AudioParamTimeline::valueAt(double time) const
{
    State state =
        m_hasAnchor ? m_anchor : State{ 0, m_initialValue,      0,
                                        0, EventType::SetValue, nullptr };
    for (const Event& event : m_events) {
        if (time < event.time) {
            if ((event.type == EventType::LinearRamp ||
                 event.type == EventType::ExponentialRamp) &&
                event.time > state.time && time >= state.time) {
                double startTime = state.time;
                float startValue = state.value;
                if (state.type == EventType::SetTarget) {
                    startTime = std::max(state.time, event.scheduledAt);
                    if (time < startTime) {
                        return targetValueAt(state, time);
                    }
                    startValue = targetValueAt(state, startTime);
                } else if (state.type == EventType::SetValueCurve) {
                    startTime = state.time + state.curve->duration;
                    if (time < startTime) {
                        return curveValueAt(state, time);
                    }
                    startValue = state.curve->values.back();
                }
                double fraction = (time - startTime) / (event.time - startTime);
                if (event.type == EventType::ExponentialRamp) {
                    if (startValue == 0 ||
                        static_cast<double>(startValue) * event.value < 0) {
                        return startValue;
                    }
                    return static_cast<float>(
                        startValue *
                        std::pow(static_cast<double>(event.value) / startValue,
                                 fraction));
                }
                return static_cast<float>(
                    startValue + (event.value - startValue) * fraction);
            }
            break;
        }
        fold(state, event);
    }
    if (state.type == EventType::SetTarget) {
        return targetValueAt(state, time);
    }
    return state.type == EventType::SetValueCurve ? curveValueAt(state, time)
                                                  : state.value;
}

// https://webaudio.github.io/web-audio-api/#computation-of-value
float AudioParamTimeline::computedValueAt(double time,
                                          float modulationValue) const
{
    double value = static_cast<double>(valueAt(time)) + modulationValue;
    if (std::isnan(value)) {
        value = m_defaultValue;
    }
    return static_cast<float>(
        std::max(static_cast<double>(m_minValue),
                 std::min(static_cast<double>(m_maxValue), value)));
}

} // namespace Starfish
#endif
