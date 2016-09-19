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

#define TEXTTRACK_INVALID_TIMEVALUE -1

class TextTrack : public EventTarget {
public:
    enum Mode {
        InvalidMode,
        Off,
        Hidden,
        Showing
    };

    enum Kind {
        InvalidKind,
        Subtitles,
        Captions,
        Descriptions,
        Chapters,
        Metadata,
    };

    TextTrack(Kind kind = Kind::Captions, String* label = String::emptyString, String* language = String::emptyString)
        : EventTarget()
        , m_mode(Mode::Off)
        , m_kind(kind)
        , m_label(label)
        , m_language(language)
        , m_cues(new TextTrackCueList())
        , m_activeCues(new TextTrackCueList())
        , m_trackElement(nullptr)
        , m_cachedTime(TEXTTRACK_INVALID_TIMEVALUE)
        , m_cachedIdx(0)
    {

    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::TextTrackObject;
    }

    void dispatchCueChangeEvent();

    void addCue(TextTrackCue* cue)
    {
        cue->setTrack(this);
        m_cues->add(cue);
    }

    void removeCue(TextTrackCue* cue)
    {
        cue->unsetTrack();
        m_cues->remove(cue);
    }

    // void addActiveCue(TextTrackCue* cue)
    // {
    //     STARFISH_ASSERT(cue);
    //     m_activeCues->add(cue);
    //     cue->dispatchEnterEvent();
    //     dispatchCueChangeEvent();
    // }

    // void removeActiveCue(TextTrackCue* cue)
    // {
    //     STARFISH_ASSERT(cue);
    //     m_activeCues->remove(cue);
    //     cue->dispatchExitEvent();
    //     dispatchCueChangeEvent();
    // }

    TextTrackCueList* cues()
    {
        STARFISH_ASSERT(m_cues);
        if (m_mode == Mode::Off)
            return nullptr;
        return m_cues;
    }

    TextTrackCueList* activeCues()
    {
        STARFISH_ASSERT(m_activeCues);
        if (m_mode == Mode::Off)
            return nullptr;
        return m_activeCues;
    }

    Kind kind()
    {
        return m_kind;
    }

    String* label()
    {
        return m_label;
    }

    String* language()
    {
        return m_language;
    }

    Mode mode()
    {
        return m_mode;
    }

    HTMLTrackElement* trackElement()
    {
        return m_trackElement;
    }

    String* id();

    void setKind(String* kind);
    void setKind(Kind kind)
    {
        m_kind = kind;
    }

    void setLabel(String* label)
    {
        m_label = label;
    }

    void setLanguage(String* language)
    {
        m_language = language;
    }

    void setMode(String* mode);
    void setMode(Mode mode)
    {
        m_mode = mode;
    }

    void setTrackElement(HTMLTrackElement* element)
    {
        m_trackElement = element;
    }

    bool hasTrackElement()
    {
        return m_trackElement != nullptr;
    }

    void clear()
    {
        STARFISH_ASSERT(m_cues);
        m_cues->clearAll();
        m_activeCues->clearAll();
        m_cachedTime = TEXTTRACK_INVALID_TIMEVALUE;
        m_cachedIdx = 0;
    }

    static Kind stringToKind(String* kindStr)
    {
        if (kindStr && kindStr->length() > 7) {
            switch (kindStr->charAt(2)) {
            case 'a':
            case 'A':
                if (kindStr->equalsWithoutCase(String::fromUTF8("chapters")))
                    return Kind::Chapters;
            case 'b':
            case 'B':
                if (kindStr->equalsWithoutCase(String::fromUTF8("subtitles")))
                    return Kind::Subtitles;
            case 'p':
            case 'P':
                if (kindStr->equalsWithoutCase(String::fromUTF8("captions")))
                    return Kind::Captions;
            case 's':
            case 'S':
                if (kindStr->equalsWithoutCase(String::fromUTF8("descriptions")))
                    return Kind::Descriptions;
            case 't':
            case 'T':
                if (kindStr->equalsWithoutCase(String::fromUTF8("metadata")))
                    return Kind::Metadata;
            default:
                return Kind::InvalidKind;
            }
        }
        return Kind::InvalidKind;
    }

    static String* kindToString(Kind kindEnum)
    {
        switch (kindEnum) {
        case Kind::Subtitles:
            return String::fromUTF8("subtitles");
        case Kind::Captions:
            return String::fromUTF8("captions");
        case Kind::Descriptions:
            return String::fromUTF8("descriptions");
        case Kind::Chapters:
            return String::fromUTF8("chapters");
        case Kind::Metadata:
            return String::fromUTF8("metadata");
        default:
            return String::emptyString;
        }
        return String::emptyString;
    }

    static Mode stringToMode(String* modeStr)
    {
        if (modeStr && modeStr->length() > 5) {
            if (modeStr->equals("disabled")) {
                return Mode::Off;
            } else if (modeStr->equals("hidden")) {
                return Mode::Hidden;
            } else if (modeStr->equals("showing")) {
                return Mode::Showing;
            }
        }
        return Mode::InvalidMode;
    }

    static String* modeToString(Mode modeEnum)
    {
        switch (modeEnum) {
        case Mode::Off:
            return String::fromUTF8("disabled");
        case Mode::Hidden:
            return String::fromUTF8("hidden");
        case Mode::Showing:
            return String::fromUTF8("showing");
        default:
            return String::emptyString;
        }
        return String::emptyString;
    }

    TextTrackCueList* updateActiveCues(double time);

protected:
    Mode m_mode;
    Kind m_kind;
    String* m_label;
    String* m_language;
    TextTrackCueList* m_cues;
    TextTrackCueList* m_activeCues;
    HTMLTrackElement* m_trackElement;
    double m_cachedTime;
    unsigned long m_cachedIdx;
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

    virtual Type type()
    {
        return ScriptWrappable::Type::TextTrackListObject;
    }

    unsigned long length() const
    {
        return m_list.size();
    }

    void add(TextTrack* track)
    {
        m_list.push_back(track);
    }

    void remove(unsigned long index)
    {
        m_list.erase(m_list.begin() + index);
    }

    void remove(TextTrack* track)
    {
        unsigned long size = m_list.size();
        unsigned long targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            if (m_list[targetIdx] == track)
                break;
        }
        if (targetIdx < size) {
            remove(targetIdx);
        }
    }

    TextTrack* at(unsigned long index)
    {
        if (index >= m_list.size())
            return nullptr;
        return m_list[index];
    }

    TextTrack* getTrackById(String* id)
    {
        unsigned long size = m_list.size();
        unsigned long targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            if (m_list[targetIdx]->id()->equals(id))
                break;
        }
        if (targetIdx < size) {
            return m_list[targetIdx];
        }
        return nullptr;
    }

protected:
    std::vector<TextTrack*, gc_allocator<TextTrack*>> m_list;
};

}

#endif
