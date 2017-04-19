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

#if defined(STARFISH_ENABLE_MULTIMEDIA)
#ifndef __StarFishTextTrack__
#define __StarFishTextTrack__

#include "dom/EventTarget.h"

namespace StarFish {

class HTMLTrackElement;
class TextTrackCueList;

#define TEXTTRACK_INVALID_TIMEVALUE -1

class TextTrack : public EventTarget {
public:
    enum Mode {
        InvalidMode,
        Off,
        Hidden,
        Showing,
    };

    enum Kind {
        InvalidKind,
        Subtitles,
        Captions,
        Descriptions,
        Chapters,
        Metadata,
    };

    TextTrack(Document* document, Kind kind = Kind::Subtitles,
              String* label = String::emptyString,
              String* language = String::emptyString);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isTextTrack() const override
    {
        return true;
    }

    void dispatchCueChangeEvent();

    void addCue(TextTrackCue* cue);

    void removeCue(TextTrackCue* cue);

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
        if (m_mode == Mode::Off) {
            return nullptr;
        }
        return m_cues;
    }

    TextTrackCueList* activeCues()
    {
        STARFISH_ASSERT(m_activeCues);
        if (m_mode == Mode::Off) {
            return nullptr;
        }
        return m_activeCues;
    }

    Kind kind()
    {
        STARFISH_ASSERT(m_kind != TextTrack::Kind::InvalidKind);
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

    void clear();

    static Kind stringToKind(String* kindStr)
    {
        if (kindStr && kindStr->length() > 7) {
            switch (kindStr->charAt(2)) {
            case 'a':
            case 'A':
                if (kindStr->equalsWithoutCase(String::fromUTF8("chapters"))) {
                    return Kind::Chapters;
                }
            case 'b':
            case 'B':
                if (kindStr->equalsWithoutCase(String::fromUTF8("subtitles"))) {
                    return Kind::Subtitles;
                }
            case 'p':
            case 'P':
                if (kindStr->equalsWithoutCase(String::fromUTF8("captions"))) {
                    return Kind::Captions;
                }
            case 's':
            case 'S':
                if (kindStr->equalsWithoutCase(
                        String::fromUTF8("descriptions"))) {
                    return Kind::Descriptions;
                }
            case 't':
            case 'T':
                if (kindStr->equalsWithoutCase(String::fromUTF8("metadata"))) {
                    return Kind::Metadata;
                }
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
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
