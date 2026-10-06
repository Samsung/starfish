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

#include "core/modules/webaudio/BaseAudioContext.h"

#include "binding/ScriptBindingInstance.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/page/Window.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"
#include "core/modules/webaudio/AudioScheduledSourceNode.h"
#include "core/modules/webaudio/ConstantSourceNode.h"
#include "core/modules/webaudio/ChannelMergerNode.h"
#include "core/modules/webaudio/ChannelSplitterNode.h"
#include "core/modules/webaudio/StereoPannerNode.h"
#include "core/modules/webaudio/PannerNode.h"
#include "core/modules/webaudio/ConvolverNode.h"
#include "core/modules/webaudio/OscillatorNode.h"
#include "core/modules/webaudio/DelayNode.h"
#include "core/modules/webaudio/IIRFilterNode.h"
#include "core/modules/webaudio/BiquadFilterNode.h"
#include "core/modules/webaudio/WaveShaperNode.h"
#include "core/modules/webaudio/AnalyserNode.h"
#include "core/modules/webaudio/DynamicsCompressorNode.h"
#include "core/modules/webaudio/PeriodicWave.h"
#include "core/modules/webaudio/AudioDestinationNode.h"
#include "core/modules/webaudio/AudioListener.h"
#include "core/modules/webaudio/GainNode.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/page/WebBase.h"
#include "core/page/WebView.h"

#include <atomic>

#include <EscargotPublic.h>

using Escargot::BackingStoreRef;
using Escargot::OptionalRef;

namespace Starfish {

WebAudioMessageQueue::WebAudioMessageQueue(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
    , m_state(AudioContextState::Suspended)
{
}

void WebAudioMessageQueue::enqueue(MessageQueueFunction fn, void* data)
{
    m_executionContext->webBase()->messageLoop()->addIdler(
        m_executionContext->globalScope(),
        [](size_t, void* func, void* data) {
            MessageQueueFunction fn = (MessageQueueFunction)func;
            fn(data);
        },
        (void*)fn, data);
}

ControlMessageQueue::ControlMessageQueue(ExecutionContext* executionContext)
    : WebAudioMessageQueue(executionContext)
{
}

RenderingMessageQueue::RenderingMessageQueue(ExecutionContext* executionContext)
    : WebAudioMessageQueue(executionContext)
{
}

void DecodeSuccessCallback::call(ScriptBindingInstance* instance,
                                 AudioBuffer* decodedData)
{
    ScriptValue args = decodedData->scriptValue();
    callScriptFunction(instance, m_callback, &args, 1, scriptUndefined());
}

void DecodeErrorCallback::call(ScriptBindingInstance* instance,
                               DOMException* exception)
{
    ScriptValue args = exception->scriptValue();
    callScriptFunction(instance, m_callback, &args, 1, scriptUndefined());
}

BaseAudioContext::BaseAudioContext(ExecutionContext* executionContext,
                                   double sampleRate)
    : EventTarget()
    , m_executionContext(executionContext)
    , m_sampleRate(sampleRate)
    , m_controlQueue(new ControlMessageQueue(executionContext))
    , m_renderingQueue(new RenderingMessageQueue(executionContext))
{
    m_graph = new AudioGraph(sampleRate);
    m_destination = new AudioDestinationNode(executionContext, this);
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void*) {
            delete static_cast<BaseAudioContext*>(obj)->m_graph;
        },
        NULL, NULL, NULL);
}

BaseAudioContext::~BaseAudioContext()
{
    // A derived constructor may throw after the GC finalizer is registered.
    // In that case C++ destroys the partially constructed object itself, so
    // cancel the finalizer before its GC allocation can be reclaimed.
    GC_REGISTER_FINALIZER_NO_ORDER(this, NULL, NULL, NULL, NULL);
    delete m_graph;
}

ScriptBindingInstance* BaseAudioContext::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

AudioDestinationNode* BaseAudioContext::destination()
{
    return m_destination;
}

AudioListener* BaseAudioContext::listener()
{
    AudioGraphLock lock(graph());
    if (!m_listener) {
        m_listener = new AudioListener(m_executionContext, this);
    }
    return m_listener.value();
}

double BaseAudioContext::currentTime() const
{
    return m_graph->currentTime();
}

