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
#include "core/modules/webaudio/OfflineAudioContext.h"

#include "core/dom/DOMException.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "binding/ScriptBindingInstance.h"
#include "binding/ScriptEngineInstance.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/AudioDestinationNode.h"
#include "core/modules/webaudio/OfflineAudioCompletionEvent.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/page/WebBase.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace Starfish {

static double validatedSampleRate(ExecutionContext* executionContext,
                                  double sampleRate)
{
    if (!executionContext->document()->isFullyActive()) {
        throw new DOMException(executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "Offline context document is not active");
    }
    if (!std::isfinite(sampleRate) || sampleRate < 3000 ||
        sampleRate > 768000) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported offline sample rate");
    }
    return sampleRate;
}

// WebIDL converts the options (an invalid hint string is a TypeError) before
// the ED's constructor steps check the document, the sample rate and then the
// hint's value.
static double validatedOptionsSampleRate(
    ExecutionContext* executionContext,
    const OfflineAudioContextOptions& options)
{
    throwIfInvalidRenderSizeCategory(executionContext,
                                     options.renderSizeHint());
    return validatedSampleRate(executionContext, options.sampleRate());
}

OfflineAudioContext::OfflineAudioContext(ExecutionContext* executionContext,
                                         OfflineAudioContextOptions options)
    : OfflineAudioContext(
          executionContext, options.numberOfChannels(), options.length(),
          options.sampleRate(),
          renderQuantumSizeFromHint(
              executionContext, options.renderSizeHint(),
              validatedOptionsSampleRate(executionContext, options)))
{
}

OfflineAudioContext::OfflineAudioContext(ExecutionContext* executionContext,
                                         uint32_t numberOfChannels,
                                         uint32_t length, double sampleRate,
                                         size_t renderQuantumSize)
    : BaseAudioContext(executionContext,
                       validatedSampleRate(executionContext, sampleRate),
                       renderQuantumSize)
    , m_length(length)
    , m_numberOfChannels(numberOfChannels)
{
    if (!numberOfChannels || numberOfChannels > 32 || !length) {
        throw new DOMException(executionContext,
                               DOMException::NOT_SUPPORTED_ERR,
                               "Unsupported offline audio dimensions");
    }
    destination()->configureOfflineChannelCount(numberOfChannels);
#if defined(STARFISH_ENABLE_AUDIO_WORKLET)
    // renderChunk() renders on the main thread, between tasks.
    graph()->setMainThreadRendering(true);
#endif
}

ScriptBindingInstance* OfflineAudioContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

DEFINE_EVENT_LISTENER(OfflineAudioContext, complete);

void OfflineAudioContext::queueRenderChunk()
{
    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->globalScope(),
        [](size_t, void* data) {
            static_cast<OfflineAudioContext*>(data)->renderChunk();
        },
        this);
}

Promise* OfflineAudioContext::startRendering()
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (!executionContext()->document()->isFullyActive() || m_started) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "Offline rendering cannot be started");
        promise->reject(error->scriptValue());
        return promise;
    }

    AudioBufferData* data =
        AudioBufferData::create(m_numberOfChannels, m_length);
    if (!data) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::NOT_SUPPORTED_ERR,
                                       "Unable to allocate rendered audio");
        promise->reject(error->scriptValue());
        return promise;
    }
    try {
        graph()->prepareOutput(m_numberOfChannels);
    } catch (const std::bad_alloc&) {
        // See BaseAudioContext::throwAllocationError().
        data->release();
        auto* error = new DOMException(
            executionContext(), DOMException::NOT_SUPPORTED_ERR,
            "Not enough memory for this render quantum size");
        promise->reject(error->scriptValue());
        return promise;
    }
    m_renderedBuffer = new AudioBuffer(executionContext(), data, sampleRate());
    m_renderPromise = promise;
    m_started = true;
    m_controlQueue->setState(AudioContextState::Running);
    m_renderingQueue->setState(AudioContextState::Running);
    // startRendering() itself does not define a state change, but rendering
    // makes the context "running" (see the state attribute and
    // AudioContextState), which is announced like any other transition.
    m_controlQueue->enqueue(
        [](void* data) {
            static_cast<OfflineAudioContext*>(data)
                ->updateStateAttributeAndQueueStateChange(
                    AudioContextState::Running);
        },
        this);
    queueRenderChunk();
    return promise;
}

