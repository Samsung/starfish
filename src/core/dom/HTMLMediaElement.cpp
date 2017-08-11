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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLMediaElement.h"
#include "core/dom/HTMLSourceElement.h"
#include "core/dom/HTMLTrackElement.h"
#include "core/dom/TextTrack.h"
#include "core/dom/TextTrackList.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/extra/MimeType.h"
#include "core/modules/mediasource/SourceBuffer.h"
#include "core/modules/mediasource/SourceBufferList.h"
#include "core/extra/TimeRanges.h"
#include "core/util/URL.h"
#include "platform/multimedia/MediaPlayer.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/Window.h"

namespace StarFish {

HTMLMediaElement::HTMLMediaElement(Document* document)
    : HTMLElement(document)
    , m_autoplayingFlag(true)
    , m_isPaused(true)
    , m_isSeeking(false)
    , m_isEnded(false)
    , m_delayingTheLoadEvent(false)
    , m_officialPlaybackPosition(0)
    , m_defaultPlaybackStartPosition(0)
    , m_muted(false)
    , m_volume(1.0)
    , m_pendingSeek(std::numeric_limits<double>::quiet_NaN())
    , m_mediaPlayer(nullptr)
    , m_currentSrc(String::emptyString)
    , m_textTracks(new TextTrackList(document))
    , m_readyState(HTMLMediaElement::HAVE_NOTHING)
    , m_networkState(NetworkState::NETWORK_EMPTY)
    , m_currentOperation(nullptr)
    , m_currentPendingOperationCount(0)
    , m_currentPendingOperationHandle(SIZE_MAX)
    , m_resourceSelectionContext(nullptr)
{
}

void* HTMLMediaElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLMediaElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMediaElement, m_mediaPlayer));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMediaElement, m_currentSrc));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMediaElement, m_textTracks));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMediaElement, m_currentOperation));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLMediaElement, m_operationQueue));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLMediaElement, m_playOperationQueue));
        GC_set_bit(
            desc, GC_WORD_OFFSET(HTMLMediaElement, m_resourceSelectionContext));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLMediaElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLMediaElement::onDOMContentLoaded()
{
    if (!document()->inParsing() &&
        (autoplay() || preloadValue() != HTMLMediaElement::PRELOAD_NONE)) {
        // STARFISH_LOG_INFO("HTMLMediaElement::onDOMContentLoaded() causes
        // content load (autoplay:%s, preload:%s)\n", autoplay() ? "true" :
        // "false", preload()->utf8Data());
        load();
        if (autoplay()) {
            appendToPlayOperationQueue(
                new MediaOperationQueueDataRequestPlay(this));
        }
    }
}

void HTMLMediaElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (name == starFish()->staticStrings()->m_src) {
        if (!document()->inParsing() &&
            (autoplay() || preloadValue() != HTMLMediaElement::PRELOAD_NONE)) {
            // STARFISH_LOG_INFO("HTMLMediaElement::Changing src attribute
            // causes content load (autoplay:%s, preload:%s)\n", autoplay() ?
            // "true" : "false", preload()->utf8Data());
            load();
            if (autoplay()) {
                appendToPlayOperationQueue(
                    new MediaOperationQueueDataRequestPlay(this));
            }
        }
    } else if (name == starFish()->staticStrings()->m_loop) {
        if (m_mediaPlayer) {
            m_mediaPlayer->setLoop(!attributeRemoved);
        }
    }
}

void HTMLMediaElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
}

void HTMLMediaElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeRemovedFromDocumentTree();
    pause();
}

void HTMLMediaElement::load()
{
    STARFISH_LOG_INFO("HTMLMediaElement::load()\n");
    // 4.8.12.5 Loading the media resource
    // While the delaying-the-load-event flag is true, the element must delay
    // the load event of its document.
    // Abort any already-running instance of the resource selection algorithm
    // for this element.

    // Let pending tasks be a list of all tasks from the media element's media
    // element event task source in one of the task queues.
    // For each task in pending tasks that would resolve pending play promises
    // or reject pending play promises, immediately resolve or reject those
    // promises in the order the corresponding tasks were queued.
    // Remove each task in pending tasks from its task queue
    abortEveryPendingOperation(new DOMException(
        document(), DOMException::DOM_EXCEPTION,
        "The play() request was interrupted by a new load request."));

    // If the media element's networkState is set to NETWORK_LOADING or
    // NETWORK_IDLE, queue a task to fire a simple event named abort at the
    // media element.
    if (networkState() == NETWORK_LOADING || networkState() == NETWORK_IDLE) {
        dispatchAbortEvent();
    }

    // If the media element's networkState is not set to NETWORK_EMPTY, then run
    // these substeps:
    if (networkState() != NETWORK_EMPTY) {
        // Queue a task to fire a simple event named emptied at the media
        // element.
        dispatchEmptiedEvent();

        // If a fetching process is in progress for the media element, the user
        // agent should stop it.
        // If the media element's assigned media provider object is a
        // MediaSource object, then detach it.
        // NOTE: MediaSource object be detached in closeMediaPlayer()
        closeMediaPlayer();

        // TODO Forget the media element's media-resource-specific tracks.

        // If readyState is not set to HAVE_NOTHING, then set it to that state.
        m_readyState = HAVE_NOTHING;

        // If the paused attribute is false, then run these substeps:
        if (m_isPaused == false) {
            // Set the paused attribute to true.
            m_isPaused = true;

            // Take pending play promises and reject pending play promises with
            // the result and an "AbortError" DOMException.
            auto iter = m_playOperationQueue.begin();
            while (iter != m_playOperationQueue.end()) {
                DOMException* exception = new DOMException(
                    document(), DOMException::ABORT_ERR,
                    "play request is aborted by load operation");
                ((MediaOperationQueueDataRequestPlay*)(*iter))
                    ->m_promise->reject(exception->scriptValue());
                m_playOperationQueue.erase(iter++);
            }
        }

        // If seeking is true, set it to false.
        m_isSeeking = false;
        m_pendingSeek = std::numeric_limits<double>::quiet_NaN();
        m_isEnded = false;

        // TODO Set the current playback position to 0.
        // Set the official playback position to 0.
        if (m_officialPlaybackPosition != 0) {
            setOfficialPlaybackPosition(0);
        }

        // TODO Set the timeline offset to Not-a-Number (NaN).
        // TODO Update the duration attribute to Not-a-Number (NaN).
        // NOTE The user agent will not fire a durationchange event for this
        // particular change of the duration.
    }

    // TODO Set the playbackRate attribute to the value of the
    // defaultPlaybackRate attribute.

    // Set the error attribute to null and the autoplaying flag to true.
    m_autoplayingFlag = true;

    // Invoke the media element's resource selection algorithm.
    resourceSelection();
}

