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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/webaudio/AudioContext.h"

#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/webaudio/AudioDestinationNode.h"
#include "core/modules/webaudio/render/AudioBus.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/webaudio/MediaElementAudioSourceNode.h"
#include "core/page/WebBase.h"
#include "core/page/Window.h"

#include <algorithm>
#include <cmath>

namespace Starfish {

static double audioContextSampleRate(ExecutionContext* executionContext,
                                     const AudioContextOptions& options)
{
    if (!options.hasSampleRate()) {
        return 48000;
    }
    const double sampleRate = options.sampleRate();
    // https://webaudio.github.io/web-audio-api/#supported-sample-rates
    if (!std::isfinite(sampleRate) || sampleRate < 3000 ||
        sampleRate > 768000) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported AudioContext sample rate");
    }
    return sampleRate;
}

static void validateLatencyHint(ExecutionContext* executionContext,
                                const AudioContextOptions& options)
{
    const auto& hint = options.latencyHint();
    if (hint.isAudioContextLatencyCategoryValue()) {
        String* category = hint.getAudioContextLatencyCategoryValue();
        if (category->equals("interactive") || category->equals("balanced") ||
            category->equals("playback")) {
            return;
        }
    } else if (hint.isdoubleValue() && std::isfinite(hint.getdoubleValue())) {
        // latencyHint is advisory; the current fixed-quantum output ignores it.
        return;
    }
    throw new DOMException(executionContext, DOMException::SCRIPT_TYPE_ERR,
                           "Invalid AudioContext latencyHint");
}

// Carries the promise of a suspend()/resume()/close() call through its
// control message and the media element task that settles it.
struct AudioContextPromiseMessage : public gc {
    AudioContextPromiseMessage(AudioContext* context, Promise* promise)
        : context(context)
        , promise(promise)
    {
    }
    AudioContext* context;
    Promise* promise;
};

// https://webaudio.github.io/web-audio-api/#AudioContext-constructors
AudioContext::AudioContext(ExecutionContext* executionContext,
                           AudioContextOptions contextOptions)
    : BaseAudioContext(executionContext,
                       audioContextSampleRate(executionContext, contextOptions))
{
    // https://webaudio.github.io/web-audio-api/#AudioContext-constructors
    validateLatencyHint(executionContext, contextOptions);
    if (!executionContext->document()->isFullyActive()) {
        throw new DOMException(executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "AudioContext document is not active");
    }
    executionContext->document()->window()->registerAudioContext(this);
    if (isAllowedToStart()) {
        // Send a control message to start processing.
        m_controlQueue->enqueue(
            [](void* data) {
                auto* self = static_cast<AudioContext*>(data);
                // A suspend(), resume() or close() issued before this message
                // runs queues its own control message, which decides the state.
                if (self->m_controlQueue->state() !=
                        AudioContextState::Suspended ||
                    self->m_suspendedByUser) {
                    return;
                }
                if (!self->startRealtime()) {
                    return;
                }
                self->m_controlQueue->setState(AudioContextState::Running);
                self->m_controlQueue->enqueue(
                    [](void* data) {
                        auto* self = static_cast<AudioContext*>(data);
                        self->setStateAttribute(AudioContextState::Running);
                        self->dispatchStateChange();
                    },
                    self);
            },
            this);
    }
}

ScriptBindingInstance* AudioContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

// https://webaudio.github.io/web-audio-api/#dom-audiocontext-baselatency
double AudioContext::baseLatency() const
{
    // The render quantum and the bounded handoff to the host audio API each
    // contribute one quantum of processing delay in the current output path.
    return 2.0 * AudioBus::RenderQuantumFrames / sampleRate();
}

double AudioContext::outputLatency() const
{
    return graph()->outputLatency();
}

