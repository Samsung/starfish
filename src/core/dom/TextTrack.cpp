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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Event.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/TextTrack.h"
#include "core/dom/TextTrackCue.h"
#include "core/dom/TextTrackCueList.h"
#include "core/page/Window.h"

namespace StarFish {

TextTrack::TextTrack(Document* document, Kind kind, String* label,
                     String* language)
    : EventTarget(document)
    , m_mode(Mode::Off)
    , m_kind(kind)
    , m_label(label)
    , m_language(language)
    , m_cues(new TextTrackCueList(document))
    , m_activeCues(new TextTrackCueList(document))
    , m_trackElement(nullptr)
    , m_cachedTime(TEXTTRACK_INVALID_TIMEVALUE)
    , m_cachedIdx(0)
{
}

DEFINE_EVENT_LISTENER(TextTrack, cuechange);

void TextTrack::dispatchCueChangeEvent()
{
    String* eventType = String::emptyString;
    if (m_trackElement) {
        eventType = m_trackElement->starFish()
                        ->staticStrings()
                        ->m_cuechange.localName();
    } else {
        eventType = String::fromUTF8("cuechange");
    }
    Event* e = new Event(document(), eventType, EventInit(false, false));
    dispatchEventByUA(e);
}

void TextTrack::addCue(TextTrackCue* cue)
{
    cue->setTrack(this);
    m_cues->push_back(cue);
}

void TextTrack::removeCue(TextTrackCue* cue)
{
    cue->unsetTrack();
    m_cues->erase(std::remove(m_cues->begin(), m_cues->end(), cue),
                  m_cues->end());
}

void TextTrack::setKind(String* kind)
{
    TextTrack::Kind kindEnum = TextTrack::stringToKind(kind);
    if (kindEnum != TextTrack::Kind::InvalidKind) {
        m_kind = TextTrack::Metadata;
    } else {
        m_kind = kindEnum;
    }
}

void TextTrack::setMode(String* mode)
{
    TextTrack::Mode modeEnum = TextTrack::stringToMode(mode);
    if (modeEnum != TextTrack::Mode::InvalidMode) {
        m_mode = modeEnum;
    }
}

void TextTrack::clear()
{
    STARFISH_ASSERT(m_cues);
    m_cues->clear();
    m_activeCues->clear();
    m_cachedTime = TEXTTRACK_INVALID_TIMEVALUE;
    m_cachedIdx = 0;
}

String* TextTrack::id()
{
    if (m_trackElement) {
        return m_trackElement->id();
    }
    return String::emptyString;
}

TextTrackCueList* TextTrack::updateActiveCues(double time)
{
    // FIXME!!
    // Current algorithm assumes,,
    // 1) m_cues has been sorted in order of start_time.
    // 2) Every cue's end_time never exceed next cue's end_time. -> FIFO

    if (m_cues->size() < 1) {
        return activeCues();
    }

    if (m_cachedTime != TEXTTRACK_INVALID_TIMEVALUE && m_cachedTime == time) {
        return activeCues();
    }

    if (m_cachedTime > time || m_cachedTime == TEXTTRACK_INVALID_TIMEVALUE) {
        m_cachedIdx = 0;
        m_activeCues->clear();
    }
    m_cachedTime = time;

    bool cueListChanged = false;
    unsigned long startIdx = 0, endIdx;
    unsigned long activesSize = m_activeCues->size();
    for (endIdx = 0; endIdx < activesSize; endIdx++) {
        TextTrackCue* cue = (*m_activeCues)[endIdx];
        if (cue->isActiveWhen(time)) {
            break;
        } else {
            cue->dispatchExitEvent();
        }
    }
    endIdx--;
    if (startIdx <= endIdx) {
        m_activeCues->erase(m_activeCues->begin() + startIdx,
                            m_activeCues->begin() + endIdx + 1);
        cueListChanged = true;
    }

    unsigned long cuesSize = m_cues->size();
    for (; m_cachedIdx < cuesSize; m_cachedIdx++) {
        TextTrackCue* cue = (*m_cues)[m_cachedIdx];
        if (cue->isActiveWhen(time)) {
            m_activeCues->push_back(cue);
            cue->dispatchEnterEvent();
            cueListChanged = true;
        } else {
            break;
        }
    }

    if (cueListChanged) {
        dispatchCueChangeEvent();
    }

    return activeCues();
}
}

#endif
