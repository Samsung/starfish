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
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLDivElement.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/TextTrack.h"
#include "core/dom/TextTrackCue.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/page/Window.h"

namespace StarFish {

void TextTrackCue::dispatchEnterEvent()
{
    String* eventType = String::emptyString;
    if (m_textTrack && m_textTrack->hasTrackElement()) {
        eventType = m_textTrack->trackElement()
                        ->starFish()
                        ->staticStrings()
                        ->m_enter.localName();
    } else {
        eventType = String::fromUTF8("enter");
    }
    Event* e = new Event(document(), eventType, EventInit(false, false));
    dispatchEventByUA(e);
}

void TextTrackCue::dispatchExitEvent()
{
    String* eventType = String::emptyString;
    if (m_textTrack && m_textTrack->hasTrackElement()) {
        eventType = m_textTrack->trackElement()
                        ->starFish()
                        ->staticStrings()
                        ->m_exit.localName();
    } else {
        eventType = String::fromUTF8("exit");
    }
    Event* e = new Event(document(), eventType, EventInit(false, false));
    dispatchEventByUA(e);
}

void TextTrackCue::setPayload(String* payload)
{
    m_payload = payload;
    m_payloadAsHTML = nullptr;
}

DocumentFragment* TextTrackCue::getCueAsHTML()
{
    // TODO : parse HTML text -> DocumentFragment
    if (!m_payloadAsHTML && document()) {
        m_payloadAsHTML = document()->createDocumentFragment();
        // FIXME : HTMLParser require context element -> make dummy element here
        HTMLDivElement* dummyDiv = new HTMLDivElement(document());
        HTMLParser parser(starFish(), m_payloadAsHTML, dummyDiv, m_payload);
        parser.startParse();
        parser.parseStep();
        STARFISH_ASSERT(m_payloadAsHTML);
    }
    return m_payloadAsHTML;
}

DEFINE_EVENT_LISTENER(TextTrackCue, enter);
DEFINE_EVENT_LISTENER(TextTrackCue, exit);
}

#endif
