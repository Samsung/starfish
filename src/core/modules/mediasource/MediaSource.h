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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined(__StarFishMediaSource__)
#define __StarFishMediaSource__

#include "core/dom/EventTarget.h"

#ifndef STARFISH_MAX_MEDIASOURCE_BUFFERSPACE
#define STARFISH_MAX_MEDIASOURCE_BUFFERSPACE 16 * 1024 * 1024
#endif

namespace StarFish {

class SourceBuffer;
class SourceBufferList;
class StarFish;
class MediaSourceClient;

class MediaSource : public EventTarget {
    friend class SourceBuffer;

public:
    enum ReadyState {
        Closed, // Indicates the source is not currently attached to a media
                // element.
        Open,  // The source has been opened by a media element and is ready for
               // data to be appended to the SourceBuffer objects in
               // sourceBuffers.
        Ended, // The source is still attached to a media element, but
               // endOfStream() has been called.
    };

    enum EndOfStreamError {
        None,
        Network, // Terminates playback and signals that a network error has
                 // occurred.
        Decode,  // Terminates playback and signals that a decoding error has
                 // occurred.
    };

    MediaSource(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMediaSource() const override;

    SourceBuffer* addSourceBuffer(String* type);
    void removeSourceBuffer(SourceBuffer* sourceBuffer);
    void endOfStream();
    bool endOfStream(String* error)
    {
        if (error == String::emptyString) {
            endOfStream();
        } else if (error->equals("network")) {
            endOfStream(EndOfStreamError::Network);
        } else if (error->equals("decode")) {
            endOfStream(EndOfStreamError::Decode);
        } else {
            return false;
        }
        return true;
    }
    void endOfStream(EndOfStreamError error);
    void endOfStreamInternal(EndOfStreamError error);

    static bool isTypeSupported(String* type)
    {
        // TODO
        // Currently, check only whether container is mp4 or NOT
        if (type->contains("video/mp4") || type->contains("audio/mp4")) {
            return true;
        }
        return false;
    }

    String* readyState();
    ReadyState readyStateValue();

    void setReadyState(ReadyState state);

    // https://www.w3.org/TR/media-source/
    // 2.4.1 Attaching to a media element
    bool attach(HTMLMediaElement* e);

    // https://www.w3.org/TR/media-source/
    // 2.4.2 Detaching from a media element
    void detach();

    SourceBufferList* sourceBuffers();

    bool isActiveBufferComputed()
    {
        return m_isActiveBufferComputed;
    }
    SourceBufferList* activeSourceBuffers();

    double duration()
    {
        return m_duration;
    }

    uint64_t shortestMediaDuration()
    {
        return m_shortestMediaDuration;
    }

    void setDuration(double d, bool checkCurrentDuration = true);

    // TODO 2.4.4 SourceBuffer Monitoring

    void addClient(MediaSourceClient* c);
    void removeClient(MediaSourceClient* c);

    SourceBuffer* activeVideoSourceBuffer()
    {
        return m_activeVideoSourceBuffer;
    }

    size_t activeVideoStreamIndex()
    {
        return m_activeVideoStreamIndex;
    }

    SourceBuffer* activeAudioSourceBuffer()
    {
        return m_activeAudioSourceBuffer;
    }

    size_t activeAudioStreamIndex()
    {
        return m_activeAudioStreamIndex;
    }

    HTMLMediaElement* attachedMediaElement()
    {
        return m_attachedMediaElement;
    }

    size_t availableBufferSize()
    {
        return bufferFull() ? 0 : STARFISH_MAX_MEDIASOURCE_BUFFERSPACE -
                                      m_usedBufferSize;
    }

    bool bufferFull()
    {
        return m_usedBufferSize >= STARFISH_MAX_MEDIASOURCE_BUFFERSPACE;
    }

    void evict(uint64_t start, uint64_t end);

protected:
    bool anySourceBufferInUpdatingState();
    void didSourceBufferUpdated(SourceBuffer* src);
    ReadyState m_readyState;
    bool m_isActiveBufferComputed;
    ScriptBindingInstance* m_scriptBindingInstance;
    HTMLMediaElement* m_attachedMediaElement;
    SourceBuffer* m_activeVideoSourceBuffer;
    size_t m_activeVideoStreamIndex;
    SourceBuffer* m_activeAudioSourceBuffer;
    size_t m_activeAudioStreamIndex;
    SourceBufferList* m_sourceBuffers;
    SourceBufferList* m_activeSourceBuffers;
    GCVector<MediaSourceClient*> m_clients;
    double m_duration;
    uint64_t m_shortestMediaDuration;
    size_t m_usedBufferSize;
};

class MediaSourceClient : public gc {
public:
    MediaSourceClient()
        : m_mediaSource(nullptr)
    {
    }

    virtual void activeSourceComputed()
    {
    }

    virtual void activeVideoSourceBufferUpdated(SourceBuffer* s)
    {
    }

    virtual void activeAudioSourceBufferUpdated(SourceBuffer* s)
    {
    }

    void setMediaSource(MediaSource* ms)
    {
        m_mediaSource = ms;
    }

    void removeMediaSource()
    {
        m_mediaSource = nullptr;
    }

protected:
    MediaSource* m_mediaSource;
};
}

#endif
