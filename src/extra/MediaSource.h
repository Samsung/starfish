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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishMediaSource__)
#define __StarFishMediaSource__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"

namespace StarFish {

class SourceBuffer;
class SourceBufferList;
class MediaSourceClient;

class MediaSource : public EventTarget {
    friend class SourceBuffer;
public:
    enum ReadyState {
        Closed, // Indicates the source is not currently attached to a media element.
        Open, // The source has been opened by a media element and is ready for data to be appended to the SourceBuffer objects in sourceBuffers.
        Ended, // The source is still attached to a media element, but endOfStream() has been called.
    };

    enum EndOfStreamError {
        None,
        Network, // Terminates playback and signals that a network error has occured.
        Decode, // Terminates playback and signals that a decoding error has occured.
    };

    MediaSource(StarFish* starFish);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::MediaSourceObject;
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    SourceBuffer* addSourceBuffer(String* type);
    void removeSourceBuffer(SourceBuffer* sourceBuffer);
    void endOfStream();
    void endOfStream(EndOfStreamError error);
    bool endOfStream(String* error)
    {
        if (error == String::emptyString)
            endOfStream();
        else if (error->equals("network"))
            endOfStream(EndOfStreamError::Network);
        else if (error->equals("decode"))
            endOfStream(EndOfStreamError::Decode);
        else
            return false;
        return true;
    }

    static bool isTypeSupported(String* type)
    {
        // TODO
        return true;
    }

    ReadyState readyState()
    {
        return m_readyState;
    }

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

    void setDuration(double d);

    // TODO 2.4.4 SourceBuffer Monitoring

    void addClient(MediaSourceClient* c)
    {
        m_clients.push_back(c);
    }

    void removeClient(MediaSourceClient* c)
    {
        m_clients.erase(std::find(m_clients.begin(), m_clients.end(), c));
    }

    SourceBuffer* activeVideoSourceBuffer()
    {
        return m_activeVideoSourceBuffer;
    }

    size_t activeVideoStreamInSourceBuffer()
    {
        return m_activeVideoStreamInSourceBuffer;
    }

    size_t activeVideoStreamIndex()
    {
        return m_activeVideoStreamIndex;
    }

    SourceBuffer* activeAudioSourceBuffer()
    {
        return m_activeAudioSourceBuffer;
    }

    size_t activeAudioStreamInSourceBuffer()
    {
        return m_activeAudioStreamInSourceBuffer;
    }

    size_t activeAudioStreamIndex()
    {
        return m_activeAudioStreamIndex;
    }

    HTMLMediaElement* attachedMediaElement()
    {
        return m_attachedMediaElement;
    }

protected:
    bool anySourceBufferInUpdatingState();
    void didSourceBufferUpdated(SourceBuffer* src);
    ReadyState m_readyState;
    bool m_isActiveBufferComputed;
    HTMLMediaElement* m_attachedMediaElement;
    SourceBuffer* m_activeVideoSourceBuffer;
    size_t m_activeVideoStreamInSourceBuffer;
    size_t m_activeVideoStreamIndex;
    SourceBuffer* m_activeAudioSourceBuffer;
    size_t m_activeAudioStreamInSourceBuffer;
    size_t m_activeAudioStreamIndex;
    StarFish* m_starFish;
    SourceBufferList* m_sourceBuffers;
    SourceBufferList* m_activeSourceBuffers;
    std::vector<MediaSourceClient*, gc_allocator<MediaSourceClient*>> m_clients;
    double m_duration;
};

class MediaSourceClient : public gc {
public:
    MediaSourceClient(MediaSource* ms)
        : m_mediaSource(ms)
    {
    }

    virtual void activeSourceComputed()
    {

    }

protected:
    MediaSource* m_mediaSource;
};


}

#endif
