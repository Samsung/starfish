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

SourceBuffer::SourceBuffer(StarFish* starFish, String* type)
    : EventTarget()
    , m_mode(AppendMode::Segments)
    , m_state(AppendState::WaitingForSegment)
    , m_isAttachedToParent(false)
    , m_updating(false)
    , m_starFish(starFish)
    , m_appendWindowStart(0)
    , m_appendWindowEnd(std::numeric_limits<double>::infinity())
    , m_type(type)
    , m_parentMediaSource(nullptr)
{

}

void SourceBuffer::setUpdating(bool flag, UpdateState state)
{
    STARFISH_ASSERT(m_updating != flag);
    m_updating = flag;

    String* eventName = String::emptyString;
    if (m_updating && state == SourceBuffer::Success)
        eventName = m_parentMediaSource->starFish()->staticStrings()->m_updatestart.localName();
    else if (!m_updating) {
        if (state == SourceBuffer::Success) {
            m_parentMediaSource->starFish()->messageLoop()->addIdler([](size_t, void* data, void* data2) {
                ((MediaSource*)data)->dispatchEvent((Event*)data2);
            }, this, new Event(m_parentMediaSource->starFish()->staticStrings()->m_update.localName()));
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_updateend.localName();
        } else if (state == SourceBuffer::Error)
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_error.localName();
        else if (state == SourceBuffer::Abort)
            eventName = m_parentMediaSource->starFish()->staticStrings()->m_abort.localName();
        else
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    m_parentMediaSource->starFish()->messageLoop()->addIdler([](size_t, void* data, void* data2) {
        ((MediaSource*)data)->dispatchEvent((Event*)data2);
    }, this, new Event(eventName));
}

void SourceBuffer::appendBuffer(const uint8_t* data, unsigned long length)
{
    // Run the prepare append algorithm.
    prepareAppend();

    // Add data to the end of the input buffer.
    auto d = new SourceBufferData(data, length);
    m_sourceBufferDataList.push_back(d);

    // Set the updating attribute to true.
    // Queue a task to fire a simple event named updatestart at this SourceBuffer object.
    STARFISH_ASSERT(m_updating == false);
    setUpdating(true, UpdateState::Success);

    // Asynchronously run the buffer append algorithm.
    m_parentMediaSource->starFish()->messageLoop()->addIdler([](size_t, void* data, void* data2) {
        ((SourceBuffer*)data)->bufferAppend((SourceBufferData*)data2);
    }, this, d);
}

void SourceBuffer::prepareAppend()
{
    // 3.5.4 Prepare Append Algorithm

    // If the SourceBuffer has been removed from the sourceBuffers attribute of the parent media source then throw an InvalidStateError exception and abort these steps.
    if (!m_isAttachedToParent) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer has been removed from from parernt MediaSource");
    }

    // If the updating attribute equals true, then throw an InvalidStateError exception and abort these steps.
    if (m_updating) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "SourceBuffer is now updating");
    }

    // TODO If the HTMLMediaElement.error attribute is not null, then throw an InvalidStateError exception and abort these steps.

    // If the readyState attribute of the parent media source is in the "ended" state then run the following steps:
    if (m_parentMediaSource->readyState() == MediaSource::Ended) {
        // Set the readyState attribute of the parent media source to "open"
        // Queue a task to fire a simple event named sourceopen at the parent media source.
        m_parentMediaSource->setReadyState(MediaSource::Open);
    }

    // Run the coded frame eviction algorithm.
    codedFrameEviction();

    // TODO If the buffer full flag equals true, then throw a QuotaExceededError exception and abort these step.
}

void SourceBuffer::codedFrameEviction()
{
    // TODO 3.5.14 Coded Frame Eviction Algorithm
}

void SourceBuffer::bufferAppend(SourceBufferData* inputBuffer)
{
    STARFISH_ASSERT(inputBuffer->m_isProcessed == false);

    // TODO

    inputBuffer->m_isProcessed = true;
    setUpdating(false, UpdateState::Success);
}

void SourceBuffer::setTimestampOffset(double timeoffset)
{
    // TODO
}

void setAppendWindowStart(double timeStamp)
{
    // TODO
}

void setAppendWindowEnd(double timeStamp)
{
    // TODO
}

}

#endif
