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
#include "core/dom/Event.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/webaudio/MediaElementAudioSourceNode.h"
#include "core/page/WebBase.h"

namespace Starfish {

// https://webaudio.github.io/web-audio-api/#AudioContext-constructors
AudioContext::AudioContext(ExecutionContext* executionContext,
                           AudioContextOptions contextOptions)
    : BaseAudioContext(executionContext)
{
    if (isAllowedToStart()) {
        m_renderingQueue->setState(AudioContextState::Running);
        m_controlQueue->setState(AudioContextState::Running);
        m_controlQueue->enqueue(
            [](void* data) {
                AudioContext* self = (AudioContext*)data;
                String* eventType = self->executionContext()
                                        ->starfish()
                                        ->staticStrings()
                                        ->m_statechange.localName();
                Event* e = new Event(self->executionContext(), eventType);
                self->dispatchEventByUA(e);
                self->m_controlQueue->setState(AudioContextState::Suspended);
                self->m_renderingQueue->setState(AudioContextState::Suspended);
            },
            this);
    } else {
        STARFISH_LOG_WARN("Failed: AudioContext cannot be created");
    }
}

ScriptBindingInstance* AudioContext::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
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

// https://webaudio.github.io/web-audio-api/#dom-audiocontext-close
Promise* AudioContext::close()
{
    Promise* promise = new Promise(scriptBindingInstance());
    if (m_controlQueue->state() == AudioContextState::Closed) {
        promise->fulfill(scriptUndefined());
        return promise;
    }

    struct Params {
        AudioContext* self;
        Promise* promise;
    };

    Params* p = new Params{ this, promise };

    // TODO: The steps in the spec seems incomplete. Skip state management now.
    m_renderingQueue->enqueue(
        [](void* data) {
            Params* p = (Params*)data;
            AudioContext* self = p->self;
            p->promise->fulfill(scriptUndefined());
            String* eventType = self->executionContext()
                                    ->starfish()
                                    ->staticStrings()
                                    ->m_statechange.localName();
            Event* e = new Event(self->executionContext(), eventType);
            self->dispatchEventByUA(e);
            delete p;
        },
        p);

    return promise;
}
} // namespace Starfish

#endif