// https://webaudio.github.io/web-audio-api/#dom-offlineaudiocontext-suspend
Promise* OfflineAudioContext::suspend(double suspendTime)
{
    Promise* promise = new Promise(scriptBindingInstance());
    const double maxTime = static_cast<double>(m_length) / sampleRate();
    if (!executionContext()->document()->isFullyActive() ||
        m_controlQueue->state() == AudioContextState::Closed ||
        !std::isfinite(suspendTime) || suspendTime <= currentTime() ||
        suspendTime >= maxTime) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "Invalid offline suspend time");
        promise->reject(error->scriptValue());
        return promise;
    }

    // The first render quantum boundary at or after suspendTime, compared
    // as times like currentTime, so n * quantum / sampleRate maps to
    // boundary n despite rounding in suspendTime * sampleRate.
    const uint64_t quantumSize = graph()->quantumFrames();
    uint64_t quanta = static_cast<uint64_t>(
        std::ceil(suspendTime * sampleRate() / quantumSize));
    if (quanta &&
        static_cast<double>((quanta - 1) * quantumSize) / sampleRate() >=
            suspendTime) {
        quanta--;
    }
    const uint64_t frame = quanta * quantumSize;
    if (frame >= m_length ||
        std::any_of(m_pendingSuspends.begin(), m_pendingSuspends.end(),
                    [frame](const PendingSuspend& pending) {
                        return pending.frame == frame;
                    })) {
        auto* error = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Duplicate or out-of-range suspend frame");
        promise->reject(error->scriptValue());
        return promise;
    }

    m_pendingSuspends.push_back({ frame, promise });
    std::sort(m_pendingSuspends.begin(), m_pendingSuspends.end(),
              [](const PendingSuspend& a, const PendingSuspend& b) {
                  return a.frame < b.frame;
              });
    return promise;
}

// https://webaudio.github.io/web-audio-api/#dom-offlineaudiocontext-resume
Promise* OfflineAudioContext::resume()
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (!executionContext()->document()->isFullyActive() || !m_started ||
        m_controlQueue->state() == AudioContextState::Closed) {
        auto* error = new DOMException(executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "Offline rendering has not started");
        promise->reject(error->scriptValue());
        return promise;
    }
    m_controlQueue->setState(AudioContextState::Running);
    struct ResumeMessage : public gc {
        OfflineAudioContext* self;
        Promise* promise;
    };
    auto* message = new ResumeMessage;
    message->self = this;
    message->promise = promise;
    // Running a control message to resume an OfflineAudioContext: rendering
    // continues, then a media element task resolves the promise and updates
    // the state attribute.
    m_controlQueue->enqueue(
        [](void* data) {
            auto* message = static_cast<ResumeMessage*>(data);
            OfflineAudioContext* self = message->self;
            if (self->m_renderingQueue->state() ==
                AudioContextState::Suspended) {
                self->m_renderingQueue->setState(AudioContextState::Running);
                self->queueRenderChunk();
            }
            self->m_controlQueue->enqueue(
                [](void* data) {
                    auto* message = static_cast<ResumeMessage*>(data);
                    message->self->updateStateAttributeAndQueueStateChange(
                        AudioContextState::Running);
                    message->promise->fulfill(scriptUndefined());
                },
                message);
        },
        message);
    return promise;
}

