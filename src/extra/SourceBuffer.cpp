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
#include "platform/multimedia/MediaSourceClient.h"

namespace StarFish {

SourceBuffer::SourceBuffer(String* type, MediaSourceClient* client)
    : EventTarget()
    , m_type(type)
    , m_mseClient(client)
{
    m_mseClient->setFormat(type);
}

void SourceBuffer::runBufferAppend()
{
    m_mseClient->appendBuffer(m_inputBuffer);
    setUpdating(false);
    dispatchEvent(new Event(String::fromUTF8("update")));
    dispatchEvent(new Event(String::fromUTF8("updateend")));
}

void SourceBuffer::prepareAppend(const void* data, unsigned long length)
{
    if (m_inputBuffer.size == 0) {
        m_inputBuffer.memory = (char*) data;
        m_inputBuffer.size = length;
    }
}

void SourceBuffer::appendBuffer(const void* data, unsigned long length)
{
    printf("SourceBuffer::appendBuffer %ld\n", length);
    prepareAppend(data, length);

    setUpdating(true);
    dispatchEvent(new Event(String::fromUTF8("updatestart")));

    // TODO: should run asynchrously
    runBufferAppend();
}

}

#endif
