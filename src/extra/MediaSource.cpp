/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "MediaSource.h"
#include "SourceBuffer.h"
#include "dom/Event.h"
#include "dom/DOMException.h"
#include "platform/window/Window.h"
#include "platform/message_loop/MessageLoop.h"

namespace StarFish {

MediaSource::MediaSource(StarFish* starFish)
    : EventTarget()
    , m_readyState(Closed)
    , m_starFish(starFish)
    , m_duration(0)
{
}

SourceBuffer* MediaSource::addSourceBuffer(String* type)
{
    if (!isTypeSupported(type))
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "Unsupport type");
    if (m_readyState != Open)
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "When execute appendSourceBuffer, readyState of MediaSource must be 'open'");

    STARFISH_ASSERT(isTypeSupported(type));
    STARFISH_ASSERT(m_readyState == Open);

    SourceBuffer* buffer = new SourceBuffer(type, this);
    if (!m_sourceBuffers)
        m_sourceBuffers = new SourceBufferList();
    m_sourceBuffers->add(buffer);

    Event* e = new Event(String::fromUTF8("addsourcebuffer"), EventInit(false, false));
    m_sourceBuffers->dispatchEvent(m_sourceBuffers, e);
    return buffer;
}

void MediaSource::endOfStream()
{
    starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
        MediaSource* mediaSource = (MediaSource*)data;
        mediaSource->setReadyState(MediaSource::Ended);
    }, this);
}

void MediaSource::endOfStream(EndOfStreamError error)
{
    // TODO
}

void MediaSource::dispatchStateChangeEvent()
{
    String* eventName = String::emptyString;
    if (m_readyState == MediaSource::Open)
        eventName = starFish()->staticStrings()->m_sourceopen.localName();
    else if (m_readyState == MediaSource::Ended)
        eventName = starFish()->staticStrings()->m_sourceended.localName();
    else if (m_readyState == MediaSource::Closed)
        eventName = starFish()->staticStrings()->m_sourceclose.localName();
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    dispatchEvent(this, new Event(eventName));
}

void MediaSource::setReadyState(ReadyState state)
{
    if (m_readyState != state) {
        m_readyState = state;
        dispatchStateChangeEvent();
    }
}

}
#endif
