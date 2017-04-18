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

namespace StarFish {

class SourceBufferList : public EventTarget {
public:
    SourceBufferList(StarFish* starFish, MediaSource* sb)
        : EventTarget()
        , m_starFish(starFish)
        , m_parentMediaSource(sb)
    {
    }

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

    void add(SourceBuffer* buffer, MediaSource* ms)
    {
        m_list.push_back(buffer);
        buffer->attachedToParent(ms);
        scheduleEvent(
            m_starFish->staticStrings()->m_addsourcebuffer.localName());
    }

    void remove(unsigned long index)
    {
        SourceBuffer* buf = m_list[index];
        m_list.erase(m_list.begin() + index);
        buf->detachFromParent();
        scheduleEvent(
            m_starFish->staticStrings()->m_removesourcebuffer.localName());
    }

    void remove(SourceBuffer* buffer)
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

    void clear()
    {
        m_list.clear();
        m_list.shrink_to_fit();
        scheduleEvent(
            m_starFish->staticStrings()->m_removesourcebuffer.localName());
    }

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