void BaseAudioContext::registerScheduledSource(AudioScheduledSourceNode* source)
{
    AudioGraphLock lock(graph());
    m_activeSources.push_back(source);
    graph()->registerScheduledHandler(source->handler());
    didRegisterScheduledSource();
}

void BaseAudioContext::renderScheduledSources()
{
    GCVector<AudioScheduledSourceNode*> finished;
    for (size_t i = 0; i < m_activeSources.size();) {
        AudioScheduledSourceNode* source = m_activeSources[i];
        if (source->renderScheduledQuantum()) {
            finished.push_back(source);
            m_activeSources.erase(m_activeSources.begin() + i);
        } else {
            i++;
        }
    }
    for (AudioScheduledSourceNode* source : finished) {
        source->dispatchEnded();
    }
}

String* BaseAudioContext::state()
{
    switch (m_stateAttribute) {
    case AudioContextState::Suspended:
        return String::createASCIIString("suspended");
    case AudioContextState::Running:
        return String::createASCIIString("running");
    case AudioContextState::Closed:
        return String::createASCIIString("closed");
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    return String::emptyString;
}

DEFINE_EVENT_LISTENER(BaseAudioContext, statechange);

void BaseAudioContext::dispatchStateChange()
{
    Event* event = new Event(m_executionContext,
                             staticStrings()->m_statechange.localName());
    dispatchEventByUA(event);
}

void BaseAudioContext::updateStateAttributeAndQueueStateChange(
    AudioContextState state)
{
    if (m_stateAttribute == state) {
        return;
    }
    m_stateAttribute = state;
    m_controlQueue->enqueue(
        [](void* data) {
            static_cast<BaseAudioContext*>(data)->dispatchStateChange();
        },
        this);
}

AudioBufferSourceNode* BaseAudioContext::createBufferSource()
{
    AudioBufferSourceOptions options;
    AudioBufferSourceNode* node =
        new AudioBufferSourceNode(m_executionContext, this, options);

    return node;
}

ConstantSourceNode* BaseAudioContext::createConstantSource()
{
    return new ConstantSourceNode(m_executionContext, this);
}

ChannelMergerNode* BaseAudioContext::createChannelMerger(
    uint32_t numberOfInputs)
{
    ChannelMergerOptions options;
    options.setNumberOfInputs(numberOfInputs);
    return new ChannelMergerNode(m_executionContext, this, options);
}

ChannelSplitterNode* BaseAudioContext::createChannelSplitter(
    uint32_t numberOfOutputs)
{
    ChannelSplitterOptions options;
    options.setNumberOfOutputs(numberOfOutputs);
    return new ChannelSplitterNode(m_executionContext, this, options);
}

StereoPannerNode* BaseAudioContext::createStereoPanner()
{
    return new StereoPannerNode(m_executionContext, this);
}

PannerNode* BaseAudioContext::createPanner()
{
    return new PannerNode(m_executionContext, this);
}

OscillatorNode* BaseAudioContext::createOscillator()
{
    return new OscillatorNode(m_executionContext, this);
}

DelayNode* BaseAudioContext::createDelay(double maxDelayTime)
{
    DelayOptions options;
    options.setMaxDelayTime(maxDelayTime);
    return new DelayNode(m_executionContext, this, options);
}

IIRFilterNode* BaseAudioContext::createIIRFilter(
    const GCAtomicVector<double>& feedforward,
    const GCAtomicVector<double>& feedback)
{
    IIRFilterOptions options;
    options.setFeedforward(feedforward);
    options.setFeedback(feedback);
    return new IIRFilterNode(m_executionContext, this, options);
}

BiquadFilterNode* BaseAudioContext::createBiquadFilter()
{
    return new BiquadFilterNode(m_executionContext, this);
}

WaveShaperNode* BaseAudioContext::createWaveShaper()
{
    return new WaveShaperNode(m_executionContext, this);
}

AnalyserNode* BaseAudioContext::createAnalyser()
{
    return new AnalyserNode(m_executionContext, this);
}

DynamicsCompressorNode* BaseAudioContext::createDynamicsCompressor()
{
    return new DynamicsCompressorNode(m_executionContext, this);
}

PeriodicWave* BaseAudioContext::createPeriodicWave(
    const GCAtomicVector<double>& real, const GCAtomicVector<double>& imag)
{
    return createPeriodicWave(real, imag, PeriodicWaveConstraints());
}

PeriodicWave* BaseAudioContext::createPeriodicWave(
    const GCAtomicVector<double>& real, const GCAtomicVector<double>& imag,
    PeriodicWaveConstraints constraints)
{
    PeriodicWaveOptions options;
    options.setReal(real);
    options.setImag(imag);
    options.setDisableNormalization(constraints.disableNormalization());
    return new PeriodicWave(m_executionContext, this, options);
}

GainNode* BaseAudioContext::createGain()
{
    return new GainNode(m_executionContext, this);
}

ConvolverNode* BaseAudioContext::createConvolver()
{
    return new ConvolverNode(m_executionContext, this);
}

AudioBuffer* BaseAudioContext::createBuffer(uint32_t numberOfChannels,
                                            uint32_t length, double sampleRate)
{
    AudioBufferOptions options;
    options.setNumberOfChannels(numberOfChannels);
    options.setLength(length);
    options.setSampleRate(sampleRate);
    return new AudioBuffer(m_executionContext, options);
}

// Only native audio data crosses the worker boundary. WebView roots the
// script-facing request, and with it the encoded bytes' backing store, until
// the completion runs on the main thread.
class AudioDecodeWork {
public:
    AudioDecodeWork(const uint8_t* bytes, size_t length, double sampleRate,
                    MessageLoop* messageLoop)
        : m_bytes(bytes)
        , m_length(length)
        , m_sampleRate(sampleRate)
        , m_messageLoop(messageLoop)
    {
    }

    ~AudioDecodeWork()
    {
        if (m_pcm) {
            m_pcm->release();
        }
    }

    void cancel()
    {
        m_cancelled.store(true, std::memory_order_release);
    }

    bool cancelled() const
    {
        return m_cancelled.load(std::memory_order_acquire);
    }

    AudioBufferData* takePCM()
    {
        AudioBufferData* pcm = m_pcm;
        m_pcm = nullptr;
        return pcm;
    }

    static void* run(void* data)
    {
        auto* work = static_cast<AudioDecodeWork*>(data);
        if (!work->cancelled() && work->m_length) {
            work->m_pcm = decodeWaveAudio(work->m_bytes, work->m_length,
                                          work->m_sampleRate);
            if (!work->m_pcm && !work->cancelled()) {
                work->m_pcm = decodeCompressedAudio(
                    work->m_bytes, work->m_length, work->m_sampleRate);
            }
        }
        work->m_bytes = nullptr;
        work->m_messageLoop->addIdlerWithNoGCRootingInOtherThread(
            nullptr,
            [](size_t, void* data) {
                auto* work = static_cast<AudioDecodeWork*>(data);
                work->request->complete();
            },
            work);
        return nullptr;
    }

    AudioDecodeRequest* request{ nullptr }; // main thread only

private:
    // Owned by the detached ArrayBuffer's backing store, which the request
    // keeps reachable until complete() or discard() runs after this worker.
    const uint8_t* m_bytes;
    size_t m_length;
    double m_sampleRate;
    MessageLoop* m_messageLoop;
    std::atomic<bool> m_cancelled{ false };
    AudioBufferData* m_pcm{ nullptr };
};

AudioDecodeRequest::AudioDecodeRequest(BaseAudioContext* context,
                                       Promise* promise,
                                       DecodeSuccessCallback* successCallback,
                                       DecodeErrorCallback* errorCallback,
                                       Window* window, WebView* webView,
                                       AudioDecodeWork* work,
                                       Optional<BackingStoreRef*> encodedData)
    : m_context(context)
    , m_promise(promise)
    , m_successCallback(successCallback)
    , m_errorCallback(errorCallback)
    , m_window(window)
    , m_webView(webView)
    , m_work(work)
    , m_encodedData(encodedData)
{
}

Window* AudioDecodeRequest::window() const
{
    return m_window;
}

void AudioDecodeRequest::cancel()
{
    if (m_work->cancelled()) {
        return;
    }
    m_work->cancel();
    // https://webaudio.github.io/web-audio-api/#unloading-a-document
    // Pending decode promises are rejected before the Window realm is torn
    // down; completion from a worker must not call script in that old realm.
    auto* exception = new DOMException(m_context->executionContext(),
                                       DOMException::INVALID_STATE_ERR,
                                       "Audio context document is not active");
    m_promise->reject(exception->scriptValue());
}

void AudioDecodeRequest::discard()
{
    delete m_work;
    m_work = nullptr;
    m_encodedData = nullptr;
}

void AudioDecodeRequest::complete()
{
    std::unique_ptr<AudioDecodeWork> work(m_work);
    m_work = nullptr;
    // The worker has finished reading; let the encoded bytes be collected.
    m_encodedData = nullptr;
    m_webView->unregisterAudioDecodeRequest(this);
    if (work->cancelled()) {
        return;
    }
    if (!m_context->executionContext()->document()->isFullyActive()) {
        auto* exception = new DOMException(
            m_context->executionContext(), DOMException::INVALID_STATE_ERR,
            "Audio context document is not active");
        m_promise->reject(exception->scriptValue());
        return;
    }

    AudioBufferData* pcm = work->takePCM();
    if (!pcm) {
        auto* exception =
            new DOMException(m_context->executionContext(),
                             String::createASCIIString("Encoding error"),
                             String::createASCIIString("EncodingError"));
        m_promise->reject(exception->scriptValue());
        if (m_errorCallback) {
            m_errorCallback->call(m_context->scriptBindingInstance(),
                                  exception);
        }
        return;
    }

    AudioBuffer* audioBuffer = new AudioBuffer(m_context->executionContext(),
                                               pcm, m_context->sampleRate());
    m_promise->fulfill(audioBuffer->scriptValue());
    if (m_successCallback) {
        m_successCallback->call(m_context->scriptBindingInstance(),
                                audioBuffer);
    }
}

// https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-decodeaudiodata
Promise* BaseAudioContext::decodeAudioData(
    ScriptArrayBuffer audioData, DecodeSuccessCallback* successCallback,
    DecodeErrorCallback* errorCallback)
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (!executionContext()->document()->isFullyActive()) {
        auto exception = new DOMException(
            executionContext(), DOMException::INVALID_STATE_ERR,
            "Audio context document is not active");
        promise->reject(exception->scriptValue());
        return promise;
    }
    if (audioData->isDetachedBuffer()) {
        auto exception =
            new DOMException(executionContext(), DOMException::DATA_CLONE_ERR,
                             "Data clone error");
        promise->reject(exception->scriptValue());
        if (errorCallback) {
            struct ErrorParams : public gc {
                BaseAudioContext* self;
                DecodeErrorCallback* callback;
                DOMException* error;
            };
            ErrorParams* params = new ErrorParams;
            params->self = this;
            params->callback = errorCallback;
            params->error = exception;
            m_controlQueue->enqueue(
                [](void* data) {
                    ErrorParams* params = (ErrorParams*)data;
                    params->callback->call(
                        params->self->scriptBindingInstance(), params->error);
                    delete params;
                },
                params);
        }
        return promise;
    }

    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-decodeaudiodata
    // Detaching transfers the bytes away from script, so the decoder takes
    // the detached backing store instead of copying it; peak memory stays at
    // the encoded size. No extra size cap is needed here: the input is
    // already allocated by script, and AudioBufferData::create() bounds the
    // decoded output.
    Optional<BackingStoreRef*> encodedData;
    if (OptionalRef<BackingStoreRef> store = audioData->backingStore()) {
        encodedData = store.get();
    }
    const uint8_t* bytes =
        encodedData ? static_cast<const uint8_t*>(encodedData.value()->data())
                    : nullptr;
    const size_t length = encodedData ? audioData->byteLength() : 0;

    detachArrayBuffer(scriptBindingInstance(), audioData);

    Window* window = executionContext()->document()->window();
    WebView* webView = window->webView();
    auto* work = new AudioDecodeWork(bytes, length, sampleRate(),
                                     webView->messageLoop());
    auto* request =
        new AudioDecodeRequest(this, promise, successCallback, errorCallback,
                               window, webView, work, encodedData);
    work->request = request;
    webView->registerAudioDecodeRequest(request);
    webView->audioDecodeThreadPool()->addWork(executionContext(),
                                              AudioDecodeWork::run, work);

    return promise;
}

// https://webaudio.github.io/web-audio-api/#allowed-to-start
bool BaseAudioContext::isAllowedToStart()
{
    // The spec lets a user agent require sticky activation. Starfish has no
    // autoplay policy, so every context may start.
    return true;
}
} // namespace Starfish

#endif
