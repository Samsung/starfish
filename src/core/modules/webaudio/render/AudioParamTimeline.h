/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioParamTimeline__
#define __StarfishAudioParamTimeline__

#include <vector>
#include <memory>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <atomic>

namespace Starfish {
class AudioBus;
class AudioNodeOutput;

// Native-only timeline. Control-side inserts events between render quanta.
class AudioParamTimeline {
public:
    explicit AudioParamTimeline(
        float initialValue, float minValue = -std::numeric_limits<float>::max(),
        float maxValue = std::numeric_limits<float>::max());

    void setValue(float value, double time);
    void setInitialValue(float value)
    {
        m_initialValue = value;
        setCurrentValue(value);
    }
    void linearRamp(float value, double endTime, double currentTime);
    void exponentialRamp(float value, double endTime, double currentTime);
    void setTarget(float target, double startTime, float timeConstant);
    void setValueCurve(std::vector<float>&& values, double startTime,
                       double duration);
    bool isInsideValueCurve(double time) const;
    bool valueCurveConflicts(double startTime, double duration) const;
    void cancelScheduledValues(double cancelTime);
    void cancelAndHoldAtTime(double cancelTime);
    float valueAt(double time) const;
    float computedValueAt(double time, float modulationValue) const;
    // Render quanta start at non-decreasing times; events that can no longer
    // affect any later query are folded into a single anchor state here.
    void beginQuantum(double time);
    void setCurrentValue(float value)
    {
        m_currentValue.store(value, std::memory_order_relaxed);
    }
    float currentValue() const
    {
        return m_currentValue.load(std::memory_order_relaxed);
    }
    ~AudioParamTimeline();
    void connect(AudioNodeOutput* output);
    void disconnect(AudioNodeOutput* output);
    // Graph teardown only: forget sources without touching their outputs.
    void clearSourcesForTeardown()
    {
        m_sources.clear();
    }
    const std::vector<AudioNodeOutput*>& sources() const
    {
        return m_sources;
    }
    // Sums the already-rendered source outputs of this render quantum.
    const AudioBus* modulation(size_t frames);
    void setARate(bool isARate)
    {
        m_isARate = isARate;
    }
    void setFixedKRate()
    {
        m_isARate = false;
        m_fixedKRate = true;
    }
    bool isFixedKRate() const
    {
        return m_fixedKRate;
    }
    bool isARate() const
    {
        return m_isARate;
    }
    float defaultValue() const
    {
        return m_defaultValue;
    }
    float minValue() const
    {
        return m_minValue;
    }
    float maxValue() const
    {
        return m_maxValue;
    }

    // Created by AudioGraph::createTimeline() rather than embedded in a
    // handler; such a timeline may be shared (AudioListener) and is freed by
    // the graph once no wrapper and no handler refers to it.
    bool isGraphOwned() const
    {
        return m_graphOwned;
    }
    void setGraphOwned()
    {
        m_graphOwned = true;
    }
    // Live AudioParam wrappers; changed under the graph lock. A timeline
    // with a wrapper must outlive its handler, since script can still use
    // the AudioParam after dropping the node.
    void addWrapper()
    {
        m_wrappers++;
        m_wrapped = true;
    }
    void releaseWrapper()
    {
        m_wrappers--;
    }
    bool hasWrappers() const
    {
        return m_wrappers;
    }
    bool wasWrapped() const
    {
        return m_wrapped;
    }

private:
    enum class EventType : uint8_t {
        SetValue,
        LinearRamp,
        ExponentialRamp,
        SetTarget,
        SetValueCurve
    };
    struct CurveData {
        std::vector<float> values;
        double duration;
    };
    struct Event {
        Event(EventType eventType, double eventTime, float eventValue,
              float eventTimeConstant = 0, double eventScheduledAt = 0,
              std::shared_ptr<CurveData> eventCurve = nullptr)
            : type(eventType)
            , time(eventTime)
            , value(eventValue)
            , timeConstant(eventTimeConstant)
            , scheduledAt(eventScheduledAt)
            , curve(std::move(eventCurve))
        {
        }
        EventType type;
        double time;
        float value;
        float timeConstant;
        double scheduledAt;
        std::shared_ptr<CurveData> curve;
    };
    // The automation state after a prefix of the event list.
    struct State {
        double time;
        float value;
        float target;
        float timeConstant;
        EventType type;
        const CurveData* curve;
    };
    static float targetValueAt(const State& state, double time);
    static float curveValueAt(const State& state, double time);
    static void fold(State& state, const Event& event);
    void insert(Event event);

    float m_defaultValue;
    float m_initialValue;
    float m_minValue;
    float m_maxValue;
    std::atomic<float> m_currentValue;
    bool m_isARate{ true };
    bool m_fixedKRate{ false };
    bool m_hasAnchor{ false };
    bool m_graphOwned{ false };
    bool m_wrapped{ false };
    unsigned m_wrappers{ 0 };
    State m_anchor;
    std::shared_ptr<CurveData> m_anchorCurve;
    std::vector<Event> m_events;
    std::vector<AudioNodeOutput*> m_sources;
    std::unique_ptr<AudioBus> m_modulationBus;
};

} // namespace Starfish

#endif
#endif