void HTMLMediaElement::closeMediaPlayer()
{
    if (m_mediaPlayer) {
        m_mediaPlayer->close();
        m_mediaPlayer = nullptr;
    }
}

void HTMLMediaElement::initMediaPlayer()
{
    closeMediaPlayer();
    m_mediaPlayer = MediaPlayer::create(this);
    m_mediaPlayer->setLoop(loop());
    m_isSeeking = false;
    m_pendingSeek = std::numeric_limits<double>::quiet_NaN();
}

void HTMLMediaElement::resourceSelection()
{
    STARFISH_LOG_INFO("HTMLMediaElement::resourceSelection()\n");
    closeMediaPlayer();
    m_networkState = NETWORK_NO_SOURCE;
    // Set the element's show poster flag to true.
    // Set the media element's delaying-the-load-event flag to true (this delays
    // the load event).
    m_delayingTheLoadEvent = true;

    m_resourceSelectionContext = new ResourceSelectionContext(this);
    appendToOperationQueue(
        new MediaOperationQueueDataRequestResourceSelection(this));
}

void HTMLMediaElement::dedicatedMediaSourceFailure()
{
    // TODO Set the error attribute to a new MediaError object whose code
    // attribute is set to MEDIA_ERR_SRC_NOT_SUPPORTED.
    // TODO Forget the media element's media-resource-specific tracks.
    // Set the element's networkState attribute to the NETWORK_NO_SOURCE value.
    m_networkState = NETWORK_NO_SOURCE;

    // TODO Set the element's show poster flag to true.
    // Fire a simple event named error at the media element.
    dispatchErrorEventNow();

    // Reject pending play promises with promises and a "NotSupportedError"
    // DOMException.
    abortEveryPendingOperation(new DOMException(
        document(), DOMException::NOT_SUPPORTED_ERR, "cannot play media"));

    // Set the element's delaying-the-load-event flag to false. This stops
    // delaying the load event.
    m_delayingTheLoadEvent = false;
}

void HTMLMediaElement::giveupFetchingResource(bool shouldSetError)
{
    if (shouldSetError) {
        // STARFISH_ASSERT(m_readyState > HAVE_NOTHING);
        // TODO Set the error attribute to a new MediaError object whose code
        // attribute is set to MEDIA_ERR_NETWORK / MEDIA_ERROR_DECODE

        // Set the element's networkState attribute to the NETWORK_IDLE value.
        m_networkState = NETWORK_IDLE;

        // Set the element's delaying-the-load-event flag to false. This stops
        // delaying the load event.
        m_delayingTheLoadEvent = false;

        // Fire a simple event named error at the media element.
        dispatchErrorEvent();
    }

    m_resourceSelectionContext = nullptr;
    // Abort the overall resource selection algorithm
    abortEveryPendingOperation(new DOMException(
        document(), DOMException::NOT_SUPPORTED_ERR, "cannot play media"));
}

Promise* HTMLMediaElement::play()
{
    // TODO If the media element is not allowed to play, return a promise
    // rejected with a "NotAllowedError" DOMException and abort these steps.
    // TODO If the media element's error attribute is not null and its code
    // attribute has the value MEDIA_ERR_SRC_NOT_SUPPORTED, return a promise
    // rejected with a "NotSupportedError" DOMException and abort these steps.

    // Let promise be a new promise and append promise to the list of pending
    // play promises.
    auto playRequest = new MediaOperationQueueDataRequestPlay(this);
    appendToPlayOperationQueue(playRequest);

    // If the media element's networkState attribute has the value
    // NETWORK_EMPTY, invoke the media element's resource selection algorithm.
    if (m_networkState == NETWORK_EMPTY) {
        resourceSelection();
    }

    // TODO If the playback has ended and the direction of playback is forwards,
    // seek to the earliest possible position of the media resource.
    if (m_isEnded) {
        setCurrentTime(0);
    }

    // If the media element's paused attribute is true, run the following
    // substeps:
    if (m_isPaused == true) {
        // Change the value of paused to false.
        m_isPaused = false;
        // TODO If the show poster flag is true, set the element's show poster
        // flag to false and run the time marches on steps.

        // Queue a task to fire a simple event named play at the element.
        dispatchPlayEvent();

        // If the media element's readyState attribute has the value
        // HAVE_NOTHING, HAVE_METADATA, or HAVE_CURRENT_DATA, queue a task to
        // fire a simple event named waiting at the element.
        if (m_readyState == HAVE_NOTHING || m_readyState == HAVE_METADATA ||
            m_readyState == HAVE_CURRENT_DATA) {
            dispatchWaitingEvent();
        } else if (m_readyState == HAVE_FUTURE_DATA ||
                   m_readyState == HAVE_ENOUGH_DATA) {
            // Otherwise, the media element's readyState attribute has the value
            // HAVE_FUTURE_DATA or HAVE_ENOUGH_DATA: notify about playing for
            // the element.
            notifyAboutPlaying();
        }
    } else if (m_readyState == HAVE_FUTURE_DATA ||
               m_readyState == HAVE_ENOUGH_DATA) {
        // Otherwise, if the media element's readyState attribute has the value
        // HAVE_FUTURE_DATA or HAVE_ENOUGH_DATA,
        // take pending play promises and queue a task to resolve pending play
        // promises with the result.
        auto iter = m_playOperationQueue.begin();
        while (iter != m_playOperationQueue.end()) {
            ((MediaOperationQueueDataRequestPlay*)(*iter))
                ->processOperationQueue();
            m_playOperationQueue.erase(iter++);
        }
    }

    // Set the media element's autoplaying flag to false.
    m_autoplayingFlag = false;

    // Return promise.
    return playRequest->m_promise;
}

