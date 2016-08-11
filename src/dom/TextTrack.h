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

    TextTrack()
        : EventTarget()
        , m_mode(Mode::Off)
        , m_kind(Kind::Captions)
        , m_label(String::emptyString)
        , m_language(String::emptyString)
        , m_cues(new TextTrackCueList())
    {
    }

    TextTrack(Kind kind, String* label, String* language)
        : EventTarget()
        , m_mode(Mode::Off)
        , m_kind(kind)
        , m_label(label)
        , m_language(language)
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
        if (m_mode == Mode::Off)
            return nullptr;
        return m_cues;
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

    void clearCues()
    {
        STARFISH_ASSERT(m_cues);
        m_cues->clearAll();
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
        if (modeStr && modeStr->length() > 0) {
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

protected:
    Mode m_mode;
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

    TextTrack* at(unsigned int index)
    {
        if (index >= m_list.size())
            return nullptr;
        return m_list[index];
    }

protected:
    std::vector<TextTrack*, gc_allocator<TextTrack*>> m_list;
};

}

#endif
