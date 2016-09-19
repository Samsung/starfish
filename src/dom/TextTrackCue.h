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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishTextTrackCue__)
#define __StarFishTextTrackCue__

#include "dom/EventTarget.h"
// #include "dom/DocumentFragment.h"

namespace StarFish {

class TextTrack;

class TimeRange {
    friend class TimeRanges;
public:
    TimeRange(double start = 0, double end = 0)
        : m_start(start)
        , m_end(end)
    { }

    bool operator==(const TimeRange& c) const
    {
        return m_start == c.m_start && m_end == c.m_end;
    }

    bool operator!=(const TimeRange& c) const
    {
        return !this->operator==(c);
    }

    double start()
    {
        return m_start;
    }

    double end()
    {
        return m_end;
    }

    void setStart(double start)
    {
        m_start = start;
    }

    void setEnd(double end)
    {
        m_end = end;
    }

    void set(double start, double end)
    {
        setStart(start);
        setEnd(end);
    }

    bool isInRange(double pivot)
    {
        return (m_start <= pivot && m_end >= pivot);
    }

private:
    double m_start;
    double m_end;
};

class TimeRanges : public ScriptWrappable {
public:
    TimeRanges()
        : ScriptWrappable(this)
    { }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::TimeRangesObject;
    }

    double start(unsigned long idx)
    {
        if (idx < m_list.size())
            return m_list[idx].m_start;
        return DBL_MAX;
    }

    double end(unsigned long idx)
    {
        if (idx < m_list.size())
            return m_list[idx].m_end;
        return DBL_MAX;
    }

    unsigned long length()
    {
        return m_list.size();
    }

private:
    std::vector<TimeRange, gc_allocator<TimeRange>> m_list;
};

class TextTrackCue : public EventTarget {
public:
    TextTrackCue(double start, double end, String* payload)
        : EventTarget()
        , m_textTrack(nullptr)
        , m_id(String::emptyString)
        , m_timeRange(TimeRange(start, end))
        , m_payload(String::emptyString)
        , m_payloadAsHTML(nullptr)
    {
        setPayload(payload);
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::TextTrackCueObject;
    }

    virtual bool isVTTCue() const
    {
        return false;
    }

    TextTrack* track()
    {
        return m_textTrack;
    }

    String* id()
    {
        return m_id;
    }

    double startTime()
    {
        return m_timeRange.start();
    }

    double endTime()
    {
        return m_timeRange.end();
    }

    void setTrack(TextTrack* textTrack)
    {
        m_textTrack = textTrack;
    }

    void unsetTrack()
    {
        m_textTrack = nullptr;
    }

    void setId(String* id)
    {
        m_id = id;
    }

    void setStartTime(double startTime)
    {
        m_timeRange.setStart(startTime);
    }

    void setEndTime(double endTime)
    {
        m_timeRange.setEnd(endTime);
    }

    bool isActiveWhen(double time)
    {
        return m_timeRange.isInRange(time);
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("[TextTrackCue]\n");
        printf("    StartTime : %lf\n", m_timeRange.start());
        printf("    EndTime : %lf\n", m_timeRange.end());
        printf("    text : \"%s\"\n", m_payload->utf8Data());
    }
#endif

    String* getPayload()
    {
        return m_payload;
    }

    void setPayload(String* payload);
    DocumentFragment* getCueAsHTML(Document* document);

    void dispatchEnterEvent();
    void dispatchExitEvent();

protected:
    TextTrack* m_textTrack;
    String* m_id;
    TimeRange m_timeRange;
    String* m_payload;
    DocumentFragment* m_payloadAsHTML;
};

class TextTrackCueList : public ScriptWrappable {
public:
    TextTrackCueList()
        : ScriptWrappable(this)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::TextTrackCueListObject;
    }

    unsigned long length() const
    {
        return m_list.size();
    }

    void add(TextTrackCue* cue)
    {
        m_list.push_back(cue);
    }

    void remove(unsigned long index)
    {
        m_list.erase(m_list.begin() + index);
    }

    void remove(unsigned long startIdx, unsigned long endIdx)
    {
        if (startIdx > endIdx || startIdx >= m_list.size() || endIdx >= m_list.size())
            return;
        m_list.erase(m_list.begin() + startIdx, m_list.begin() + endIdx + 1);
    }

    void remove(TextTrackCue* cue)
    {
        unsigned long size = m_list.size();
        unsigned long targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            if (m_list[targetIdx] == cue)
                break;
        }
        if (targetIdx < size) {
            remove(targetIdx);
        }
    }

    void clearAll()
    {
        m_list.clear();
    }

    TextTrackCue* at(unsigned int index) const
    {
        if (index >= m_list.size())
            return nullptr;
        return m_list[index];
    }

    TextTrackCue* operator[](unsigned int index) const
    {
        return at(index);
    }

    TextTrackCue* getCueById(String* id)
    {
        // TODO
        return nullptr;
    }

protected:
    std::vector<TextTrackCue*, gc_allocator<TextTrackCue*>> m_list;
};

}

#endif