void HTMLMediaElement::pause()
{
    // If the media element's networkState attribute has the value
    // NETWORK_EMPTY, invoke the media element's resource selection algorithm.
    if (networkState() == NETWORK_EMPTY) {
        resourceSelection();
    }
    // Set the media element's autoplaying flag to false.
    m_autoplayingFlag = false;

    // If the media element's paused attribute is false, run the following
    // steps:
    if (m_isPaused == false) {
        // Change the value of paused to true.
        m_isPaused = true;
        // Queue a task to run the following substeps:
        appendToOperationQueue(new MediaOperationQueueDataRequestPause(this));
    }
}

void HTMLMediaElement::addTextTrack(TextTrack* track)
{
    if (track) {
        m_textTracks->push_back(track);
    }
}

void HTMLMediaElement::removeTextTrack(TextTrack* track)
{
    if (track) {
        auto iter =
            std::find(m_textTracks->begin(), m_textTracks->end(), track);
        m_textTracks->erase(iter);
    }
}

void HTMLMediaElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

String* HTMLMediaElement::src()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_src);
}

TextTrack* HTMLMediaElement::addTextTrack(String* kind, String* label,
                                          String* language)
{
    TextTrack::Kind kindEnum = TextTrack::stringToKind(kind);
    // TODO (escargot2) change Typeof kind to TextTrack::Kind
    // TODO (escargot2) move this exception to binding layer
    /*
    if (kindEnum == TextTrack::Kind::InvalidKind) {
        COMPOSE_MESSAGE(reason, ARG_TYPE_MISMATCH_WITH_ENUM, "TextTrackKind");
        COMPOSE_MESSAGE(msg, FAILED_TO_EXECUTE, "addTextTrack",
                        "HTMLMediaElement", reason);
        THROW_EXCEPTION(msg);
    }*/
    TextTrack* textTrack = new TextTrack(m_document, kindEnum, label, language);
    m_textTracks->push_back(textTrack);
    return textTrack;
}

void HTMLMediaElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    if (parent == this && newChild->isHTMLTrackElement()) {
        HTMLTrackElement* trackElement = newChild->asHTMLTrackElement();
        STARFISH_ASSERT(trackElement->track());
        addTextTrack(trackElement->track());
    }
    if (m_resourceSelectionContext &&
        m_resourceSelectionContext->waitingChildren()) {
        // TODO Release waiting?
    }
}

void HTMLMediaElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    HTMLElement::didNodeRemoved(parent, oldChild);
    if (parent == this && oldChild->isHTMLTrackElement()) {
        HTMLTrackElement* trackElement = oldChild->asHTMLTrackElement();
        STARFISH_ASSERT(trackElement->track());
        removeTextTrack(trackElement->track());
    }
    if (m_resourceSelectionContext &&
        m_resourceSelectionContext->hasPointer()) {
        if (oldChild == m_resourceSelectionContext->nodeBeforePointer()) {
            m_resourceSelectionContext->updatePointer(previousSibling());
        }
    }
}

HTMLMediaElement::PreloadState HTMLMediaElement::preloadValue()
{
    QualifiedName preload = starFish()->staticStrings()->m_preload;
    size_t siz = hasAttribute(preload);
    if (siz != SIZE_MAX) {
        String* value = getAttributeOrEmpty(preload);
        if (value->length() == 0 ||
            (value->length() == 4 &&
             value->equalsWithoutCase(
                 AtomicString::createAtomicString(starFish(), "auto")
                     .string()))) {
            // The empty string is also a valid keyword, and maps to the
            // Automatic state
            return HTMLMediaElement::PRELOAD_AUTOMATIC;
        } else if (value->length() == 8 &&
                   value->equalsWithoutCase(
                       AtomicString::createAtomicString(starFish(), "metadata")
                           .string())) {
            return HTMLMediaElement::PRELOAD_METADATA;
        } else if (value->length() == 4 &&
                   value->equalsWithoutCase(
                       AtomicString::createAtomicString(starFish(), "none")
                           .string())) {
            return HTMLMediaElement::PRELOAD_NONE;
        }
    }
    // Default : Automatic
    return HTMLMediaElement::PRELOAD_AUTOMATIC;
}

String* HTMLMediaElement::preload()
{
    return HTMLMediaElement::preloadToString(starFish(), preloadValue());
}

TimeRanges* HTMLMediaElement::buffered()
{
    return seekable();
}

