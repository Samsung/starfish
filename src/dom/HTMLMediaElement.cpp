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
#include "util/URL.h"

namespace StarFish {

HTMLMediaElement::HTMLMediaElement(Document* document)
    : HTMLElement(document)
    , m_mediaPlayer(nullptr)
    , m_textTracks(new TextTrackList())
    , m_readyState(HTMLMediaElement::HAVE_NOTHING)
{
}

void HTMLMediaElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        if (!m_mediaPlayer) {
            return;
        }
        URL* url = nullptr;
        if (value->length() != 0) {
            url = URL::createURL(document()->documentURI()->urlString(), value);
        }
        m_mediaPlayer->setURL(url);
        if (preloadEnum() != HTMLMediaElement::PRELOAD_NONE || autoplay()) {
            load();
        }
    }
}

void HTMLMediaElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    if (!m_mediaPlayer) {
        return;
    }
    if (autoplay()) {
        m_mediaPlayer->play();
    }
}

void HTMLMediaElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    if (!m_mediaPlayer) {
        return;
    }
    m_mediaPlayer->pause();
}

void HTMLMediaElement::load()
{
    if (!m_mediaPlayer) {
        return;
    }
    m_mediaPlayer->prepare();
}

void HTMLMediaElement::play()
{
    if (!m_mediaPlayer) {
        return;
    }
    m_mediaPlayer->play();
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

HTMLMediaElement::PreloadState HTMLMediaElement::preloadEnum()
{
    QualifiedName preload = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "preload"));
    size_t siz = hasAttribute(preload);
    if (siz != SIZE_MAX) {
        String* value = getAttribute(preload);
        if (value->length() == 0 || (value->length() == 4 && value->equalsWithoutCase(AtomicString::createAtomicString(document()->window()->starFish(), "auto").string()))) {
            // The empty string is also a valid keyword, and maps to the Automatic state
            return HTMLMediaElement::PRELOAD_AUTOMATIC;
        } else if (value->length() == 8 && value->equalsWithoutCase(AtomicString::createAtomicString(document()->window()->starFish(), "metadata").string())) {
            return HTMLMediaElement::PRELOAD_METADATA;
        } else if (value->length() == 4 && value->equalsWithoutCase(AtomicString::createAtomicString(document()->window()->starFish(), "none").string())) {
            return HTMLMediaElement::PRELOAD_NONE;
        }
    }
    // Default : Automatic
    return HTMLMediaElement::PRELOAD_AUTOMATIC;
}

String* HTMLMediaElement::preload()
{
    return HTMLMediaElement::preloadToString(document()->window()->starFish(), preloadEnum());
}

TimeRanges* HTMLMediaElement::buffered()
{
    // TODO
    return nullptr;
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
    if (!m_mediaPlayer)
        return false;
    return m_mediaPlayer->isPublicState(MediaPlayer::STATE_PAUSED);
}

void HTMLMediaElement::pause()
{
    if (!m_mediaPlayer)
        return;
    m_mediaPlayer->pause();
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
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "autoplay"));
    size_t siz = hasAttribute(name);
    if (siz == SIZE_MAX)
        return false;
    return true;
}

bool HTMLMediaElement::loop()
{
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "loop"));
    size_t siz = hasAttribute(name);
    if (siz == SIZE_MAX)
        return false;
    return true;
}

bool HTMLMediaElement::controls()
{
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "controls"));
    size_t siz = hasAttribute(name);
    if (siz == SIZE_MAX)
        return false;
    return true;
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
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "preload"));
    setAttribute(name, preload);
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
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "autoplay"));
    if (autoplay) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::fromUTF8(""));
        }
    } else {
        removeAttribute(name);
    }
}

void HTMLMediaElement::setLoop(bool loop)
{
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "loop"));
    if (loop) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::fromUTF8(""));
        }
    } else {
        removeAttribute(name);
    }
    m_mediaPlayer->setLoop(loop);
}

void HTMLMediaElement::setControls(bool controls)
{
    QualifiedName name = QualifiedName(AtomicString::emptyAtomicString(), AtomicString::createAtomicString(document()->window()->starFish(), "controls"));
    if (controls) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::fromUTF8(""));
        }
    } else {
        removeAttribute(name);
    }
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
