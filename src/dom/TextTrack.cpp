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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFishConfig.h"
#include "TextTrack.h"
#include "Event.h"
#include "Document.h"
#include "HTMLTrackElement.h"

namespace StarFish {

void TextTrack::dispatchCueChangeEvent()
{
    String* eventType = String::emptyString;
    if (m_trackElement) {
        eventType = m_trackElement->document()
                        ->window()
                        ->starFish()
                        ->staticStrings()
                        ->m_cuechange.localName();
    } else {
        eventType = String::fromUTF8("cuechange");
    }
    Event* e = new Event(eventType, EventInit(false, false));
    dispatchEvent(e);
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
    if (!modeEnum == TextTrack::Mode::InvalidMode) {
        m_mode = modeEnum;
    }
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

    if (m_cues->length() < 1) {
        return activeCues();
    }

    if (m_cachedTime != TEXTTRACK_INVALID_TIMEVALUE && m_cachedTime == time) {
        return activeCues();
    }

    if (m_cachedTime > time || m_cachedTime == TEXTTRACK_INVALID_TIMEVALUE) {
        m_cachedIdx = 0;
        m_activeCues->clearAll();
    }
    m_cachedTime = time;

    bool cueListChanged = false;
    unsigned long startIdx = 0, endIdx;
    unsigned long activesSize = m_activeCues->length();
    for (endIdx = 0; endIdx < activesSize; endIdx++) {
        TextTrackCue* cue = m_activeCues->at(endIdx);
        if (cue->isActiveWhen(time)) {
            break;
        } else {
            cue->dispatchExitEvent();
        }
    }
    endIdx--;
    if (startIdx <= endIdx) {
        m_activeCues->remove(startIdx, endIdx);
        cueListChanged = true;
    }

    unsigned long cuesSize = m_cues->length();
    for (; m_cachedIdx < cuesSize; m_cachedIdx++) {
        TextTrackCue* cue = m_cues->at(m_cachedIdx);
        if (cue->isActiveWhen(time)) {
            m_activeCues->add(cue);
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