String* HTMLMediaElement::canPlayType(String* type)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return String::emptyString;
}

bool HTMLMediaElement::seeking()
{
    return m_mediaPlayer ? m_isSeeking : false;
}

double HTMLMediaElement::currentTime()
{
    // Note :
    // https://www.w3.org/TR/html51/semantics-embedded-content.html
    //         #dom-htmlmediaelement-currenttime
    return m_mediaPlayer ? m_officialPlaybackPosition : 0;
}

double HTMLMediaElement::duration()
{
    return m_mediaPlayer ? m_mediaPlayer->duration()
                         : std::numeric_limits<double>::quiet_NaN();
}

bool HTMLMediaElement::paused()
{
    return m_isPaused;
}

double HTMLMediaElement::defaultPlaybackRate()
{
    // Unimplmented
    STARFISH_ASSERT_NOT_REACHED();
    return 1;
}

double HTMLMediaElement::playbackRate()
{
    // Unimplmented
    STARFISH_ASSERT_NOT_REACHED();
    return 1;
}

TimeRanges* HTMLMediaElement::played()
{
    // Unimplmented
    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

TimeRanges* HTMLMediaElement::seekable()
{
    if (!m_mediaPlayer) {
        return new TimeRanges(document());
    }
    if (m_mediaPlayer->activeMediaSource()) {
        SourceBufferList* bufferList =
            m_mediaPlayer->activeMediaSource()->activeSourceBuffers();
        STARFISH_ASSERT(bufferList);
        unsigned nbuffer = bufferList->length();

        if (nbuffer == 0) {
            return new TimeRanges(document());
        }

        TimeRanges* result = (*bufferList)[0]->buffered();
        for (unsigned i = 1; i < nbuffer; i++) {
            TimeRanges* buffered = (*bufferList)[i]->buffered();
            unsigned bufferedSize = buffered->size();
            unsigned resultSize = result->size();
            TimeRanges* newResult = new TimeRanges(document());

            unsigned t = 0, j = 0;
            while (t != bufferedSize && j != resultSize) {
                if (buffered->end(t) < result->start(j)) {
                    t++;
                } else if (buffered->start(t) > result->end(j)) {
                    j++;
                } else {
                    newResult->emplace_back(
                        std::max(buffered->start(t), result->start(j)),
                        std::min(buffered->end(t), result->end(j)));
                    if (buffered->start(t) >= result->start(j) &&
                        buffered->end(t) <= result->end(j)) {
                        t++;
                    } else if (buffered->start(t) <= result->start(j) &&
                               buffered->end(t) >= result->end(j)) {
                        j++;
                    } else if (buffered->start(t) < result->start(j)) {
                        t++;
                    } else {
                        j++;
                    }
                }
            }
            result = newResult;
        }
        return result;
    } else {
        TimeRanges* r = new TimeRanges(document());
        r->emplace_back(0, m_mediaPlayer->duration());
        return r;
    }
}

bool HTMLMediaElement::ended()
{
    return m_isEnded;
}

bool HTMLMediaElement::autoplay()
{
    size_t siz = hasAttribute(starFish()->staticStrings()->m_autoplay);
    if (siz == SIZE_MAX) {
        return false;
    }
    return true;
}

bool HTMLMediaElement::loop()
{
    size_t siz = hasAttribute(starFish()->staticStrings()->m_loop);
    if (siz == SIZE_MAX) {
        return false;
    }
    return true;
}

bool HTMLMediaElement::controls()
{
    size_t siz = hasAttribute(starFish()->staticStrings()->m_controls);
    if (siz == SIZE_MAX) {
        return false;
    }
    return true;
}

double HTMLMediaElement::volume()
{
    return m_volume;
}

bool HTMLMediaElement::muted()
{
    return m_muted;
}

String* HTMLMediaElement::currentSrc()
{
    return m_currentSrc;
}

void HTMLMediaElement::setPreload(String* preload)
{
    setAttribute(starFish()->staticStrings()->m_preload, preload);
}

double HTMLMediaElement::defaultPlaybackStartPosition()
{
    return m_defaultPlaybackStartPosition;
}

void HTMLMediaElement::setDefaultPlaybackStartPosition(double position)
{
    m_defaultPlaybackStartPosition = position;
}

void HTMLMediaElement::setCurrentTime(double currentTime)
{
    // On setting, if the media element’s readyState is HAVE_NOTHING,
    // then it must set the media element’s default playback start position
    // to the new value; otherwise, it must set the official playback position
    // to the new value and then seek to the new value.
    STARFISH_LOG_INFO("HTMLMediaElement::setCurrentTime() %lf \n", currentTime);
    if (m_readyState == HAVE_NOTHING) {
        m_defaultPlaybackStartPosition = currentTime;
        STARFISH_LOG_INFO(
            "HTMLMediaElement::setCurrentTime() readyState is "
            "HAVE_NOTHING..\n");
    } else {
        m_isEnded = false;
        if (m_isSeeking) {
            // https://www.w3.org/TR/html51/semantics-embedded-content.html#seek
            // If the element's seeking IDL attribute is true, (...) algorithm
            // is already running.
            // Abort that other instance of the algorithm without waiting for
            // the step that it is running to complete.
            // Note : But there is no way of aborting player_set_position_async,
            // we have to wait.
            STARFISH_LOG_INFO(
                "HTMLMediaElement::setCurrentTime() Seek pending..\n");
            m_pendingSeek = currentTime;
        } else {
            // Note: Set seeking flag to true here to prevent MediaPlayer's
            // timer updating officialPlaybackPosition
            //       while "seek" is in the operation queue.
            m_isSeeking = true;
            // Note: Update officialPlaybackPosition here but not dispatch
            // timeupdate event.
            // Event sequence : seeking -> (SEEK) -> timeupdate -> seeked ->
            // timeupdate
            m_officialPlaybackPosition = currentTime;
            m_readyState = HAVE_METADATA;
            appendToOperationQueue(
                new MediaOperationQueueDataRequestSeek(this, currentTime));
        }
    }
}

void HTMLMediaElement::setOfficialPlaybackPosition(double time)
{
    // Note: officialPlaybackPosition should be updated manually when seeking
    if (!m_isSeeking && m_officialPlaybackPosition != time) {
        m_officialPlaybackPosition = time;
        dispatchTimeupdateEvent();
    }
}

void HTMLMediaElement::setDefaultPlaybackRate(double defaultPlaybackRate)
{
    // Unimplmented
    STARFISH_ASSERT_NOT_REACHED();
}

void HTMLMediaElement::setPlaybackRate(double playbackRate)
{
    // Unimplmented
    STARFISH_ASSERT_NOT_REACHED();
}

void HTMLMediaElement::setAutoplay(bool autoplay)
{
    QualifiedName name = starFish()->staticStrings()->m_autoplay;
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
    QualifiedName name = starFish()->staticStrings()->m_loop;
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
    QualifiedName name = starFish()->staticStrings()->m_controls;
    if (controls) {
        size_t siz = hasAttribute(name);
        if (siz == SIZE_MAX) {
            setAttribute(name, String::emptyString);
        }
    } else {
        removeAttribute(name);
    }
}

void HTMLMediaElement::setVolume(double volume)
{
    if (volume < 0.0f || volume > 1.0f) {
        throw new DOMException(document(), DOMException::INDEX_SIZE_ERR,
                               "volume should be in the range 0.0 to 1.0");
    }

    if (m_volume != volume) {
        m_volume = volume;
        if (m_mediaPlayer) {
            m_mediaPlayer->setVolume(volume);
        }
        dispatchVolumechangeEvent();
    }
}

void HTMLMediaElement::setMuted(bool muted)
{
    if (m_muted != muted) {
        if (m_mediaPlayer) {
            m_mediaPlayer->setMuted(muted);
        }
        dispatchVolumechangeEvent();
    }
}

HTMLMediaElement::ReadyState HTMLMediaElement::readyState()
{
    return m_readyState;
}

void HTMLMediaElement::notifyAboutPlaying()
{
    // https://html.spec.whatwg.org/multipage/embedded-content.html#notify-about-playing
    dispatchPlayingEvent();
    while (m_playOperationQueue.size()) {
        ((MediaOperationQueueDataRequestPlay*)m_playOperationQueue.front())
            ->processOperationQueue();
        m_playOperationQueue.pop_front();
    }
}

void HTMLMediaElement::mediaPlayerNotifyUpdateReadyStateItsContainer(
    HTMLMediaElement::ReadyState state)
{
    if (state == m_readyState) {
        return;
    }

    HTMLMediaElement::ReadyState prevState = m_readyState;
    m_readyState = state;

    if (networkState() != HTMLMediaElement::NETWORK_EMPTY) {
        // If the previous ready state was HAVE_NOTHING, and the new ready state
        // is HAVE_METADATA
        if (prevState == HTMLMediaElement::HAVE_NOTHING &&
            state == HTMLMediaElement::HAVE_METADATA) {
            // NOTE : Before this task is run, as part of the event loop
            // mechanism,
            //        the rendering will have been updated to resize the video
            //        element if appropriate.
            if (isHTMLVideoElement() && frame()) {
                setNeedsLayout();
            }
            // Queue a task to fire an event named loadedmetadata at the
            // element.
            dispatchLoadedmetadataEvent();
        }

        // If the previous ready state was HAVE_METADATA and the new ready state
        // is HAVE_CURRENT_DATA or greater
        if (prevState == HTMLMediaElement::HAVE_METADATA &&
            state >= HTMLMediaElement::HAVE_CURRENT_DATA) {
            dispatchLoadeddataEvent();
        }
        // If the previous ready state was HAVE_FUTURE_DATA or more, and the new
        // ready state is HAVE_CURRENT_DATA or less
        if (prevState >= HTMLMediaElement::HAVE_FUTURE_DATA &&
            state <= HTMLMediaElement::HAVE_CURRENT_DATA) {
            if (m_mediaPlayer &&
                m_mediaPlayer->playbackState() ==
                    MediaPlayer::PLAYBACK_STATE_PLAYING) {
                dispatchTimeupdateEvent();
                dispatchWaitingEvent();
            }
        }
        // If the previous ready state was HAVE_CURRENT_DATA or less, and the
        // new ready state is HAVE_FUTURE_DATA,
        if (prevState <= HTMLMediaElement::HAVE_CURRENT_DATA &&
            state == HTMLMediaElement::HAVE_FUTURE_DATA) {
            // The user agent must queue a task to fire an event named canplay
            // at the element.
            dispatchCanplayEvent();

            // If the element's paused attribute is false, the user agent must
            // notify about playing for the element.
            if (!m_isPaused) {
                notifyAboutPlaying();
            }
        }
        // If the new ready state is HAVE_ENOUGH_DATA,
        if (state == HTMLMediaElement::HAVE_ENOUGH_DATA) {
            // If the previous ready state was HAVE_CURRENT_DATA or lesss,
            // the user agent must queue a task to fire an event named canplay
            // at the element, and,
            // if the element's paused attribute is false, notify about playing
            // for the element.
            if (prevState <= HTMLMediaElement::HAVE_CURRENT_DATA) {
                dispatchCanplayEvent();
                if (!m_isPaused) {
                    notifyAboutPlaying();
                }
            }

            if (eligibleForAutoplay()) {
                // 1. Set the paused attribute to false.
                m_isPaused = false;
                // 2. If the element's show poster flag is true, set it to false
                // and run the time marches on steps.
                // TODO
                // 3. Queue a task to fire an event named play at the element.
                dispatchPlayEvent();
                // 4. Notify about playing for the element.
                notifyAboutPlaying();
                // 5. Set the autoplaying flag to false.
                m_autoplayingFlag = false;
            }

            // The user agent must queue a task to fire an event named
            // canplaythrough at the element.
            dispatchCanplaythroughEvent();
        }
    } else {
    }
}

void HTMLMediaElement::mediaPlayerNotifySeekedItsContainer(double currentTime)
{
    STARFISH_LOG_INFO(
        "HTMLMediaElement::mediaPlayerNotifySeekedItsContainer (%lf)\n",
        currentTime);
    // Note : Set officialPlaybackPosition manually instead of calling
    // setOfficialPlaybackPosition()
    //        Because m_isSeeking effects setOfficialPlaybackPosition()
    m_officialPlaybackPosition = currentTime;

    if (!std::isnan(m_pendingSeek) && m_pendingSeek != currentTime) {
        STARFISH_LOG_INFO(
            "HTMLMediaElement::mediaPlayerNotifySeekedItsContainer found "
            "pending seek operation (%lf)\n",
            m_pendingSeek);
        double pendingSeek = m_pendingSeek;
        m_pendingSeek = std::numeric_limits<double>::quiet_NaN();
        appendToOperationQueue(
            new MediaOperationQueueDataRequestSeek(this, pendingSeek));
    } else {
        // Finish "seek"
        m_isSeeking = false;
        mediaPlayerNotifyUpdateReadyStateItsContainer(HAVE_ENOUGH_DATA);
        dispatchTimeupdateEvent();
        dispatchSeekedEvent();
        dispatchTimeupdateEvent();
    }
}

void HTMLMediaElement::mediaPlayerNotifySeekFailureItsContainer()
{
    STARFISH_LOG_INFO(
        "HTMLMediaElement::mediaPlayerNotifySeekFailureItsContainer\n");
    m_isSeeking = false;
    m_pendingSeek = std::numeric_limits<double>::quiet_NaN();
    // TODO
    dedicatedMediaSourceFailure();
    closeMediaPlayer();
}

void HTMLMediaElement::mediaPlayerNotifyEndedItsContainer()
{
    m_isPaused = true;
    dispatchPauseEventNow();

    m_isEnded = true;
    m_officialPlaybackPosition = duration();
    dispatchTimeupdateEvent();
    dispatchEndedEvent();
}

HTMLMediaElement::NetworkState HTMLMediaElement::networkState()
{
    return m_networkState;
}

void HTMLMediaElement::addEventToOperationQueue(EventTarget* t, Event* e)
{
    appendToOperationQueue(
        new MediaOperationQueueDataRequestDispatchEvent(this, t, e));
}

#define ADD_DISPATCH_EVENT_DEF(name, Name)                                     \
    void HTMLMediaElement::dispatch##Name##EventNow()                          \
    {                                                                          \
        String* eventType = starFish()->staticStrings()->m_##name.localName(); \
        Event* e = new Event(document(), eventType, EventInit(false, false));  \
        dispatchEvent(e);                                                      \
    }                                                                          \
    void HTMLMediaElement::dispatch##Name##Event()                             \
    {                                                                          \
        String* eventType = starFish()->staticStrings()->m_##name.localName(); \
        Event* e = new Event(document(), eventType, EventInit(false, false));  \
        addEventToOperationQueue(this, e);                                     \
    }

ADD_DISPATCH_EVENT_DEF(progress, Progress);
ADD_DISPATCH_EVENT_DEF(suspend, Suspend);
ADD_DISPATCH_EVENT_DEF(abort, Abort);
ADD_DISPATCH_EVENT_DEF(error, Error);
ADD_DISPATCH_EVENT_DEF(emptied, Emptied);
ADD_DISPATCH_EVENT_DEF(stalled, Stalled);
ADD_DISPATCH_EVENT_DEF(loadedmetadata, Loadedmetadata);
ADD_DISPATCH_EVENT_DEF(loadeddata, Loadeddata);
ADD_DISPATCH_EVENT_DEF(loadstart, Loadstart);
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

void HTMLMediaElement::abortEveryPendingOperation(
    DOMException* exceptionForPlayPromise)
{
    STARFISH_LOG_INFO("HTMLMediaElement::abortEveryPendingOperation()\n");
    if (m_currentOperation) {
        m_currentOperation->cancelOperation();
        m_currentOperation = nullptr;
    }

    while (m_operationQueue.size()) {
        m_operationQueue.front()->cancelOperation();
        m_operationQueue.pop_front();
    }

    while (m_playOperationQueue.size()) {
        ((MediaOperationQueueDataRequestPlay*)m_playOperationQueue.front())
            ->cancelOperation(exceptionForPlayPromise);
        m_playOperationQueue.pop_front();
    }

    if (m_currentPendingOperationHandle != SIZE_MAX) {
        starFish()->messageLoop()->removeIdler(m_currentPendingOperationHandle);
        m_currentPendingOperationHandle = SIZE_MAX;
        m_currentPendingOperationCount = 0;
    }

    m_pendingSeek = std::numeric_limits<double>::quiet_NaN();
    STARFISH_ASSERT(m_currentPendingOperationCount == 0);
    STARFISH_ASSERT(m_currentPendingOperationHandle == SIZE_MAX);
}

void HTMLMediaElement::processNextOperationQueue()
{
    STARFISH_ASSERT(m_currentPendingOperationCount == 0);
    m_currentOperation = nullptr;

    if (m_operationQueue.size()) {
        STARFISH_ASSERT(m_currentPendingOperationCount == 0);
        STARFISH_ASSERT(m_currentOperation == nullptr);
        STARFISH_ASSERT(m_currentPendingOperationHandle == SIZE_MAX);

        m_currentPendingOperationCount++;
        MediaOperationQueueData* next = m_operationQueue.front();
        STARFISH_ASSERT(!next->isPlayRequest());
        m_operationQueue.pop_front();

        m_currentPendingOperationHandle = starFish()->messageLoop()->addIdler(
            document()->browsingContext(),
            [](size_t, void* data) {
                MediaOperationQueueData* queueData =
                    (MediaOperationQueueData*)data;
                // Clear pending data
                queueData->m_mediaElement->m_currentPendingOperationHandle =
                    SIZE_MAX;
                queueData->m_mediaElement->m_currentPendingOperationCount--;
                STARFISH_ASSERT(
                    queueData->m_mediaElement->m_currentPendingOperationCount ==
                    0);

                // Register and execute current processing operation
                queueData->m_mediaElement->m_currentOperation = queueData;
                queueData->processOperationQueue();
            },
            next);
    }
}

MediaOperationQueueData::MediaOperationQueueData(HTMLMediaElement* p)
    : m_mediaElement(p)
{
}

MediaPlayer* MediaOperationQueueData::mediaPlayer()
{
    return m_mediaElement->mediaPlayer();
}

HTMLSourceElement* ResourceSelectionContext::getNextCandidate()
{
    STARFISH_ASSERT(m_mode == ResourceSelectionContext::MODE_CHILDREN);
    Node* child;
    if (hasPointer()) {
        child = nodeBeforePointer()->nextSibling();
    } else {
        child = m_mediaElement->firstChild();
    }
    updatePointer(child);

    while (child) {
        if (child->isHTMLSourceElement()) {
            return child->asHTMLSourceElement();
        }
        child = child->nextSibling();
        updatePointer(child);
    }

    m_waiting = true;
    return nullptr;
}

void ResourceSelectionContext::failedWithElements(Element* candidate)
{
    // Failed with elements:
    // Queue a task to fire an event named error at the candidate element.
    String* eventType =
        m_mediaElement->starFish()->staticStrings()->m_error.localName();
    Event* e =
        new Event(candidate->document(), eventType, EventInit(false, false));
    m_mediaElement->addEventToOperationQueue(candidate, e);
}

void MediaOperationQueueDataRequestResourceSelection::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestResourceSelection::"
        "processOperationQueue()\n");
    HTMLMediaElement* self = m_mediaElement;
    self->processNextOperationQueue();

    STARFISH_ASSERT(self->m_resourceSelectionContext);
    ResourceSelectionContext* context = self->m_resourceSelectionContext;

    // TODO If the media element's blocked-on-parser flag is false, then
    // populate the list of pending text tracks.
    // TODO If the media element has an assigned media provider object, then let
    // mode be object.

    // Otherwise, if the media element has no assigned media provider object but
    // has a src attribute, then let mode be attribute.
    // Otherwise, if the media element does not have an assigned media provider
    // object and does not have a src attribute,
    // but does have a source element child, then let mode be children and let
    // candidate be the first such source element child in tree order.
    if (context->m_mode == ResourceSelectionContext::MODE_NONE) {
        if (self->hasAttribute(self->starFish()->staticStrings()->m_src) !=
            SIZE_MAX) {
            context->m_mode = ResourceSelectionContext::MODE_ATTRIBUTE;
        } else if (self->hasSourceElementChild()) {
            context->m_mode = ResourceSelectionContext::MODE_CHILDREN;
        } else {
            // Otherwise the media element has no assigned media provider object
            // and has neither a src attribute nor a source element child: set
            // the networkState to NETWORK_EMPTY, and abort these steps; the
            // synchronous section ends.
            self->m_networkState = HTMLMediaElement::NETWORK_EMPTY;
            // NOTE: Firing emptied event and Detaching MediaSource must have
            // done in "load()"
            //       (or does not have to consider if m_networkState were
            //       already NETWORK_EMPTY)
            return;
        }

        // Set the media element's networkState to NETWORK_LOADING.
        self->m_networkState = HTMLMediaElement::NETWORK_LOADING;
        // Queue a task to fire a simple event named loadstart at the media
        // element.
        self->dispatchLoadstartEvent();
    }

    if (context->m_mode == ResourceSelectionContext::MODE_ATTRIBUTE) {
        // If the src attribute's value is the empty string, then end the
        // synchronous section, and jump down to the failed with attribute step
        // below.
        if (self->src()->containsOnlyWhitespace()) {
            self->dedicatedMediaSourceFailure();
            return;
        }

        // If urlString was obtained successfully, set the currentSrc attribute
        // to urlString.
        ResourceURL* url =
            new ResourceURL(self->src(), self->document()->urlString());
        self->m_currentSrc = url->urlString();
        // End the synchronous section, continuing the remaining steps in
        // parallel.
        self->initMediaPlayer();
        self->appendToOperationQueue(
            new MediaOperationQueueDataRequestPrepare(self, url));
        self->appendToOperationQueue(
            new MediaOperationQueueDataRequestSeekToDefault(self));
        return;
    } else if (context->m_mode == ResourceSelectionContext::MODE_CHILDREN) {
        HTMLSourceElement* candidate = context->getNextCandidate();
        if (!candidate) {
            // Wait until the node after pointer is a node other than the end of
            // the list. (This step might wait forever.)
            self->m_networkState = HTMLMediaElement::NETWORK_NO_SOURCE;
            return;
        }
        // Process candidate:
        if (candidate->src()->length() == 0 ||
            candidate->src()->containsOnlyWhitespace()) {
            // If candidate does not have a src attribute, or if its src
            // attribute's value is the empty string,
            // then end the synchronous section, and jump down to the failed
            // with elements step below.
            context->failedWithElements(candidate);
            self->appendToOperationQueue(
                new MediaOperationQueueDataRequestResourceSelection(self));
            return;
        } else if (candidate->type()->length() > 0 &&
                   MimeType::parseFromString(candidate->type()).isValid() &&
                   !MediaSource::isTypeSupported(candidate->type())) {
            // FIXME : MediaSource::isTypeSupported() -> replaced method to
            // proper one.

            // If candidate has a type attribute whose value, when parsed as a
            // MIME type,
            // represents a type that the user agent knows it cannot render,
            // then end the synchronous section,
            // and jump down to the failed with elements step below.
            context->failedWithElements(candidate);
            self->appendToOperationQueue(
                new MediaOperationQueueDataRequestResourceSelection(self));
            return;
        }
        ResourceURL* url =
            new ResourceURL(candidate->src(), self->document()->urlString());
        self->m_currentSrc = url->urlString();
        // End the synchronous section, continuing the remaining steps in
        // parallel.
        self->initMediaPlayer();
        self->appendToOperationQueue(
            new MediaOperationQueueDataRequestPrepare(self, url));
        self->appendToOperationQueue(
            new MediaOperationQueueDataRequestSeekToDefault(self));
        return;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void MediaOperationQueueDataRequestPrepare::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestPrepare::processOperationQueue()\n");
    mediaPlayer()->prepare(m_url);
}

