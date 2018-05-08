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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishHTMLMediaElement__)
#define __StarFishHTMLMediaElement__

#include "core/dom/HTMLElement.h"
#include "core/extra/TimeRange.h"

namespace StarFish {

class Event;
class EventTarget;
class HTMLMediaElement;
class HTMLSourceElement;
class MediaPlayer;
class Node;
class TextTrack;
class TextTrackList;
class TimeRanges;
class URL;

class ResourceSelectionContext : public gc {
public:
    enum Mode {
        MODE_NONE,   /* INITIAL */
        MODE_OBJECT, /* NOT SUPPORT */
        MODE_ATTRIBUTE,
        MODE_CHILDREN,
    };

    ResourceSelectionContext(HTMLMediaElement* element)
        : m_mediaElement(element)
        , m_nodeBeforePointer(nullptr)
        , m_mode(MODE_NONE)
        , m_waiting(false)
    {
    }

    Node* nodeBeforePointer()
    {
        return m_nodeBeforePointer;
    }
    void updatePointer(Node* nodeBeforePointer)
    {
        m_nodeBeforePointer = nodeBeforePointer;
    }
    bool hasPointer()
    {
        return (m_nodeBeforePointer != nullptr);
    }
    bool waitingChildren()
    {
        return m_waiting;
    }
    void failedWithElements(Element* candidate);
    HTMLSourceElement* getNextCandidate();

    HTMLMediaElement* m_mediaElement;
    Node* m_nodeBeforePointer;
    Mode m_mode;
    bool m_waiting;
};

class MediaOperationQueueData : public gc {
public:
    MediaOperationQueueData(HTMLMediaElement* p);
    virtual void cancelOperation()
    {
    }
    virtual void processOperationQueue() = 0;
    virtual ~MediaOperationQueueData()
    {
    }
    virtual bool isPlayRequest()
    {
        return false;
    }
    MediaPlayer* mediaPlayer();

    HTMLMediaElement* m_mediaElement;
};

class MediaOperationQueueDataRequestResourceSelection
    : public MediaOperationQueueData {
public:
    MediaOperationQueueDataRequestResourceSelection(HTMLMediaElement* p)
        : MediaOperationQueueData(p)
    {
    }

    virtual void processOperationQueue() override;
};

class MediaOperationQueueDataRequestPrepare : public MediaOperationQueueData {
public:
    MediaOperationQueueDataRequestPrepare(HTMLMediaElement* p, ResourceURL* u)
        : MediaOperationQueueData(p)
        , m_url(u)
    {
    }

    virtual void processOperationQueue() override;
    virtual void cancelOperation() override;

    ResourceURL* m_url;
};

class MediaOperationQueueDataRequestPlay : public MediaOperationQueueData {
public:
    MediaOperationQueueDataRequestPlay(HTMLMediaElement* p,
                                       Promise* pm = nullptr);
    virtual void processOperationQueue() override;
    void cancelOperationWithException(DOMException* exception);
    virtual bool isPlayRequest() override
    {
        return true;
    }

    Promise* m_promise;
};

class MediaOperationQueueDataRequestPause : public MediaOperationQueueData {
public:
    MediaOperationQueueDataRequestPause(HTMLMediaElement* p)
        : MediaOperationQueueData(p)
    {
    }

    virtual void processOperationQueue() override;
};

class MediaOperationQueueDataRequestSeek : public MediaOperationQueueData {
public:
    MediaOperationQueueDataRequestSeek(HTMLMediaElement* p, double position)
        : MediaOperationQueueData(p)
        , m_seekPosition(position)
    {
    }

    virtual void processOperationQueue() override;