// https://webaudio.github.io/web-audio-api/#dom-audiocontext-getoutputtimestamp
AudioTimestamp AudioContext::getOutputTimestamp() const
{
    AudioTimestamp result = m_lastOutputTimestamp;
    double contextTime;
    uint64_t wallClockUs;
    if (graph()->outputTimestamp(contextTime, wallClockUs)) {
        result.setContextTime(contextTime);
        const uint64_t originUs = executionContext()->createdTick();
        result.setPerformanceTime(
            wallClockUs >= originUs ? (wallClockUs - originUs) / 1000.0 : 0);
    }
    return result;
}

MediaElementAudioSourceNode* AudioContext::createMediaElementSource(
    HTMLMediaElement* mediaElement)
{
    MediaElementAudioSourceOptions init;
    init.m_mediaElement = mediaElement;
    MediaElementAudioSourceNode* source =
        new MediaElementAudioSourceNode(executionContext(), this, init);
    return source;
}

bool AudioContext::startRealtime()
{
    if (m_renderingQueue->state() == AudioContextState::Running) {
        return true;
    }
    if (m_renderingQueue->state() == AudioContextState::Closed) {
        return false;
    }
    if (!executionContext()->document()->isFullyActive()) {
        shutdownForNavigation();
        return false;
    }
    if (!graph()->startRealtime(destination()->channelCount())) {
        return false;
    }
    m_renderingQueue->setState(AudioContextState::Running);
    updateRealtimeTimer();
    return true;
}

void AudioContext::stopRealtime()
{
    if (m_renderingQueue->state() != AudioContextState::Running) {
        return;
    }
    m_renderingQueue->setState(AudioContextState::Suspended);
    updateRealtimeTimer();
    m_lastOutputTimestamp = getOutputTimestamp();
    graph()->stopRealtime();
}

void AudioContext::didRegisterScheduledSource()
{
    updateRealtimeTimer();
}

// The timer only delivers source completion (`ended`) to DOM tasks; the audio
// clock and all DSP run on the render thread. It is armed only while there
// is a scheduled source to watch, so an idle context costs no wakeups.
void AudioContext::updateRealtimeTimer()
{
    const bool needed =
        m_renderingQueue->state() == AudioContextState::Running &&
        !m_activeSources.empty();
    const bool armed = m_realtimeTimer != TimerInvalidID;
    if (needed == armed) {
        return;
    }
    Timer* timer = executionContext()->webBase()->timer();
    if (!needed) {
        timer->removeTimer(m_realtimeTimer);
        m_realtimeTimer = TimerInvalidID;
        return;
    }
    m_realtimeTimer = timer->addTimer(
        8, executionContext()->document()->window(),
        [](void* data) { static_cast<AudioContext*>(data)->renderRealtime(); },
        this, true);
}

void AudioContext::renderRealtime()
{
    if (!executionContext()->document()->isFullyActive()) {
        shutdownForNavigation();
        return;
    }
    renderScheduledSources();
    updateRealtimeTimer();
}

bool AudioContext::shutdownForNavigation()
{
    stopRealtime();
    m_controlQueue->setState(AudioContextState::Closed);
    m_renderingQueue->setState(AudioContextState::Closed);
    // https://webaudio.github.io/web-audio-api/#unloading-a-document
    // Reject [[pending promises]]; their settling tasks never run once the
    // Window's queued tasks are cleared.
    if (m_pendingPromises.empty()) {
        return false;
    }
    GCVector<Promise*> pending;
    pending.swap(m_pendingPromises);
    for (Promise* promise : pending) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "AudioContext document is not active");
        promise->reject(error->scriptValue());
    }
    return true;
}

bool AudioContext::takePendingPromise(Promise* promise)
{
    auto it =
        std::find(m_pendingPromises.begin(), m_pendingPromises.end(), promise);
    if (it == m_pendingPromises.end()) {
        return false; // Already rejected by document unload.
    }
    m_pendingPromises.erase(it);
    return true;
}

void AudioContext::resolvePendingPromise(Promise* promise)
{
    if (takePendingPromise(promise)) {
        promise->fulfill(scriptUndefined());
    }
}

