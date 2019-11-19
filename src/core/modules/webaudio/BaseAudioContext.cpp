/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#include "core/page/Window.h"
#include "core/modules/webaudio/AudioBuffer.h"
#include "core/modules/webaudio/AudioBufferSourceNode.h"
#include "core/modules/webaudio/AudioDestinationNode.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebBase.h"

namespace Starfish {
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

BaseAudioContext::BaseAudioContext(ExecutionContext* executionContext)
    : EventTarget()
    , m_executionContext(executionContext)
{
}

ScriptBindingInstance* BaseAudioContext::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

AudioDestinationNode* BaseAudioContext::destination()
{
    // TODO: destination represents the actual hardware output stream
    AudioDestinationNode* dest =
        new AudioDestinationNode(m_executionContext, this);
    return dest;
}

String* BaseAudioContext::state()
{
    switch (m_state) {
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

AudioBufferSourceNode* BaseAudioContext::createBufferSource()
{
    AudioBufferSourceOptions options;
    AudioBufferSourceNode* node =
        new AudioBufferSourceNode(m_executionContext, this, options);

    return node;
}

// https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-decodeaudiodata
Promise* BaseAudioContext::decodeAudioData(
    ScriptArrayBuffer audioData, DecodeSuccessCallback* successCallback,
    DecodeErrorCallback* errorCallback)
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (audioData->isDetachedBuffer()) {
        auto exception =
            new DOMException(executionContext(), DOMException::DATA_CLONE_ERR,
                             "Data clone error");
        promise->reject(exception->scriptValue());
        if (errorCallback) {
            errorCallback->call(scriptBindingInstance(), exception);
        }
    }

    uint8_t* buffer = new uint8_t[audioData->byteLength()];
    uint32_t length = audioData->byteLength();
    memcpy(buffer, audioData->rawBuffer(), audioData->byteLength());

    detachArrayBuffer(scriptBindingInstance(), audioData);

    struct Params {
        BaseAudioContext* self;
        Promise* promise;
        uint8_t* buffer;
        uint32_t length;
        DecodeSuccessCallback* successCallback;
    };

    Params* p = new Params{ this, promise, buffer, length, successCallback };

    executionContext()->webBase()->messageLoop()->addIdler(
        executionContext()->document()->window(),
        [](size_t, void* data) {
            Params* p = (Params*)data;
            BaseAudioContext* self = p->self;
            Promise* promise = p->promise;
            DecodeSuccessCallback* successCallback = p->successCallback;

            bool canDecode = true;
            // 2:
            // https://mimesniff.spec.whatwg.org/#matching-an-audio-or-video-type-pattern

            if (!canDecode) {
                auto exception = new DOMException(self->executionContext(),
                                                  DOMException::ENCODING_ERROR,
                                                  "Encoding error");
                promise->reject(exception->scriptValue());
            }

            std::unique_ptr<uint8_t> buffer(p->buffer);
            AudioBuffer* audioBuffer = new AudioBuffer(
                self->executionContext(), std::move(buffer), p->length);

            promise->fulfill(audioBuffer->scriptValue());
            if (successCallback) {
                successCallback->call(self->scriptBindingInstance(),
                                      audioBuffer);
            }
            delete p;
        },
        p);

    return promise;
}
} // namespace Starfish

#endif
