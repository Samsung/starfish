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

#include "dom/VTTCue.h"

namespace StarFish {

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
