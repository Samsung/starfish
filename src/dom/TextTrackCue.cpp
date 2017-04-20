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

#ifdef STARFISH_ENABLE_MULTIMEDIA

#include "StarFish.h"
#include "dom/Document.h"
#include "dom/Event.h"
#include "dom/HTMLDivElement.h"
#include "dom/HTMLTrackElement.h"
#include "dom/TextTrack.h"
#include "dom/TextTrackCue.h"
#include "dom/parser/HTMLParser.h"
#include "platform/window/Window.h"

namespace StarFish {

void TextTrackCue::dispatchEnterEvent()
{
    String* eventType = String::emptyString;
    if (m_textTrack && m_textTrack->hasTrackElement()) {
        eventType = m_textTrack->trackElement()
                        ->document()
                        ->window()
                        ->starFish()
                        ->staticStrings()
                        ->m_enter.localName();
    } else {
        eventType = String::fromUTF8("enter");
    }
    Event* e = new Event(eventType, EventInit(false, false));
    dispatchEvent(e);
}

void TextTrackCue::dispatchExitEvent()
{
    String* eventType = String::emptyString;
    if (m_textTrack && m_textTrack->hasTrackElement()) {
        eventType = m_textTrack->trackElement()
                        ->document()
                        ->window()
                        ->starFish()
                        ->staticStrings()
                        ->m_exit.localName();
    } else {
        eventType = String::fromUTF8("exit");
    }
    Event* e = new Event(eventType, EventInit(false, false));
    dispatchEvent(e);
}

void TextTrackCue::setPayload(String* payload)
{
    m_payload = payload;
    m_payloadAsHTML = nullptr;
}

DocumentFragment* TextTrackCue::getCueAsHTML(Document* document)
{
    // TODO : parse HTML text -> DocumentFragment
    if (!m_payloadAsHTML && document) {
        m_payloadAsHTML = document->createDocumentFragment();
        // FIXME : HTMLParser require context element -> make dummy element here
        HTMLDivElement* dummyDiv = new HTMLDivElement(document);
        HTMLParser parser(document->window()->starFish(), m_payloadAsHTML,
                          dummyDiv, m_payload);
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
