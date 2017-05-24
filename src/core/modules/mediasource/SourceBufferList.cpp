/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/mediasource/SourceBufferList.h"

namespace StarFish {

SourceBufferList::SourceBufferList(Document* document, MediaSource* sb)
    : EventTarget(document)
    , m_starFish(document->starFish())
    , m_parentMediaSource(sb)
{
}

void SourceBufferList::add(SourceBuffer* buffer, MediaSource* ms)
{
    m_list.push_back(buffer);
    buffer->attachedToParent(ms);
    scheduleEvent(m_starFish->staticStrings()->m_addsourcebuffer.localName());
}

void SourceBufferList::remove(unsigned long index)
{
    SourceBuffer* buf = m_list[index];
    m_list.erase(m_list.begin() + index);
    buf->detachFromParent();
    scheduleEvent(
        m_starFish->staticStrings()->m_removesourcebuffer.localName());
}

void SourceBufferList::remove(SourceBuffer* buffer)
{
    unsigned long size = m_list.size();
    unsigned long targetIdx = 0;
    for (targetIdx = 0; targetIdx < size; targetIdx++) {
        if (m_list[targetIdx] == buffer) {
            break;
        }
    }
    if (targetIdx < size) {
        remove(targetIdx);
    }
}

void SourceBufferList::clear()
{
    m_list.clear();
    m_list.shrink_to_fit();
    scheduleEvent(
        m_starFish->staticStrings()->m_removesourcebuffer.localName());
}

void SourceBufferList::scheduleEvent(String* eventName)
{
    m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(
        this, new Event(eventName));
}
}
