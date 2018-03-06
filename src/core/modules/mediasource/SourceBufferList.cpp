/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
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
    , m_parentMediaSource(sb)
{
}

void SourceBufferList::add(SourceBuffer* buffer, MediaSource* ms)
{
    m_list.push_back(buffer);
    buffer->attachedToParent(ms);
    scheduleEvent(starFish()->staticStrings()->m_addsourcebuffer.localName());
}

void SourceBufferList::remove(unsigned long index)
{
    SourceBuffer* buf = m_list[index];
    m_list.erase(m_list.begin() + index);
    buf->detachFromParent();
    scheduleEvent(
        starFish()->staticStrings()->m_removesourcebuffer.localName());
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
        starFish()->staticStrings()->m_removesourcebuffer.localName());
}

void SourceBufferList::scheduleEvent(String* eventName)
{
    if (m_parentMediaSource) {
        m_parentMediaSource->attachedMediaElement()->addEventToOperationQueue(
            this, new Event(document(), eventName));
    }
}
}
#endif
