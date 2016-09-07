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
#include "platform/multimedia/MediaSourceClient.h"

namespace StarFish {

MediaSource::MediaSource(StarFish* starFish)
    : EventTarget()
    , m_starFish(starFish)
{
    m_mseClient = new MediaSourceClient();
}

SourceBuffer* MediaSource::addSourceBuffer(String* type)
{
    // TODO: consider type Error
    //
    // if (m_readyState != Open)
    //    throw InvalidStateError
//    STARFISH_ASSERT(isTypeSupported(type) == true)
//    STARFISH_ASSERT(m_readyState == Open)

    SourceBuffer* buffer = new SourceBuffer(type, this, m_mseClient);
    if (!m_sourceBuffers)
        m_sourceBuffers = new SourceBufferList();
    m_sourceBuffers->add(buffer);

    Event* e = new Event(String::fromUTF8("addsourcebuffer"), EventInit(false, false));
    m_sourceBuffers->dispatchEvent(e);
    return buffer;
}

void MediaSource::endOfStream()
{
    // TODO
    setReadyState(Ended);
    dispatchEvent(new Event(String::fromUTF8("sourceended")));
}

void MediaSource::endOfStream(EndOfStreamError error)
{
    // TODO
}

void MediaSource::registerMediaPlayer(VideoPlayer* player)
{
    m_mseClient->registerMediaPlayer(player);
    setReadyState(Open);
    dispatchEvent(new Event(String::fromUTF8("sourceopen")));
}

MediaSourceClient* MediaSource::mseClient()
{
    return m_mseClient;
}

}
#endif