void MediaOperationQueueDataRequestPrepare::cancelOperation()
{
    m_mediaElement->m_resourceSelectionContext = nullptr;
}

void MediaOperationQueueDataRequestSeek::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestSeek::processOperationQueue()\n");
    // TODO Set the media element’s show poster flag to false.
    // If the media element’s readyState is HAVE_NOTHING, abort these steps.
    if (m_mediaElement->readyState() == HTMLMediaElement::HAVE_NOTHING) {
        return;
    }

    m_mediaElement->m_isEnded = false;
    m_mediaElement->m_isSeeking = true;
    if (mediaPlayer()->seeking()) {
        // UNLIKELY
        m_mediaElement->m_pendingSeek = m_seekPosition;
        return;
    }

    if (m_seekPosition < 0) {
        m_seekPosition = 0;
    }

    m_mediaElement->m_officialPlaybackPosition = m_seekPosition;
    m_mediaElement->dispatchSeekingEvent();

    // Async Seek task
    STARFISH_ASSERT(mediaPlayer());
    mediaPlayer()->seek(m_seekPosition);

    // Seek task does not hold operation queue
    m_mediaElement->processNextOperationQueue();
}

void MediaOperationQueueDataRequestSeekToDefault::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestSeekToDefault::processOperationQueue()"
        "\n");
    if (m_mediaElement->m_defaultPlaybackStartPosition != 0) {
        m_seekPosition = m_mediaElement->m_defaultPlaybackStartPosition;
        m_mediaElement->m_defaultPlaybackStartPosition = 0;
        MediaOperationQueueDataRequestSeek::processOperationQueue();
    } else {
        m_mediaElement->processNextOperationQueue();
    }
}

