/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/mediasource/SourceBufferList.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace StarFish {

MediaSource::MediaSource(Document* document)
    : EventTarget(document)
    , m_readyState(Closed)
    , m_isActiveBufferComputed(false)
    , m_attachedMediaElement(nullptr)
    , m_activeVideoSourceBuffer(nullptr)
    , m_activeVideoStreamIndex(SIZE_MAX)
    , m_activeAudioSourceBuffer(nullptr)
    , m_activeAudioStreamIndex(SIZE_MAX)
    , m_duration(std::numeric_limits<double>::quiet_NaN())
    , m_shortestMediaDuration(std::numeric_limits<uint64_t>::max())
{
#ifndef NDEBUG
    STARFISH_LOG_INFO("[TRACE_MSE_GC] MediaSource::MediaSource (%p)\n", this);
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            STARFISH_LOG_INFO("[TRACE_MSE_GC] MediaSource::~MediaSource (%p)\n",
                              obj);
        },
        NULL, NULL, NULL);
#endif
}

SourceBuffer* MediaSource::addSourceBuffer(String* type)
{
    // If type is an empty string then throw a TypeError exception and abort
    // these steps.
    if (type->equals(String::emptyString)) {
        throw new DOMException(document(), DOMException::TYPE_ERR,
                               "Unsupport type");
    }

    // If type contains a MIME type that is not supported or contains a MIME
    // type that is not supported with the types specified for the other
    // SourceBuffer objects in sourceBuffers, then throw a NotSupportedError
    // exception and abort these steps.
    if (!isTypeSupported(type)) {
        throw new DOMException(document(), DOMException::NOT_SUPPORTED_ERR,
                               "Unsupport type");
    }

    // If the user agent can't handle any more SourceBuffer objects or if
    // creating a SourceBuffer based on type would result in an unsupported
    // SourceBuffer configuration, then throw a QuotaExceededError exception and
    // abort these steps.
    if (m_isActiveBufferComputed) {
        throw new DOMException(document(), DOMException::QUOTA_EXCEEDED_ERR,
                               "This MediaSource has reached the limit of "
                               "SourceBuffer objects it can handle. No "
                               "additional SourceBuffer objects may be added.");
    }

    // If the readyState attribute is not in the "open" state then throw an
    // InvalidStateError exception and abort these steps.
    if (m_readyState != Open) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "When execute appendSourceBuffer, readyState of "
                               "MediaSource must be 'open'");
    }

    STARFISH_ASSERT(isTypeSupported(type));
    STARFISH_ASSERT(m_readyState == Open);

    SourceBuffer* buffer = new SourceBuffer(m_document, type);
    if (!m_sourceBuffers) {
        m_sourceBuffers = new SourceBufferList(m_document, this);
    }
    m_sourceBuffers->add(buffer, this);

    // FIXME Set the generate timestamps flag on the new object to the value in
    // the "Generate Timestamps Flag" column of the byte stream format registry
    // [MSE-REGISTRY] entry that is associated with type.
    bool generateTimestampsFlag = false;
    if (type->contains("webm") || type->contains("mp4") ||
        type->contains("mp2t")) {
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

    Event* e = new Event(document(), String::fromUTF8("addsourcebuffer"),
                         EventInit(false, false));
    m_sourceBuffers->dispatchEventByUA(m_sourceBuffers, e);
    return buffer;
}

void MediaSource::removeSourceBuffer(SourceBuffer* buffer)
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void MediaSource::endOfStream()
{
    endOfStream(None);
}

