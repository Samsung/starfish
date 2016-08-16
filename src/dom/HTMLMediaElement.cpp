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
#include "dom/HTMLMediaElement.h"
#include "dom/HTMLTrackElement.h"
#include "dom/TextTrack.h"

namespace StarFish {

HTMLMediaElement::HTMLMediaElement(Document* document)
    : HTMLElement(document)
    , m_textTracks(new TextTrackList())
{
}

void HTMLMediaElement::addTextTrack(TextTrack* track)
{
    if (track) {
        m_textTracks->add(track);
    }
}

void HTMLMediaElement::removeTextTrack(TextTrack* track)
{
    if (track) {
        m_textTracks->remove(track);
    }
}

TextTrack* HTMLMediaElement::addTextTrack(String* kind, String* label, String* language)
{
    TextTrack::Kind kindEnum = TextTrack::stringToKind(kind);
    if (kindEnum == TextTrack::Kind::InvalidKind) {
        return nullptr;
    }
    TextTrack* textTrack = new TextTrack(kindEnum, label, language);
    m_textTracks->add(textTrack);
    return textTrack;
}

void HTMLMediaElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    if (parent == this && newChild->isElement() && newChild->asElement()->isHTMLElement() && newChild->asElement()->asHTMLElement()->isHTMLTrackElement()) {
        HTMLTrackElement* trackElement = newChild->asElement()->asHTMLElement()->asHTMLTrackElement();
        STARFISH_ASSERT(trackElement->track());
        addTextTrack(trackElement->track());
    }
}

void HTMLMediaElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    HTMLElement::didNodeRemoved(parent, oldChild);
    if (parent == this && oldChild->isElement() && oldChild->asElement()->isHTMLElement() && oldChild->asElement()->asHTMLElement()->isHTMLTrackElement()) {
        HTMLTrackElement* trackElement = oldChild->asElement()->asHTMLElement()->asHTMLTrackElement();
        STARFISH_ASSERT(trackElement->track());
        removeTextTrack(trackElement->track());
    }
}

HTMLMediaElement::NetState HTMLMediaElement::networkState()
{
    // TODO
    return HTMLMediaElement::NETWORK_EMPTY;
}

String* HTMLMediaElement::preload()
{
    // TODO
    return String::emptyString;
}

TimeRanges* HTMLMediaElement::buffered()
{
    // TODO
    return nullptr;
}

void HTMLMediaElement::load()
{
    // TODO
}

String* HTMLMediaElement::canPlayType(String* type)
{
    // TODO
    return String::emptyString;
}

HTMLMediaElement::ReadyState HTMLMediaElement::readyState()
{
    // TODO
    return HTMLMediaElement::HAVE_NOTHING;
}

bool HTMLMediaElement::seeking()
{
    // TODO
    return false;
}

double HTMLMediaElement::currentTime()
{
    // TODO
    return 0;
}

double HTMLMediaElement::duration()
{
    // TODO
    return 0;
}

bool HTMLMediaElement::paused()
{
    // TODO
    return 0;
}

void HTMLMediaElement::pause()
{
    // TODO
}

double HTMLMediaElement::defaultPlaybackRate()
{
    // TODO
    return 0;
}

double HTMLMediaElement::playbackRate()
{
    // TODO
    return 0;
}

TimeRanges* HTMLMediaElement::played()
{
    // TODO
    return nullptr;
}

TimeRanges* HTMLMediaElement::seekable()
{
    // TODO
    return nullptr;
}

bool HTMLMediaElement::ended()
{
    // TODO
    return false;
}

bool HTMLMediaElement::autoplay()
{
    // TODO
    return false;
}

bool HTMLMediaElement::loop()
{
    // TODO
    return false;
}

bool HTMLMediaElement::controls()
{
    // TODO
    return false;
}

bool HTMLMediaElement::volume()
{
    // TODO
    return false;
}

bool HTMLMediaElement::muted()
{
    // TODO
    return false;
}

void HTMLMediaElement::setPreload(String* preload)
{
    // TODO
}

void HTMLMediaElement::setSeeking(bool seeking)
{
    // TODO
}

void HTMLMediaElement::setCurrentTime(double currentTime)
{
    // TODO
}

void HTMLMediaElement::setDefaultPlaybackRate(double defaultPlaybackRate)
{
    // TODO
}

void HTMLMediaElement::setPlaybackRate(double playbackRate)
{
    // TODO
}

void HTMLMediaElement::setAutoplay(bool autoplay)
{
    // TODO
}

void HTMLMediaElement::setLoop(bool loop)
{
    // TODO
}

void HTMLMediaElement::setControls(bool controls)
{
    // TODO
}

void HTMLMediaElement::setVolume(bool volume)
{
    // TODO
}

void HTMLMediaElement::setMuted(bool muted)
{
    // TODO
}

}

#endif
