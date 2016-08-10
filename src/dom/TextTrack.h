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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishTextTrack__)
#define __StarFishTextTrack__

#include "dom/EventTarget.h"
// #include "dom/DocumentFragment.h"

namespace StarFish {

class TextTrack;

// TextTrackCue
// : Only support "startTime", "endTime"
class TextTrackCue : public EventTarget {
public:
    TextTrackCue(double start, double end, String* payload)
        : EventTarget()
        , m_textTrack(nullptr)
        , m_id(String::emptyString)
        , m_startTime(start)
        , m_endTime(end)
        , m_payload(String::emptyString)
        , m_payloadAsHTML(nullptr)
    {
        setPayload(payload);
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isVTTCue() const
    {
        return false;
    }

    String* id()
    {
        return m_id;
    }

    double startTime()
    {
        return m_startTime;
    }

    double endTime()
    {
        return m_endTime;
    }

    void setTrack(TextTrack* textTrack)
    {
        m_textTrack = textTrack;
    }

    void setId(String* id)
    {
        m_id = id;
    }

    void setStartTime(double startTime)
    {
        m_startTime = startTime;
    }

    void setEndTime(double endTime)
    {
        m_endTime = endTime;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("[TextTrackCue]\n");
        printf("    StartTime : %lf\n", m_startTime);
        printf("    EndTime : %lf\n", m_endTime);
        printf("    text : \"%s\"\n", m_payload->utf8Data());
    }
#endif

protected:
    String* getPayload()
    {
        return m_payload;
    }

    void setPayload(String* payload);

protected:
    TextTrack* m_textTrack;
    String* m_id;
    double m_startTime;
    double m_endTime;
    String* m_payload;
    DocumentFragment* m_payloadAsHTML;
};

class TextTrackCueList : public ScriptWrappable {
// TODO:
// interface TextTrackCueList {
//   readonly attribute unsigned long length;
//   getter TextTrackCue (in unsigned long index);
//   TextTrackCue getCueById(in DOMString id);
// };
public:
    TextTrackCueList()
        : ScriptWrappable(this)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    unsigned long length() const
    {
        return m_list.size();
    }

    void add(TextTrackCue* cue)
    {
        m_list.push_back(cue);
    }

    void clearAll()
    {
        m_list.clear();
    }

    TextTrackCue* at(unsigned int index)    
    {
        if (index >= m_list.size())
            return nullptr;
        return m_list[index];
    }

    TextTrackCue* getCueById(String* id)
    {
        // TODO
        return nullptr;
    }

protected:
    std::vector<TextTrackCue*, gc_allocator<TextTrackCue*>> m_list;
};

class TextTrack : public EventTarget {
public:
    enum Mode {
        Off,
        Hidden,
        Showing
    };

    enum Kind {
        Subtitles,
        Captions,
        Descriptions,
        Chapters,
        Metadata,
    };

    TextTrack()
        : EventTarget()
        , m_kind(Kind::Captions)
        , m_label(String::emptyString)
        , m_language(String::emptyString)
        , m_cues(new TextTrackCueList())
    {

    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    void addCue(TextTrackCue* cue)
    {
        cue->setTrack(this);
        m_cues->add(cue);
    }

    void removeCue(TextTrackCue* cue)
    {
        // TODO
    }

    TextTrackCueList* cues()
    {
        STARFISH_ASSERT(m_cues);
        return m_cues;
    }

protected:
    Kind m_kind;
    String* m_label;
    String* m_language;
    TextTrackCueList* m_cues;
};

class TextTrackList : public EventTarget {
public:
    TextTrackList()
        : EventTarget()
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    unsigned long length() const
    {
        return m_list.size();
    }

    void add(TextTrack* track)
    {
        m_list.push_back(track);
    }

protected:
    std::vector<TextTrack*, gc_allocator<TextTrack*>> m_list;
};

}

#endif
