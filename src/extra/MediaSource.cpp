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
    , m_duration(std::numeric_limits<double>::quiet_NaN())
{
}

SourceBuffer* MediaSource::addSourceBuffer(String* type)
{
    // If type is an empty string then throw a TypeError exception and abort these steps.
    if (type->equals(String::emptyString))
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "Unsupport type");
    // If type contains a MIME type that is not supported or contains a MIME type that is not supported with the types specified for the other SourceBuffer objects in sourceBuffers, then throw a NotSupportedError exception and abort these steps.
    if (!isTypeSupported(type))
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::NOT_SUPPORTED_ERR, "Unsupport type");

    // TODO If the user agent can't handle any more SourceBuffer objects or if creating a SourceBuffer based on type would result in an unsupported SourceBuffer configuration, then throw a QuotaExceededError exception and abort these steps.
    // If the readyState attribute is not in the "open" state then throw an InvalidStateError exception and abort these steps.
    if (m_readyState != Open)
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "When execute appendSourceBuffer, readyState of MediaSource must be 'open'");

    STARFISH_ASSERT(isTypeSupported(type));
    STARFISH_ASSERT(m_readyState == Open);

    SourceBuffer* buffer = new SourceBuffer(m_starFish, type);
    if (!m_sourceBuffers)
        m_sourceBuffers = new SourceBufferList();
    m_sourceBuffers->add(buffer, this);

    // FIXME Set the generate timestamps flag on the new object to the value in the "Generate Timestamps Flag" column of the byte stream format registry [MSE-REGISTRY] entry that is associated with type.
    bool generateTimestampsFlag = false;
    if (type->contains("webm") || type->contains("mp4") || type->contains("mp2t")) {
        generateTimestampsFlag = false;
    } else if (type->contains("mpeg") || type->contains("aac")) {
        generateTimestampsFlag = true;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // If the generate timestamps flag equals true:
    if (generateTimestampsFlag) {
        // Set the mode attribute on the new object to "sequence".
        buffer->setMode(SourceBuffer::AppendMode::Sequence);
    } else {
        // Set the mode attribute on the new object to "segments".
        buffer->setMode(SourceBuffer::AppendMode::Segments);
    }

    Event* e = new Event(String::fromUTF8("addsourcebuffer"), EventInit(false, false));
    m_sourceBuffers->dispatchEvent(m_sourceBuffers, e);
    return buffer;
}

void MediaSource::endOfStream()
{
    endOfStream(None);
}

void MediaSource::endOfStream(EndOfStreamError error)
{
    // If the readyState attribute is not in the "open" state then throw an InvalidStateError exception and abort these steps.
    if (m_readyState != Open) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "readyState must be OPEN");
    }

    // TODO If the updating attribute equals true on any SourceBuffer in sourceBuffers, then throw an InvalidStateError exception and abort these steps.
    // TODO Run the end of stream algorithm with the error parameter set to error.
}

void MediaSource::setDuration(double d)
{
    // If the value being set is negative or NaN then throw a TypeError exception and abort these steps.
    if (d < 0 || std::isnan(d)) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::TYPE_ERR, "duration must be postive and not NaN.");
    }

    // If the readyState attribute is not "open" then throw an InvalidStateError exception and abort these steps.
    if (m_readyState != ReadyState::Open) {
        throw new DOMException(m_starFish->window()->scriptBindingInstance(), DOMException::INVALID_STATE_ERR, "readyState must be OPEN");
    }

    // TODO If the updating attribute equals true on any SourceBuffer in sourceBuffers, then throw an InvalidStateError exception and abort these steps.
    // TODO Run the duration change algorithm with new duration set to the value being assigned to this attribute.
    m_duration = d;
}

void MediaSource::setReadyState(ReadyState state)
{
    STARFISH_ASSERT(m_readyState != state);
    m_readyState = state;

    String* eventName = String::emptyString;
    if (m_readyState == MediaSource::Open)
        eventName = starFish()->staticStrings()->m_sourceopen.localName();
    else if (m_readyState == MediaSource::Ended)
        eventName = starFish()->staticStrings()->m_sourceended.localName();
    else if (m_readyState == MediaSource::Closed)
        eventName = starFish()->staticStrings()->m_sourceclose.localName();
    else
        STARFISH_RELEASE_ASSERT_NOT_REACHED();

    m_starFish->messageLoop()->addIdler([](size_t, void* data, void* data2) {
        ((MediaSource*)data)->dispatchEvent((Event*)data2);
    }, this, new Event(eventName));
}

// https://www.w3.org/TR/media-source/
// 2.4.1 Attaching to a media element
bool MediaSource::attach()
{
    if (m_readyState != Closed) {
        return false;
    }
    setReadyState(Open);
    return true;
}


// https://www.w3.org/TR/media-source/
// 2.4.2 Detaching from a media element
void MediaSource::detach()
{
    // TODO
    setReadyState(Closed);
}

}
#endif
