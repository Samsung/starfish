/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAudioContext__
#define __StarfishAudioContext__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/webaudio/BaseAudioContext.h"
#include "core/modules/message_loop/Timer.h"
#include "binding/generated/doubleOrAudioContextLatencyCategoryUnion.h"

namespace Starfish {
class ExecutionContext;
class HTMLMediaElement;
class MediaElementAudioSourceNode;

struct AudioContextOptions {
    DEFINE_GETTER(float, sampleRate)
    DEFINE_GETTER_SETTER(doubleOrAudioContextLatencyCategory, latencyHint,
                         LatencyHint)

    void setSampleRate(float value)
    {
        m_sampleRate = value;
        m_hasSampleRate = true;
    }

    bool hasSampleRate() const
    {
        return m_hasSampleRate;
    }

    float m_sampleRate{ 0 };
    bool m_hasSampleRate{ false };
    doubleOrAudioContextLatencyCategory m_latencyHint =
        doubleOrAudioContextLatencyCategory::createAudioContextLatencyCategory(
            String::createASCIIString("interactive"));
};

struct AudioTimestamp {
    double contextTime() const
    {
        return m_contextTime;
    }
    void setContextTime(double value)
    {
        m_contextTime = value;
    }
    double performanceTime() const
    {
        return m_performanceTime;
    }
    void setPerformanceTime(double value)
    {
        m_performanceTime = value;
    }

private:
    double m_contextTime{ 0 };
    double m_performanceTime{ 0 };
};

class AudioContext : public BaseAudioContext {
public:
    AudioContext(ExecutionContext* executionContext,
                 AudioContextOptions contextOptions = AudioContextOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioContext);

    MediaElementAudioSourceNode* createMediaElementSource(
        HTMLMediaElement* mediaElement);

    Promise* close();
    Promise* suspend();
    Promise* resume();
    // Returns whether pending promises were rejected, so the caller can run
    // their reactions before the realm is torn down.
    bool shutdownForNavigation();
    double baseLatency() const;
    double outputLatency() const;
    AudioTimestamp getOutputTimestamp() const;

protected:
    void didRegisterScheduledSource() override;

private:
    bool startRealtime();
    void stopRealtime();
    void renderRealtime();
    void updateRealtimeTimer();
    void queuePromiseMessage(Promise* promise, MessageQueueFunction message);
    bool takePendingPromise(Promise* promise);
    void resolvePendingPromise(Promise* promise);

    size_t m_realtimeTimer{ TimerInvalidID };
    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-pending-promises-slot
    GCVector<Promise*> m_pendingPromises;
    AudioTimestamp m_lastOutputTimestamp;
};
} // namespace Starfish
#endif
#endif
