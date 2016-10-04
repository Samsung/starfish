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
#include "SourceBuffer.h"
#include "dom/Event.h"
#include "dom/DOMException.h"
#include "dom/HTMLMediaElement.h"
#include "platform/multimedia/MediaPlayer.h"
#include "platform/message_loop/MessageLoop.h"
#include "MediaSource.h"

namespace StarFish {

SourceBuffer::SourceBuffer(String* type, MediaSource* parent)
    : EventTarget()
    , m_updating(false)
    , m_type(type)
    , m_parentMediaSource(parent)
{
}

void SourceBuffer::dispatchUpdateEvent(UpdateState state)
{
    String* eventName = String::emptyString;
    if (m_updating && state == SourceBuffer::Success)
        eventName = m_parentMediaSource->starFish()->staticStrings()->m_updatestart.localName();
    else if (!m_updating) {
        if (state == SourceBuffer::Success)
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_update.localName();
        else if (state == SourceBuffer::Error)
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_error.localName();
        else if (state == SourceBuffer::Abort)
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_abort.localName();
        else
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    dispatchEvent(this, new Event(eventName));

    if (!m_updating) {
        eventName = m_parentMediaSource->starFish()->staticStrings()->m_updateend.localName();
        dispatchEvent(this, new Event(eventName));
    }
}

void SourceBuffer::setUpdating(bool flag, UpdateState state)
{
    if (m_updating != flag) {
        m_updating = flag;
        dispatchUpdateEvent(state);
    }
}

void SourceBuffer::appendBuffer(const void* data, unsigned long length)
{
    m_parentMediaSource->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
        SourceBuffer* sourceBuffer = (SourceBuffer*)data;
        sourceBuffer->setUpdating(true, SourceBuffer::Success);
    }, this);
}

}

#endif
