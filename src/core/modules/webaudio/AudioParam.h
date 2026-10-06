/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioParam__
#define __StarfishAudioParam__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class BaseAudioContext;
class AudioParamTimeline;
class AudioGraphReleaseQueue;

class AudioParam : public ScriptWrappable {
public:
    AudioParam(BaseAudioContext* context, AudioParamTimeline* timeline);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioParam)

    float value() const;
    void setValue(float value);
    String* automationRateStr() const;
    void setAutomationRateStr(String* rate);
    float defaultValue() const;
    float minValue() const;
    float maxValue() const;
    AudioParam* setValueAtTime(float value, double startTime);
    AudioParam* linearRampToValueAtTime(float value, double endTime);
    AudioParam* exponentialRampToValueAtTime(float value, double endTime);
    AudioParam* setTargetAtTime(float target, double startTime,
                                float timeConstant);
    AudioParam* setValueCurveAtTime(const GCAtomicVector<double>& values,
                                    double startTime, double duration);
    AudioParam* cancelScheduledValues(double cancelTime);
    AudioParam* cancelAndHoldAtTime(double cancelTime);

    BaseAudioContext* context() const
    {
        return m_context;
    }
    AudioParamTimeline* timeline() const
    {
        return m_timeline;
    }

private:
    void ensureEventOutsideValueCurve(double time) const;
    BaseAudioContext* m_context;
    AudioParamTimeline* m_timeline;
    // Reports collection so the graph can free m_timeline and its handler.
    AudioGraphReleaseQueue* m_releaseQueue;
};

} // namespace Starfish
#endif
#endif