void AudioContext::queuePromiseMessage(Promise* promise,
                                       MessageQueueFunction message)
{
    m_pendingPromises.push_back(promise);
    m_controlQueue->enqueue(message,
                            new AudioContextPromiseMessage(this, promise));
}

// https://webaudio.github.io/web-audio-api/#dom-audiocontext-suspend
Promise* AudioContext::suspend()
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (!executionContext()->document()->isFullyActive() ||
        m_controlQueue->state() == AudioContextState::Closed) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "AudioContext cannot be suspended");
        promise->reject(error->scriptValue());
        return promise;
    }
    m_suspendedByUser = true;
    m_controlQueue->setState(AudioContextState::Suspended);
    // Running a control message to suspend an AudioContext.
    queuePromiseMessage(promise, [](void* data) {
        auto* message = static_cast<AudioContextPromiseMessage*>(data);
        message->context->stopRealtime();
        message->context->m_controlQueue->enqueue(
            [](void* data) {
                auto* message = static_cast<AudioContextPromiseMessage*>(data);
                AudioContext* self = message->context;
                self->updateStateAttributeAndQueueStateChange(
                    AudioContextState::Suspended);
                self->resolvePendingPromise(message->promise);
            },
            message);
    });
    return promise;
}

// https://webaudio.github.io/web-audio-api/#dom-audiocontext-resume
Promise* AudioContext::resume()
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (!executionContext()->document()->isFullyActive() ||
        m_controlQueue->state() == AudioContextState::Closed) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "AudioContext cannot be resumed");
        promise->reject(error->scriptValue());
        return promise;
    }
    m_suspendedByUser = false;
    m_controlQueue->setState(AudioContextState::Running);
    // Running a control message to resume an AudioContext.
    queuePromiseMessage(promise, [](void* data) {
        auto* message = static_cast<AudioContextPromiseMessage*>(data);
        AudioContext* self = message->context;
        if (!self->startRealtime()) {
            self->m_controlQueue->enqueue(
                [](void* data) {
                    auto* message =
                        static_cast<AudioContextPromiseMessage*>(data);
                    AudioContext* self = message->context;
                    if (!self->takePendingPromise(message->promise)) {
                        return;
                    }
                    auto* error =
                        new DOMException(self->executionContext(),
                                         DOMException::NOT_SUPPORTED_ERR,
                                         "Unable to start audio rendering");
                    message->promise->reject(error->scriptValue());
                },
                message);
            return;
        }
        self->m_controlQueue->enqueue(
            [](void* data) {
                auto* message = static_cast<AudioContextPromiseMessage*>(data);
                AudioContext* self = message->context;
                self->updateStateAttributeAndQueueStateChange(
                    AudioContextState::Running);
                self->resolvePendingPromise(message->promise);
            },
            message);
    });
    return promise;
}

// https://webaudio.github.io/web-audio-api/#dom-audiocontext-close
Promise* AudioContext::close()
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (!executionContext()->document()->isFullyActive() ||
        m_controlQueue->state() == AudioContextState::Closed) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "AudioContext cannot be closed");
        promise->reject(error->scriptValue());
        return promise;
    }
    m_controlQueue->setState(AudioContextState::Closed);
    // Running a control message to close an AudioContext.
    queuePromiseMessage(promise, [](void* data) {
        auto* message = static_cast<AudioContextPromiseMessage*>(data);
        AudioContext* self = message->context;
        self->stopRealtime();
        self->m_renderingQueue->setState(AudioContextState::Closed);
        self->m_controlQueue->enqueue(
            [](void* data) {
                auto* message = static_cast<AudioContextPromiseMessage*>(data);
                AudioContext* self = message->context;
                self->updateStateAttributeAndQueueStateChange(
                    AudioContextState::Closed);
                self->resolvePendingPromise(message->promise);
                // Every earlier control message has settled by now, so there
                // is nothing left for document unload to shut down.
                self->executionContext()
                    ->document()
                    ->window()
                    ->unregisterAudioContext(self);
            },
            message);
    });
    return promise;
}
} // namespace Starfish

#endif
