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

#if STARFISH_ENABLE_MULTIMEDIA
#ifndef __StarFishSourceBufferList__
#define __StarFishSourceBufferList__

#include "dom/EventTarget.h"

namespace StarFish {

class MediaSource;
class SourceBuffer;

class SourceBufferList : public EventTarget {
public:
    SourceBufferList(Document* document, MediaSource* sb);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isSourceBufferList() const override
    {
        return true;
    }

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

protected:
    GCVector<SourceBuffer*> m_list;
    StarFish* m_starFish;
    MediaSource* m_parentMediaSource;
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
