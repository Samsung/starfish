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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishTextTrackCue__)
#define __StarFishTextTrackCue__

#include "dom/EventTarget.h"
// #include "dom/DocumentFragment.h"
#include "extra/TimeRange.h"

namespace StarFish {

class TextTrack;
class TimeRange;

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

class TextTrackCueList : public ScriptWrappable,
                         public GCVector<TextTrackCue*> {
public:
    TextTrackCueList() : ScriptWrappable(this), GCVector<TextTrackCue*>()
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
};
}

#endif
