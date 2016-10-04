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
#include "platform/multimedia/MediaPlayer.h"
#include "platform/message_loop/MessageLoop.h"

namespace StarFish {

HTMLMediaElement::HTMLMediaElement(Document* document)
    : HTMLElement(document)
    , m_mediaPlayer(MediaPlayer::create(this))
    , m_textTracks(new TextTrackList())
    , m_readyState(HTMLMediaElement::HAVE_NOTHING)
{
}

void HTMLMediaElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);

    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        if (value->length() != 0) {
            m_mediaPlayer->setURL(URL::createURL(document()->documentURI()->urlString(), value));
            if (autoplay()) {
                play();
            }
        } else {
            m_mediaPlayer->setURL(nullptr);
        }
    } else if (name == document()->window()->starFish()->staticStrings()->m_loop) {
        if (attributeRemoved)
            m_mediaPlayer->setLoop(false);
        else
            m_mediaPlayer->setLoop(true);
    }
}

void HTMLMediaElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    if (autoplay()) {
        m_mediaPlayer->play();
    }
}

void HTMLMediaElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
    m_mediaPlayer->pause();
}

void HTMLMediaElement::load()
{
    m_mediaPlayer->prepare();
}

void HTMLMediaElement::play()
{
    // TODO return promise object
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

HTMLMediaElement::PreloadState HTMLMediaElement::preloadEnum()
{
    QualifiedName preload = document()->window()->starFish()->staticStrings()->m_preload;
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

bool HTMLMediaElement::seeking()
{
    // TODO
    return false;
}

double HTMLMediaElement::currentTime()
{
    return m_mediaPlayer->currentTime();
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
    return m_mediaPlayer->playbackState() != MediaPlayer::PLAYBACK_STATE_PLAYING;
}

void HTMLMediaElement::pause()
{
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
    size_t siz = hasAttribute(document()->window()->starFish()->staticStrings()->m_autoplay);
    if (siz == SIZE_MAX)
        return false;
    return true;
}

bool HTMLMediaElement::loop()
{
    size_t siz = hasAttribute(document()->window()->starFish()->staticStrings()->m_loop);
    if (siz == SIZE_MAX)
        return false;
    return true;
}

bool HTMLMediaElement::controls()
{
    size_t siz = hasAttribute(document()->window()->starFish()->staticStrings()->m_controls);
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

String* HTMLMediaElement::currentSrc()
{
    STARFISH_ASSERT(m_mediaPlayer);
    return m_mediaPlayer->url() ? m_mediaPlayer->url()->urlString() : String::emptyString;
}

void HTMLMediaElement::setPreload(String* preload)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_preload, preload);
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
    QualifiedName name = document()->window()->starFish()->staticStrings()->m_autoplay;
    if (autoplay) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::emptyString);
        }
    } else {
        removeAttribute(name);
    }
}

void HTMLMediaElement::setLoop(bool loop)
{
    QualifiedName name = document()->window()->starFish()->staticStrings()->m_loop;
    if (loop) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::emptyString);
        }
    } else {
        removeAttribute(name);
    }
}

void HTMLMediaElement::setControls(bool controls)
{
    QualifiedName name = document()->window()->starFish()->staticStrings()->m_controls;
    if (controls) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::emptyString);
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

HTMLMediaElement::ReadyState HTMLMediaElement::readyState()
{
    // TODO
    return HTMLMediaElement::HAVE_NOTHING;
}

