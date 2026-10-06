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

#ifndef __StarfishBaseAudioContext__
#define __StarfishBaseAudioContext__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

namespace Escargot {
class ValueRef;
class BackingStoreRef;
} // namespace Escargot

namespace Starfish {
class ExecutionContext;
class AudioDestinationNode;
class AudioListener;
class AudioBufferSourceNode;
class AudioScheduledSourceNode;
class ConstantSourceNode;
class ChannelMergerNode;
class ChannelSplitterNode;
class StereoPannerNode;
class PannerNode;
class ConvolverNode;
class OscillatorNode;
class DelayNode;
class IIRFilterNode;
class BiquadFilterNode;
class WaveShaperNode;
class AnalyserNode;
class DynamicsCompressorNode;
class PeriodicWave;
struct PeriodicWaveConstraints;
class AudioGraph;
class AudioDecodeWork;
class BaseAudioContext;
class Window;
class WebView;

enum class AudioContextState { Suspended, Running, Closed };

typedef void (*MessageQueueFunction)(void*);

class WebAudioMessageQueue : public gc {
public:
    void enqueue(MessageQueueFunction fn, void* data);

    virtual AudioContextState state()
    {
        return m_state;
    }

    virtual void setState(AudioContextState state)
    {
        m_state = state;
    }

protected:
    WebAudioMessageQueue(ExecutionContext* executionContext);
    ExecutionContext* m_executionContext{ nullptr };
    AudioContextState m_state{ AudioContextState::Suspended };
};

class ControlMessageQueue : public WebAudioMessageQueue {
public:
    ControlMessageQueue(ExecutionContext* executionContext);

private:
};

class RenderingMessageQueue : public WebAudioMessageQueue {
public:
    RenderingMessageQueue(ExecutionContext* executionContext);

private:
};

class DecodeSuccessCallback : public gc {
public:
    static DecodeSuccessCallback* toDecodeSuccessCallback(ScriptValue fn)
    {
        if (!isCallableScriptValue(fn)) {
            return nullptr;
        }

        return new DecodeSuccessCallback(fn);
    }

    void call(ScriptBindingInstance* instance, AudioBuffer* decodedData);

private:
    DecodeSuccessCallback(ScriptValue fn)
        : m_callback(fn)
    {
    }

    ScriptValue m_callback;
};

class DecodeErrorCallback : public gc {
public:
    static DecodeErrorCallback* toDecodeErrorCallback(ScriptValue fn)
    {
        if (!isCallableScriptValue(fn)) {
            return nullptr;
        }
        return new DecodeErrorCallback(fn);
    }

    void call(ScriptBindingInstance* instance, DOMException* error);

private:
    DecodeErrorCallback(ScriptValue fn)
        : m_callback(fn)
    {
    }

    ScriptValue m_callback;
};

// Rooted by WebView until the worker's main-thread completion runs.
class AudioDecodeRequest : public gc {
public:
    AudioDecodeRequest(BaseAudioContext* context, Promise* promise,
                       DecodeSuccessCallback* successCallback,
                       DecodeErrorCallback* errorCallback, Window* window,
                       WebView* webView, AudioDecodeWork* work,
                       Optional<Escargot::BackingStoreRef*> encodedData);
    Window* window() const;
    void cancel();
    void complete();
    void discard();

private:
    BaseAudioContext* m_context;
    Promise* m_promise;
    DecodeSuccessCallback* m_successCallback;
    DecodeErrorCallback* m_errorCallback;
    Window* m_window;
    WebView* m_webView;
    AudioDecodeWork* m_work;
    // The detached ArrayBuffer's store. Keeping it reachable keeps the encoded
    // bytes alive while the decode worker reads them without a copy.
    Optional<Escargot::BackingStoreRef*> m_encodedData;
};

class BaseAudioContext : public EventTarget {
public:
    ~BaseAudioContext() override;
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(BaseAudioContext)

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    virtual AudioDestinationNode* destination();
    AudioListener* listener();
    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-state
    // The attribute is updated only from queued media element tasks, so it
    // can lag behind [[control thread state]] (m_controlQueue).
    virtual String* state();
    DEFINE_GETTER(double, sampleRate)
    double currentTime() const;

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(statechange);
#undef VIRTUAL
#undef OVERRIDE

    virtual AudioBufferSourceNode* createBufferSource();
    ConstantSourceNode* createConstantSource();
    ChannelMergerNode* createChannelMerger(uint32_t numberOfInputs = 6);
    ChannelSplitterNode* createChannelSplitter(uint32_t numberOfOutputs = 6);
    StereoPannerNode* createStereoPanner();
    PannerNode* createPanner();
    ConvolverNode* createConvolver();
    OscillatorNode* createOscillator();
    DelayNode* createDelay(double maxDelayTime = 1.0);
    IIRFilterNode* createIIRFilter(const GCAtomicVector<double>& feedforward,
                                   const GCAtomicVector<double>& feedback);
    BiquadFilterNode* createBiquadFilter();
    WaveShaperNode* createWaveShaper();
    AnalyserNode* createAnalyser();
    DynamicsCompressorNode* createDynamicsCompressor();
    PeriodicWave* createPeriodicWave(const GCAtomicVector<double>& real,
                                     const GCAtomicVector<double>& imag);
    PeriodicWave* createPeriodicWave(const GCAtomicVector<double>& real,
                                     const GCAtomicVector<double>& imag,
                                     PeriodicWaveConstraints constraints);
    GainNode* createGain();
    virtual AudioBuffer* createBuffer(uint32_t numberOfChannels,
                                      uint32_t length, double sampleRate);

    virtual Promise* decodeAudioData(
        ScriptArrayBuffer audioData,
        DecodeSuccessCallback* successCallback = nullptr,
        DecodeErrorCallback* errorCallback = nullptr);

    virtual ControlMessageQueue* controlQueue()
    {
        return m_controlQueue;
    }

    virtual RenderingMessageQueue* renderingQueue()
    {
        return m_renderingQueue;
    }

    virtual bool isAllowedToStart();

    virtual bool suspendedByUser()
    {
        return m_suspendedByUser;
    }

    AudioGraph* graph() const
    {
        return m_graph;
    }
    void registerScheduledSource(AudioScheduledSourceNode* source);
    void renderScheduledSources();

protected:
    BaseAudioContext(ExecutionContext* executionContext,
                     double sampleRate = 48000);

    void setStateAttribute(AudioContextState state)
    {
        m_stateAttribute = state;
    }
    // Sets the state attribute to |state| unless it already has that value,
    // then queues a media element task to fire statechange, as the context
    // state transition algorithms require.
    // Promise::fulfill()/reject() run the microtask checkpoint on return, so
    // call this before settling the promise: reactions must observe the new
    // state, as they would at the end of the spec's media element task.
    void updateStateAttributeAndQueueStateChange(AudioContextState state);
    void dispatchStateChange();
    virtual void didRegisterScheduledSource()
    {
    }

    ExecutionContext* m_executionContext{ nullptr };

    AudioDestinationNode* m_destination{ nullptr };
    Optional<AudioListener*> m_listener;
    double m_sampleRate{ 48000 };
    AudioGraph* m_graph{ nullptr };

    ControlMessageQueue* m_controlQueue{ nullptr };
    RenderingMessageQueue* m_renderingQueue{ nullptr };
    GCVector<AudioScheduledSourceNode*> m_activeSources;
    AudioContextState m_stateAttribute{ AudioContextState::Suspended };
    bool m_suspendedByUser{ false };
};
} // namespace Starfish
#endif
#endif
