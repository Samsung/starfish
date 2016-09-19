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
class VideoPlayer;
class MediaSourceClient;

class MediaSource : public EventTarget {
public:
    enum ReadyState {
        Closed,
        Open,
        Ended,
    };

    enum EndOfStreamError {
        Network,
        Decode,
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

    bool isTypeSupported(String* type)
    {
        // TODO
        return true;
    }

    void registerMediaPlayer(VideoPlayer* player);
    MediaSourceClient* mseClient();
    ReadyState readyState()
    {
        return m_readyState;
    }
    void setReadyState(ReadyState state);
    void dispatchStateChangeEvent();

protected:
    ReadyState m_readyState;
    StarFish* m_starFish;
    SourceBufferList* m_sourceBuffers;
    SourceBufferList* m_activeSourceBuffers;
    double m_duration;
    MediaSourceClient* m_mseClient;

};

}

#endif