void HTMLMediaElement::updateReadyState(HTMLMediaElement::ReadyState state)
{
    if (state == m_readyState) {
        return;
    }
    if (networkState() != HTMLMediaElement::NETWORK_EMPTY) {
        HTMLMediaElement::ReadyState prevState = m_readyState;
        if (prevState == HTMLMediaElement::HAVE_NOTHING && state == HTMLMediaElement::HAVE_METADATA) {
            dispatchLoadedmetadataEvent();
        }
        if (prevState == HTMLMediaElement::HAVE_METADATA && state >= HTMLMediaElement::HAVE_CURRENT_DATA) {
            dispatchLoadeddataEvent();
        }
        if (prevState >= HTMLMediaElement::HAVE_FUTURE_DATA && state <= HTMLMediaElement::HAVE_CURRENT_DATA) {
            if (m_mediaPlayer && m_mediaPlayer->isPlaybackState(MediaPlayer::PLAYBACK_STATE_PLAYING)) {
                dispatchTimeupdateEvent();
                dispatchWaitingEvent();
            }
        }
        if (prevState <= HTMLMediaElement::HAVE_CURRENT_DATA && state >= HTMLMediaElement::HAVE_FUTURE_DATA) {
            dispatchCanplayEvent();
            if (m_mediaPlayer && m_mediaPlayer->isPlaybackState(MediaPlayer::PLAYBACK_STATE_PLAYING)) {
                dispatchPlayingEvent();
            }
        }
        if (state == HTMLMediaElement::HAVE_ENOUGH_DATA) {
            dispatchCanplaythroughEvent();
            // if(autoplay() && m_mediaPlayer && m_mediaPlayer->isState(MediaPlayer::STATE_PAUSED)
        }
    }
    m_readyState = state;
}

HTMLMediaElement::NetState HTMLMediaElement::networkState()
{
    // NOTE: We do not have aync resource selecting, so no NETWORK_NO_SOURCE state
    if (currentSrc() == String::emptyString) {
        return NetState::NETWORK_EMPTY;
    }
    MediaPlayer::LoadState dState = m_mediaPlayer->loadState();
    if (dState == MediaPlayer::LOAD_STATE_NONE) {
        return NetState::NETWORK_IDLE;
    }
    return NetState::NETWORK_LOADING;
}

#define ADD_DISPATCH_EVENT_DEF(name, Name) \
void HTMLMediaElement::dispatch##Name##EventNow() \
{ \
    String* eventType = document()->window()->starFish()->staticStrings()->m_##name.localName(); \
    Event* e = new Event(eventType, EventInit(false, false)); \
    dispatchEvent(e); \
} \
void HTMLMediaElement::dispatch##Name##Event() \
{ \
    document()->window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) { \
        HTMLMediaElement* element = (HTMLMediaElement*)data; \
        element->dispatch##Name##EventNow(); \
    }, this); \
}
ADD_DISPATCH_EVENT_DEF(progress, Progress);
ADD_DISPATCH_EVENT_DEF(suspend, Suspend);
ADD_DISPATCH_EVENT_DEF(abort, Abort);
ADD_DISPATCH_EVENT_DEF(error, Error);
ADD_DISPATCH_EVENT_DEF(emptied, Emptied);
ADD_DISPATCH_EVENT_DEF(stalled, Stalled);
ADD_DISPATCH_EVENT_DEF(loadedmetadata, Loadedmetadata);
ADD_DISPATCH_EVENT_DEF(loadeddata, Loadeddata);
ADD_DISPATCH_EVENT_DEF(canplay, Canplay);
ADD_DISPATCH_EVENT_DEF(canplaythrough, Canplaythrough);
ADD_DISPATCH_EVENT_DEF(playing, Playing);
ADD_DISPATCH_EVENT_DEF(waiting, Waiting);
ADD_DISPATCH_EVENT_DEF(seeking, Seeking);
ADD_DISPATCH_EVENT_DEF(seeked, Seeked);
ADD_DISPATCH_EVENT_DEF(ended, Ended);
ADD_DISPATCH_EVENT_DEF(durationchange, Durationchange);
ADD_DISPATCH_EVENT_DEF(timeupdate, Timeupdate);
ADD_DISPATCH_EVENT_DEF(play, Play);
ADD_DISPATCH_EVENT_DEF(pause, Pause);
ADD_DISPATCH_EVENT_DEF(ratechange, Ratechange);
ADD_DISPATCH_EVENT_DEF(volumechange, Volumechange);
#undef ADD_DISPATCH_EVENT_DEF

}

#endif
