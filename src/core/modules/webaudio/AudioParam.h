/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioParam__
#define __StarfishAudioParam__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ExecutionContext;

class AudioParam : public ScriptWrappable {
public:
    AudioParam(ExecutionContext* executionContext, float defaultValue = 1.0f,
               float minValue = -3.4028235e38f, float maxValue = 3.4028235e38f);
    virtual ~AudioParam()
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioParam)

    float value() const
    {
        return m_value;
    }

    void setValue(float v)
    {
        m_value = v;
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

    // https://webaudio.github.io/web-audio-api/#dfn-automation-event
    // There is no automation timeline yet: each scheduling call applies its
    // end value immediately and the time arguments are ignored, and the
    // cancel calls have nothing to cancel. Consumers read value() once, when
    // playback starts, rather than per render quantum.
    AudioParam* setValueAtTime(float value, double startTime)
    {
        m_value = value;
        return this;
    }

    AudioParam* linearRampToValueAtTime(float value, double endTime)
    {
        m_value = value;
        return this;
    }

    AudioParam* exponentialRampToValueAtTime(float value, double endTime)
    {
        m_value = value;
        return this;
    }

    AudioParam* setTargetAtTime(float target, double startTime,
                                double timeConstant)
    {
        m_value = target;
        return this;
    }

    AudioParam* setValueCurveAtTime(const GCAtomicVector<double>& values,
                                    double startTime, double duration)
    {
        if (!values.empty()) {
            m_value = values.back();
        }
        return this;
    }

    AudioParam* cancelScheduledValues(double cancelTime)
    {
        return this;
    }

    AudioParam* cancelAndHoldAtTime(double cancelTime)
    {
        return this;
    }

private:
    ExecutionContext* m_executionContext{ nullptr };
    float m_value{ 1.0f };
    float m_defaultValue{ 1.0f };
    float m_minValue{ -3.4028235e38f };
    float m_maxValue{ 3.4028235e38f };
};

} // namespace Starfish

#endif
#endif
