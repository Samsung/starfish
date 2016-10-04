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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishMediaPlayer__)
#define __StarFishMediaPlayer__

#define STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS 300
#define STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS 150

#include "util/URL.h"

namespace StarFish {

class Document;
class URL;
class PlayerWindowData;
class HTMLElement;
class MediaPlayer;
class Canvas;

class MediaPlayerOperationQueueData : public gc {
public:
    enum EventType {
        SetURLEventType,
        RequestPrepareEventType,
        RequestPlayEventType,
        RequestPauseEventType,
    };
    MediaPlayerOperationQueueData(MediaPlayer* p)
        : m_mediaPlayer(p)
    {
    }
    virtual ~MediaPlayerOperationQueueData() { }
    virtual EventType eventType() = 0;

    MediaPlayer* m_mediaPlayer;
};

class MediaPlayerOperationQueueDataSetURL : public MediaPlayerOperationQueueData {
public:
    MediaPlayerOperationQueueDataSetURL(MediaPlayer* p, URL* u)
        : MediaPlayerOperationQueueData(p)
        , m_url(u)
    {
    }

    virtual EventType eventType()
    {
        return EventType::SetURLEventType;
    }

    URL* m_url;
};

class MediaPlayerOperationQueueDataRequestPrepare : public MediaPlayerOperationQueueData {
public:
    MediaPlayerOperationQueueDataRequestPrepare(MediaPlayer* p)
        : MediaPlayerOperationQueueData(p)
    {
    }

    virtual EventType eventType()
    {
        return EventType::RequestPrepareEventType;
    }
};

class MediaPlayerOperationQueueDataRequestPlay : public MediaPlayerOperationQueueData {
public:
    MediaPlayerOperationQueueDataRequestPlay(MediaPlayer* p)
        : MediaPlayerOperationQueueData(p)
    {
    }

    virtual EventType eventType()
    {
        return EventType::RequestPlayEventType;
    }
};

class MediaPlayerOperationQueueDataRequestPause : public MediaPlayerOperationQueueData {
public:
    MediaPlayerOperationQueueDataRequestPause(MediaPlayer* p)
        : MediaPlayerOperationQueueData(p)
    {
    }

    virtual EventType eventType()
    {
        return EventType::RequestPauseEventType;
    }
};

typedef std::list<MediaPlayerOperationQueueData*, gc_allocator<MediaPlayerOperationQueueData*>> MediaPlayerOperationQueue;

class MediaPlayer : public gc {
public:
    enum State {
        STATE_NONE = 1 << 0,
        STATE_PLAYING = 1 << 1,
        STATE_PAUSED = 1 << 2,
    };

    static MediaPlayer* create(HTMLMediaElement* element);
    void close()
    {
        // TODO
    }

    void play()
    {
        m_state = State::STATE_PLAYING;
        appendToOperationQueue(new MediaPlayerOperationQueueDataRequestPlay(this));
        startOperationQueueIfNeeded();
    }

    void pause()
    {
        m_state = State::STATE_PAUSED;
        appendToOperationQueue(new MediaPlayerOperationQueueDataRequestPause(this));
        startOperationQueueIfNeeded();
    }

    void setLoop(bool loop) { m_isLooping = true; }
    bool loop()
    {
        return m_isLooping;
    }

    void setURL(URL* url)
    {
        m_state = State::STATE_NONE;
        m_url = url;
        appendToOperationQueue(new MediaPlayerOperationQueueDataSetURL(this, m_url));
        startOperationQueueIfNeeded();
    }

    void prepare()
    {
        appendToOperationQueue(new MediaPlayerOperationQueueDataRequestPrepare(this));
        startOperationQueueIfNeeded();
    }

    virtual double currentTime()
    {
        return 0;
    }

    State state()
    {
        return m_state;
    }

    URL* url() { return m_url; }

    virtual void drawVideo(Canvas* canvas, const LayoutRect& videoRect, const LayoutRect& absVideoRect)
    {
    }

    virtual unsigned long videoWidth()
    {
        return STARFISH_VIDEO_WIDTH_WHEN_VIDEO_NOT_EXISTS;
    }

    virtual unsigned long videoHeight()
    {
        return STARFISH_VIDEO_HEIGHT_WHEN_VIDEO_NOT_EXISTS;
    }

protected:
    MediaPlayer(HTMLMediaElement* element);
    virtual void processOperationQueue(MediaPlayerOperationQueueData*)
    {
        processNextOperationQueue();
    }
    void processNextOperationQueue();
    void startOperationQueueIfNeeded()
    {
        if (m_currentPendingOperationCount == 0) {
            processNextOperationQueue();
        }
    }
    void prependToOperationQueue(MediaPlayerOperationQueueData* data)
    {
        m_operationQueue.push_front(data);
    }

    void appendToOperationQueue(MediaPlayerOperationQueueData* data)
    {
        m_operationQueue.push_back(data);
    }

    bool m_isLooping;
    bool m_hasVideo;
    State m_state;
    size_t m_currentPendingOperationCount;
    HTMLMediaElement* m_container;
    StarFish* m_starFish;
    URL* m_url;
    MediaPlayerOperationQueue m_operationQueue;
};

}

#endif