    double m_seekPosition;
};

class MediaOperationQueueDataRequestSeekToDefault
    : public MediaOperationQueueDataRequestSeek {
public:
    MediaOperationQueueDataRequestSeekToDefault(HTMLMediaElement* p)
        : MediaOperationQueueDataRequestSeek(p, 0)
    {
    }

    virtual void processOperationQueue() override;
};

class MediaOperationQueueDataRequestDispatchEvent
    : public MediaOperationQueueData {
public:
    MediaOperationQueueDataRequestDispatchEvent(HTMLMediaElement* p,
                                                EventTarget* target, Event* e)
        : MediaOperationQueueData(p)
        , m_target(target)
        , m_event(e)
    {
    }

    virtual void processOperationQueue() override;
    EventTarget* m_target;
    Event* m_event;
};

typedef std::list<MediaOperationQueueData*,
                  gc_allocator_ignore_off_page<MediaOperationQueueData*>>
    MediaOperationQueue;

class HTMLMediaElement : public HTMLElement {
    friend class MediaPlayer;
    friend class MediaOperationQueueDataRequestPause;
    friend class MediaOperationQueueDataRequestSeek;
    friend class MediaOperationQueueDataRequestSeekToDefault;
    friend class MediaOperationQueueDataRequestResourceSelection;
    friend class MediaOperationQueueDataRequestPrepare;
    friend class MediaOperationQueueDataRequestDispatchEvent;

public:
    enum NetworkState {
        NETWORK_EMPTY,
        NETWORK_IDLE,
        NETWORK_LOADING,
        NETWORK_NO_SOURCE, // TODO
    };

    enum ReadyState {
        HAVE_NOTHING,
        HAVE_METADATA,
        HAVE_CURRENT_DATA,
        HAVE_FUTURE_DATA,
        HAVE_ENOUGH_DATA,
    };

    enum PreloadState {
        PRELOAD_NONE,
        PRELOAD_METADATA,
        PRELOAD_AUTOMATIC,
    };

    HTMLMediaElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLMediaElement() const override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;
    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeInsertedToDocumentTree() override;
    virtual void didNodeRemovedFromDocumentTree() override;
    void onDOMContentLoaded();

    TextTrackList* textTracks()
    {
        return m_textTracks;
    }

    void addTextTrack(TextTrack* track);
    TextTrack* addTextTrack(String* kind, String* label, String* language);
    void removeTextTrack(TextTrack* track);

    void setSrc(String* src);
    String* src();

    String* currentSrc();
    NetworkState networkState();
    PreloadState preloadValue();
    String* preload();
    TimeRanges* buffered();
    String* canPlayType(String* type);
    ReadyState readyState();
    bool seeking();
    double currentTime();
    double duration();
    bool paused();
    double defaultPlaybackRate();
    double playbackRate();
    double defaultPlaybackStartPosition();
    double officialPlaybackPosition()
    {
        return m_officialPlaybackPosition;
    }

    TimeRanges* played();
    TimeRanges* seekable();

    void load();
    Promise* play();
    void pause();
    bool ended();
    bool autoplay();
    bool loop();
    bool controls();
    double volume();
    bool muted();

    void setPreload(String* preload);
    void setCurrentTime(double currentTime);
    void setOfficialPlaybackPosition(double time);
    void setDefaultPlaybackStartPosition(double position);
    void setDefaultPlaybackRate(double defaultPlaybackRate);
    void setPlaybackRate(double playbackRate);
    void setAutoplay(bool autoplay);
    void setLoop(bool loop);
    void setControls(bool controls);
    void setVolume(double volume);
    void setMuted(bool muted);

    void setPlayStartPos(double start);
    void setPlayEndPos(double end);

    static String* preloadToString(StarFish* starfish, PreloadState state)
    {
        switch (state) {
        case PRELOAD_NONE:
            return AtomicString::createAtomicString(starfish, "none").string();
        case PRELOAD_METADATA:
            return AtomicString::createAtomicString(starfish, "metadata")
                .string();
        case PRELOAD_AUTOMATIC:
            return AtomicString::createAtomicString(starfish, "auto").string();
        }
        return String::emptyString;
    }

    MediaPlayer* activeMediaPlayer();

    void mediaPlayerNotifyUpdateReadyStateItsContainer(ReadyState state);
    void mediaPlayerNotifySeekedItsContainer(double currentTime);
    void mediaPlayerNotifySeekFailureItsContainer();
    void mediaPlayerNotifyEndedItsContainer();
    void mediaPlayerRequestRestartItsContainer();
    void addEventToOperationQueue(EventTarget* t, Event* e);
    void processNextOperationQueue();