void MediaSource::endOfStream(EndOfStreamError error)
{
    // If the readyState attribute is not in the "open" state then throw an
    // InvalidStateError exception and abort these steps.
    if (m_readyState != Open) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "readyState must be OPEN");
    }

    // If the updating attribute equals true on any SourceBuffer in
    // sourceBuffers, then throw an InvalidStateError exception and abort these
    // steps.
    if (anySourceBufferInUpdatingState()) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "When execute endOfStream, updating state of "
                               "child SourceBuffer must be false");
    }
    // Run the end of stream algorithm with the error parameter set to error.
    if (error == None) {
        // Run the duration change algorithm with new duration set to the
        // largest track buffer ranges end time across all the track buffers
        // across all SourceBuffer objects in sourceBuffers.
        uint64_t lastTimeStamp = 0;
        if (activeVideoSourceBuffer()) {
            lastTimeStamp = activeVideoSourceBuffer()->lastBufferedTimestamp(
                activeVideoStreamIndex());
        }
        if (activeAudioSourceBuffer()) {
            uint64_t temp = activeAudioSourceBuffer()->lastBufferedTimestamp(
                activeAudioStreamIndex());
            if (temp > lastTimeStamp) {
                lastTimeStamp = temp;
            }
        }
        setDuration(lastTimeStamp / 1000.0);
    } else if (error == Network) {
        // If the HTMLMediaElement.readyState attribute equals HAVE_NOTHING
        if (attachedMediaElement()->readyState() ==
            HTMLMediaElement::HAVE_NOTHING) {
            // Run the "If the media data cannot be fetched at all, due to
            // network errors, causing the user agent to give up trying to fetch
            // the resource" steps of the resource fetch algorithm's media data
            // processing steps list.
            attachedMediaElement()->giveupFetchingResource(false);
        } else {
            // If the HTMLMediaElement.readyState attribute is greater than
            // HAVE_NOTHING
            // Run the "If the connection is interrupted after some media data
            // has been received, causing the user agent to give up trying to
            // fetch the resource" steps of the resource fetch algorithm's media
            // data processing steps list.
            // TODO add error
            attachedMediaElement()->giveupFetchingResource();
        }
    } else if (error == Decode) {
        // If the HTMLMediaElement.readyState attribute equals HAVE_NOTHING
        if (attachedMediaElement()->readyState() ==
            HTMLMediaElement::HAVE_NOTHING) {
            // Run the "If the media data can be fetched but is found by
            // inspection to be in an unsupported format, or can otherwise not
            // be rendered at all" steps of the resource fetch algorithm's media
            // data processing steps list.
            attachedMediaElement()->giveupFetchingResource(false);
        } else {
            // If the HTMLMediaElement.readyState attribute is greater than
            // HAVE_NOTHING
            // Run the media data is corrupted steps of the resource fetch
            // algorithm's media data processing steps list.
            // TODO add error
            attachedMediaElement()->giveupFetchingResource();
        }
    }

    // Change the readyState attribute value to "ended".
    // Queue a task to fire a simple event named sourceended at the MediaSource.
    setReadyState(Ended);
}

void MediaSource::setDuration(double d, bool checkCurrentDuration)
{
    // If the value being set is negative or NaN then throw a TypeError
    // exception and abort these steps.
    if (d < 0 || std::isnan(d)) {
        throw new DOMException(document(), DOMException::TYPE_ERR,
                               "duration must be postive and not NaN.");
    }

    // If the readyState attribute is not "open" then throw an InvalidStateError
    // exception and abort these steps.
    if (m_readyState != ReadyState::Open) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "readyState must be OPEN");
    }

    // If the updating attribute equals true on any SourceBuffer in
    // sourceBuffers, then throw an InvalidStateError exception and abort these
    // steps.
    if (anySourceBufferInUpdatingState()) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "when updating duration of MediaSource, every "
                               "SourceBuffer must has non-updating state");
    }

    // 2.4.6 Duration change
    // If the current value of duration is equal to new duration, then return.
    if (m_duration == d) {
        return;
    }

    // If new duration is less than the highest presentation timestamp of any
    // buffered coded frames for all SourceBuffer objects in sourceBuffers, then
    // throw an InvalidStateError exception and abort these steps.
    uint64_t lastTimeStamp = 0;
    uint64_t shorest = 0;
    if (activeVideoSourceBuffer()) {
        shorest = lastTimeStamp =
            activeVideoSourceBuffer()->lastBufferedTimestamp(
                activeVideoStreamIndex());
    }
    if (activeAudioSourceBuffer()) {
        uint64_t temp = activeAudioSourceBuffer()->lastBufferedTimestamp(
            activeAudioStreamIndex());
        if (temp > lastTimeStamp) {
            lastTimeStamp = temp;
        }
        if (temp < shorest) {
            shorest = temp;
        }
    }

    STARFISH_LOG_INFO("MediaSource got new duration -> %lf %lf\n", d,
                      (lastTimeStamp / 1000.0));
    if (checkCurrentDuration && (d < (lastTimeStamp / 1000.0))) {
        throw new DOMException(document(), DOMException::INVALID_STATE_ERR,
                               "when updating duration of MediaSource, new "
                               "duration value should be larger than the "
                               "lagest value of current buffer stream.");
    }

    m_duration = d;
    m_shortestMediaDuration = shorest;
    m_attachedMediaElement->dispatchDurationchangeEvent();
}

