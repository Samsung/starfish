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
#ifndef __StarFishSourceBufferList__
#define __StarFishSourceBufferList__

#include "core/dom/EventTarget.h"

namespace StarFish {

class MediaSource;
class SourceBuffer;

class SourceBufferList : public EventTarget {
public:
    SourceBufferList(Document* document, MediaSource* sb);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSourceBufferList() const override;

    size_t length() const
    {
        return m_list.size();
    }

    void addWithoutEvent(SourceBuffer* buffer)
    {
        m_list.push_back(buffer);
    }

    void add(SourceBuffer* buffer, MediaSource* ms);
    void remove(unsigned long index);
    void remove(SourceBuffer* buffer);
    void clear();

    void detachFromParent()
    {
        m_parentMediaSource = nullptr;
    }

    SourceBuffer* operator[](size_t index)
    {
        return m_list[index];
    }

    void scheduleEvent(String* eventName);

    // DOM binding API
    SourceBuffer* defaultIndexedGetter(uint32_t idx)
    {
        return m_list[idx];
    }

protected:
    GCVector<SourceBuffer*> m_list;
    MediaSource* m_parentMediaSource;
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