void OfflineAudioContext::renderChunk()
{
    if (!executionContext()->document()->isFullyActive()) {
        auto* error = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Offline context document is not active");
        m_renderPromise->reject(error->scriptValue());
        m_renderPromise = nullptr;
        m_renderedBuffer = nullptr;
        for (const PendingSuspend& pending : m_pendingSuspends) {
            pending.promise->reject(error->scriptValue());
        }
        m_pendingSuspends.clear();
        m_controlQueue->setState(AudioContextState::Closed);
        m_renderingQueue->setState(AudioContextState::Closed);
        return;
    }
    if (m_renderingQueue->state() != AudioContextState::Running) {
        return;
    }

    // Yield after about 8192 frames (at least one quantum) so long offline
    // renders leave room for document tasks and event dispatch.
    const size_t quantumFrames = graph()->quantumFrames();
    for (size_t chunkFrames = 0;
         chunkFrames < 8192 && m_renderedFrames < m_length;
         chunkFrames += quantumFrames) {
        if (!m_pendingSuspends.empty() &&
            m_pendingSuspends.front().frame <= graph()->renderedFrames()) {
            // This chunk is itself a queued task: suspend rendering, resolve
            // the promise, then update the state attribute and queue the
            // statechange task, in the order the suspend/resume algorithms
            // use.
            Promise* promise = m_pendingSuspends.front().promise;
            m_pendingSuspends.erase(m_pendingSuspends.begin());
            m_renderingQueue->setState(AudioContextState::Suspended);
            m_controlQueue->setState(AudioContextState::Suspended);
            updateStateAttributeAndQueueStateChange(
                AudioContextState::Suspended);
            promise->fulfill(scriptUndefined());
            return;
        }
        size_t frames =
            std::min<size_t>(quantumFrames, m_length - m_renderedFrames);
#if defined(STARFISH_ENABLE_AUDIO_WORKLET)
        const AudioBus* rendered;
        {
            // https://webaudio.github.io/web-audio-api/#rendering-loop
            // "Perform a microtask checkpoint" after each render quantum:
            // promise jobs of AudioWorkletProcessors run between quanta,
            // never between two process() calls of one quantum.
            MicroTaskExecutionManager microtasks(
                scriptBindingInstance()->engineInstance());
            rendered = &graph()->renderQuantum(frames);
        }
        const AudioBus& output = *rendered;
#else
        const AudioBus& output = graph()->renderQuantum(frames);
#endif
        for (size_t channel = 0; channel < m_numberOfChannels; channel++) {
            memcpy(m_renderedBuffer->data()->channel(channel) +
                       m_renderedFrames,
                   output.channel(channel), frames * sizeof(float));
        }
        m_renderedFrames += frames;
        renderScheduledSources();
        if (!m_scriptProcessors.empty()) {
            // An offline context waits for script: each event runs before
            // the next quantum renders. The handler may detach the document.
            dispatchScriptProcessorEvents();
            if (!executionContext()->document()->isFullyActive()) {
                break;
            }
        }
    }

    if (m_renderedFrames < m_length) {
        queueRenderChunk();
        return;
    }

    // https://webaudio.github.io/web-audio-api/#begin-offline-rendering
    // Once rendering is complete, a media element task resolves the
    // startRendering() promise, closes the context, and queues a separate
    // task to fire `complete`. The rendering thread is already stopped here,
    // so the close control message's own task (set the state attribute,
    // queue statechange) runs inside this task: script awaiting the promise
    // then observes "closed", and statechange still follows `complete`.
    m_renderingQueue->setState(AudioContextState::Closed);
    m_controlQueue->enqueue(
        [](void* data) {
            auto* self = static_cast<OfflineAudioContext*>(data);
            AudioBuffer* buffer = self->m_renderedBuffer;
            self->m_controlQueue->setState(AudioContextState::Closed);
            const bool stateChanged =
                self->m_stateAttribute != AudioContextState::Closed;
            self->setStateAttribute(AudioContextState::Closed);
            self->m_renderPromise->fulfill(buffer->scriptValue());
            self->m_renderPromise = nullptr;
            self->m_controlQueue->enqueue(
                [](void* data) {
                    auto* self = static_cast<OfflineAudioContext*>(data);
                    OfflineAudioCompletionEventInit init;
                    init.setRenderedBuffer(self->m_renderedBuffer);
                    auto* event = new OfflineAudioCompletionEvent(
                        self->executionContext(),
                        self->staticStrings()->m_complete.localName(), init);
                    self->m_renderedBuffer = nullptr;
                    self->dispatchEventByUA(event);
                },
                self);
            if (stateChanged) {
                self->m_controlQueue->enqueue(
                    [](void* data) {
                        static_cast<OfflineAudioContext*>(data)
                            ->dispatchStateChange();
                    },
                    self);
            }
        },
        this);
}

} // namespace Starfish

#endif