String* MediaSource::readyState()
{
    switch (m_readyState) {
    case MediaSource::Open:
        return starFish()->staticStrings()->m_open.localName();
    case MediaSource::Ended:
        return starFish()->staticStrings()->m_ended.localName();
    case MediaSource::Closed:
        return starFish()->staticStrings()->m_closed.localName();
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

MediaSource::ReadyState MediaSource::readyStateValue()
{
    return m_readyState;
}

void MediaSource::setReadyState(MediaSource::ReadyState state)
{
    STARFISH_ASSERT(m_readyState != state);
    m_readyState = state;

    String* eventName = String::emptyString;
    switch (m_readyState) {
    case MediaSource::Open:
        eventName = starFish()->staticStrings()->m_sourceopen.localName();
        break;
    case MediaSource::Ended:
        eventName = starFish()->staticStrings()->m_sourceended.localName();
        break;
    case MediaSource::Closed:
        eventName = starFish()->staticStrings()->m_sourceclose.localName();
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    m_attachedMediaElement->addEventToOperationQueue(
        this, new Event(document(), eventName));
}

// https://www.w3.org/TR/media-source/
// 2.4.1 Attaching to a media element
bool MediaSource::attach(HTMLMediaElement* e)
{
    if (m_readyState != Closed) {
        return false;
    }
    m_attachedMediaElement = e;
    setReadyState(Open);

    if (m_isActiveBufferComputed) {
        for (size_t i = 0; i < m_clients.size(); i++) {
            m_clients[i]->activeSourceComputed();
        }
    }
    return true;
}

// https://www.w3.org/TR/media-source/
// 2.4.2 Detaching from a media element
void MediaSource::detach()
{
    STARFISH_LOG_INFO("[TRACE_MSE_GC] MediaSource::detach()\n");
    // Update duration to NaN.
    m_duration = std::numeric_limits<
        double>::quiet_NaN(); // update duration directly for avoiding exception
    m_shortestMediaDuration = std::numeric_limits<uint64_t>::max();

    // Remove all the SourceBuffer objects from activeSourceBuffers.
    // Queue a task to fire a simple event named removesourcebuffer at
    // activeSourceBuffers.
    if (m_activeSourceBuffers) {
        m_activeSourceBuffers->clear();
    }

    // Remove all the SourceBuffer objects from sourceBuffers.
    // Queue a task to fire a simple event named removesourcebuffer at
    // sourceBuffers.
    if (m_sourceBuffers) {
        for (unsigned i = 0; i < m_sourceBuffers->length(); i++) {
            (*m_sourceBuffers)[i]->detachFromParent();
        }
        m_sourceBuffers->clear();
        m_sourceBuffers->detachFromParent();
    }

    m_clients.clear();
    m_clients.shrink_to_fit();

    // Set the readyState attribute to "closed".
    // Queue a task to fire a simple event named sourceclose at the MediaSource.
    setReadyState(Closed);

    m_attachedMediaElement = nullptr;

    m_isActiveBufferComputed = false;

    m_activeVideoSourceBuffer = nullptr;
    m_activeVideoStreamIndex = SIZE_MAX;

    m_activeAudioSourceBuffer = nullptr;
    m_activeAudioStreamIndex = SIZE_MAX;

    m_sourceBuffers = nullptr;
    m_activeSourceBuffers = nullptr;
}

SourceBufferList* MediaSource::sourceBuffers()
{
    if (!m_sourceBuffers) {
        m_sourceBuffers = new SourceBufferList(m_document, this);
    }
    return m_sourceBuffers;
}

SourceBufferList* MediaSource::activeSourceBuffers()
{
    if (!m_activeSourceBuffers) {
        m_activeSourceBuffers = new SourceBufferList(m_document, this);
    }
    return m_activeSourceBuffers;
}

bool MediaSource::anySourceBufferInUpdatingState()
{
    if (m_sourceBuffers) {
        for (size_t i = 0; i < m_sourceBuffers->length(); i++) {
            if ((*m_sourceBuffers)[i]->updating()) {
                return true;
            }
        }
    }

    return false;
}

void MediaSource::didSourceBufferUpdated(SourceBuffer* src)
{
    if (m_readyState >= Open) {
        if (!m_isActiveBufferComputed) {
            // check every source buffer updating flag is false
            if (!anySourceBufferInUpdatingState()) {
                // check every source buffer has stream info
                bool allHaveInfo = true;
                for (size_t i = 0; i < m_sourceBuffers->length(); i++) {
                    if ((*m_sourceBuffers)[i]->m_indexPerInitSegment == 0) {
                        allHaveInfo = false;
                        break;
                    }
                }

                if (allHaveInfo) {
                    // find proper stream
                    // TODO implement this correctly
                    // currently, we choose the first stream of each media

                    SourceBufferList* activeSourceBuffers =
                        this->activeSourceBuffers();
                    uint64_t newDurationMS = 0;
                    for (size_t i = 0; i < m_sourceBuffers->length(); i++) {
                        STARFISH_ASSERT(
                            (*m_sourceBuffers)[i]->m_streamInfo.size() != 0);
                        const GCVector<StreamInfo*>& streamInfo =
                            (*m_sourceBuffers)[i]->m_streamInfo[0];
                        bool thisBufferAdded = false;
                        for (size_t j = 0; j < streamInfo.size(); j++) {
                            if (streamInfo[j]->m_type == StreamInfo::Video) {
                                if (m_activeVideoSourceBuffer == nullptr) {
                                    m_activeVideoSourceBuffer =
                                        (*m_sourceBuffers)[i];
                                    m_activeVideoStreamIndex =
                                        streamInfo[j]->m_streamIndex;
                                    if (newDurationMS <
                                        streamInfo[j]->m_duration) {
                                        newDurationMS =
                                            streamInfo[j]->m_duration;
                                    }
                                    if (!thisBufferAdded) {
                                        activeSourceBuffers->addWithoutEvent(
                                            m_activeVideoSourceBuffer);
                                        thisBufferAdded = true;
                                    }
                                }
                            } else if (streamInfo[j]->m_type ==
                                       StreamInfo::Audio) {
                                if (m_activeAudioSourceBuffer == nullptr) {
                                    m_activeAudioSourceBuffer =
                                        (*m_sourceBuffers)[i];
                                    m_activeAudioStreamIndex =
                                        streamInfo[j]->m_streamIndex;
                                    if (newDurationMS <
                                        streamInfo[j]->m_duration) {
                                        newDurationMS =
                                            streamInfo[j]->m_duration;
                                    }
                                    if (!thisBufferAdded) {
                                        activeSourceBuffers->addWithoutEvent(
                                            m_activeAudioSourceBuffer);
                                        thisBufferAdded = true;
                                    }
                                }
                            }
                        }
                    }
                    if (newDurationMS != 0) {
                        setDuration(newDurationMS / 1000.0, false);
                    } else if (std::isnan(m_duration)) {
                        setDuration(std::numeric_limits<double>::infinity(),
                                    false);
                    }
                    m_isActiveBufferComputed = true;

                    for (size_t i = 0; i < m_clients.size(); i++) {
                        m_clients[i]->activeSourceComputed();
                    }
                }
            }
        } else {
            if (src == m_activeVideoSourceBuffer) {
                for (size_t i = 0; i < m_clients.size(); i++) {
                    m_clients[i]->activeVideoSourceBufferUpdated(src);
                }
            }
            if (src == m_activeAudioSourceBuffer) {
                for (size_t i = 0; i < m_clients.size(); i++) {
                    m_clients[i]->activeAudioSourceBufferUpdated(src);
                }
            }
        }
    }
}

void MediaSource::addClient(MediaSourceClient* c)
{
    c->setMediaSource(this);
    m_clients.push_back(c);
}

void MediaSource::removeClient(MediaSourceClient* c)
{
    c->removeMediaSource();
    m_clients.erase(std::find(m_clients.begin(), m_clients.end(), c));
}
}
#endif