    DOMTokenList* controlsList();

#define ADD_DISPATCH_EVENT_DECL(Name) \
    void dispatch##Name##EventNow();  \
    void dispatch##Name##Event();
    ADD_DISPATCH_EVENT_DECL(Progress)
    ADD_DISPATCH_EVENT_DECL(Suspend)
    ADD_DISPATCH_EVENT_DECL(Abort)
    ADD_DISPATCH_EVENT_DECL(Error)
    ADD_DISPATCH_EVENT_DECL(Emptied)
    ADD_DISPATCH_EVENT_DECL(Stalled)
    ADD_DISPATCH_EVENT_DECL(Loadedmetadata)
    ADD_DISPATCH_EVENT_DECL(Loadeddata)
    ADD_DISPATCH_EVENT_DECL(Loadstart)
    ADD_DISPATCH_EVENT_DECL(Canplay)
    ADD_DISPATCH_EVENT_DECL(Canplaythrough)
    ADD_DISPATCH_EVENT_DECL(Playing)
    ADD_DISPATCH_EVENT_DECL(Waiting)
    ADD_DISPATCH_EVENT_DECL(Seeking)
    ADD_DISPATCH_EVENT_DECL(Seeked)
    ADD_DISPATCH_EVENT_DECL(Ended)
    ADD_DISPATCH_EVENT_DECL(Durationchange)
    ADD_DISPATCH_EVENT_DECL(Timeupdate)
    ADD_DISPATCH_EVENT_DECL(Play)
    ADD_DISPATCH_EVENT_DECL(Pause)
    ADD_DISPATCH_EVENT_DECL(Ratechange)
    ADD_DISPATCH_EVENT_DECL(Volumechange)
#undef ADD_DISPATCH_EVENT_DECL

    void giveupFetchingResource(bool shouldSetError = true);
    void setNetworkStateAsHaveNothing();
    void dispose()
    {
        closeMediaPlayer();
    }

protected:
    bool m_autoplayingFlag;
    bool m_isPaused;
    bool m_isSeeking;
    bool m_isEnded;
    bool m_delayingTheLoadEvent;
    double m_officialPlaybackPosition;
    double m_defaultPlaybackStartPosition;
    bool m_muted;
    double m_volume;
    double m_pendingSeek;
    MediaPlayer* m_mediaPlayer;
    String* m_currentSrc;
    TextTrackList* m_textTracks;
    ReadyState m_readyState;
    NetworkState m_networkState;

    MediaOperationQueueData* m_currentOperation;
    GCDeque<MediaOperationQueueData*> m_operationQueue;
    GCDeque<MediaOperationQueueData*> m_playOperationQueue;
    size_t m_currentPendingOperationCount;
    size_t m_currentPendingOperationHandle;
    ResourceSelectionContext* m_resourceSelectionContext;
    double m_currentPlayStart;
    GCAtomicVector<TimeRange> m_pastPlayed;

    void initMediaPlayer();
    void closeMediaPlayer();

    void resourceSelection();
    void dedicatedMediaSourceFailure();

    void startOperationQueueIfNeeded()
    {
        if (m_currentPendingOperationCount == 0 &&
            m_currentOperation == nullptr) {
            processNextOperationQueue();
        }
    }
    void prependToOperationQueue(MediaOperationQueueData* data)
    {
        m_operationQueue.push_front(data);
        startOperationQueueIfNeeded();
    }

    void appendToOperationQueue(MediaOperationQueueData* data)
    {
        m_operationQueue.push_back(data);
        startOperationQueueIfNeeded();
    }

    void appendToPlayOperationQueue(MediaOperationQueueData* data)
    {
        m_playOperationQueue.push_back(data);
    }

    void abortEveryPendingOperation(DOMException* exceptionForPlayPromise);

    void notifyAboutPlaying();

    bool eligibleForAutoplay()
    {
        // https://html.spec.whatwg.org/multipage/embedded-content.html#eligible-for-autoplay

        // TODO Check the element's node document's active sandboxing flag set
        //      does not have the sandboxed automatic features browsing context
        //      flag set.
        return (m_autoplayingFlag && m_isPaused && autoplay());
    }

    bool hasSourceElementChild()
    {
        Node* child = firstChild();
        while (child) {
            if (child->isHTMLSourceElement()) {
                return true;
            }
            child = child->nextSibling();
        }
        return false;
    }

private:
    DOMTokenList* m_controlsList;
};
}

#endif