void MediaOperationQueueDataRequestPause::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestPause::processOperationQueue()\n");
    mediaPlayer()->pause();
    m_mediaElement->processNextOperationQueue();
    // Fire a simple event named timeupdate at the element.
    m_mediaElement->dispatchTimeupdateEventNow();
    // Fire a simple event named pause at the element.
    m_mediaElement->dispatchPauseEventNow();
    // Reject pending play promises with promises and an "AbortError"
    // DOMException.
    auto iter = m_mediaElement->m_playOperationQueue.begin();
    while (iter != m_mediaElement->m_playOperationQueue.end()) {
        DOMException* exception = new DOMException(
            m_mediaElement->document(), DOMException::ABORT_ERR,
            "play request is aborted by pause()");
        ((MediaOperationQueueDataRequestPlay*)(*iter))
            ->m_promise->reject(exception->scriptValue());
        m_mediaElement->m_playOperationQueue.erase(iter++);
    }
    // Set the official playback position to the current playback position.
    m_mediaElement->setOfficialPlaybackPosition(mediaPlayer()->currentTime());
}

void MediaOperationQueueDataRequestDispatchEvent::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestDispatchEvent::processOperationQueue() "
        "-> %s\n",
        m_event->type()->utf8Data());
    m_mediaElement->processNextOperationQueue();
    m_target->dispatchEvent(m_event);
}

MediaOperationQueueDataRequestPlay::MediaOperationQueueDataRequestPlay(
    HTMLMediaElement* p, Promise* pm)
    : MediaOperationQueueData(p)
{
    if (pm) {
        m_promise = pm;
    } else {
        m_promise = new Promise(p->scriptBindingInstance());
    }
}

void MediaOperationQueueDataRequestPlay::processOperationQueue()
{
    STARFISH_LOG_INFO(
        "MediaOperationQueueDataRequestPlay::processOperationQueue()\n");
    mediaPlayer()->play();
    m_promise->fulfill(scriptUndefined());
}

void MediaOperationQueueDataRequestPlay::cancelOperation(
    DOMException* exception)
{
    m_promise->reject(exception->scriptValue());
}
}

#endif
