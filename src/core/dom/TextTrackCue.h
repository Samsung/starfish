/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_MULTIMEDIA)
#ifndef __StarFishTextTrackCue__
#define __StarFishTextTrackCue__

#include "core/dom/EventTarget.h"
#include "core/extra/TimeRange.h"

namespace StarFish {

class Document;
class DocumentFragment;
class TextTrack;

class TextTrackCue : public EventTarget {
public:
    TextTrackCue(Document* document, double start, double end, String* payload)
        : EventTarget(document)
        , m_textTrack(nullptr)
        , m_id(String::emptyString)
        , m_timeRange(TimeRange(start, end))
        , m_payload(String::emptyString)
        , m_payloadAsHTML(nullptr)
    {
        setPayload(payload);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTextTrackCue() const override;

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

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(enter);
    DECLARE_EVENT_LISTENER(exit);
#undef VIRTUAL
#undef OVERRIDE

    String* getPayload()
    {
        return m_payload;
    }

    void setPayload(String* payload);
    DocumentFragment* getCueAsHTML();

    void dispatchEnterEvent();
    void dispatchExitEvent();

protected:
    TextTrack* m_textTrack;
    String* m_id;
    TimeRange m_timeRange;
    String* m_payload;
    DocumentFragment* m_payloadAsHTML;
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
