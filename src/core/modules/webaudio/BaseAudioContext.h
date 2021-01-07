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
}

namespace Starfish {
class ExecutionContext;
class AudioDestinationNode;

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

class DecodeSuccessCallback {
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

class DecodeErrorCallback {
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

class BaseAudioContext : public EventTarget {
public:
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(BaseAudioContext)

    virtual ExecutionContext* executionContext() const override
    {
        return m_executionContext;
    }

    virtual AudioDestinationNode* destination();
    virtual String* state();

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(statechange);
#undef VIRTUAL
#undef OVERRIDE

    virtual AudioBufferSourceNode* createBufferSource();

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

protected:
    BaseAudioContext(ExecutionContext* executionContext);

    ExecutionContext* m_executionContext{ nullptr };

    AudioDestinationNode* m_destination{ nullptr };

    ControlMessageQueue* m_controlQueue{ nullptr };
    RenderingMessageQueue* m_renderingQueue{ nullptr };
    bool m_suspendedByUser{ false };
};
} // namespace Starfish
#endif
#endif
