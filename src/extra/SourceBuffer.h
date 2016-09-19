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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishSourceBuffer__)
#define __StarFishSourceBuffer__

#include "dom/binding/ScriptWrappable.h"
#include "dom/EventTarget.h"
#include "platform/multimedia/MediaSourceClient.h"

namespace StarFish {

class AudioTrackList;
class VideoTrackList;
class TextTrackList;
class TimeRanges;
class MediaSourceClient;
class MediaSource;

class SourceBuffer : public EventTarget {
public:
    enum UpdateState {
        Success,
        Error,
        Abort,
    };

    enum AppendMode {
        Segments,
        Sequence,
    };

    SourceBuffer(String* type, MediaSource* parent, MediaSourceClient* client);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::SourceBufferObject;
    }

    // data: ArrayBuffer/ArrayBufferView type
    void appendBuffer(const void* data, unsigned long length);
    // TODO
//    void appendStream(ReadableStream stream, unsigned long long maxSize);
    void abort();
    void remove(double start, double end);

    void prepareAppend(const void* data, unsigned long length);
    void runBufferAppend();
    void setUpdating(bool flag, UpdateState state);
    void dispatchUpdateEvent(UpdateState state);

    MediaSourceClient* mseClient() { return m_mseClient; }
    MediaSource* parentMediaSource() { return m_parentMediaSource; }

protected:
    AppendMode m_mode;
    bool m_updating;
    TimeRanges* buffered;
    double m_timestampOffset;
    AudioTrackList* m_audioTracks;
    VideoTrackList* m_videoTracks;
    TextTrackList* m_textTracks;
    double m_appendWindowStart;
    double m_appendWindowEnd;
    String* m_type;

    MediaSource* m_parentMediaSource;
    MediaSourceClient* m_mseClient;
    MediaRawData m_inputBuffer;
};

class SourceBufferList : public EventTarget {
public:
    SourceBufferList()
        : EventTarget()
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::SourceBufferListObject;
    }

    unsigned long length() const
    {
        return m_list.size();
    }

    void add(SourceBuffer* buffer)
    {
        m_list.push_back(buffer);
    }

    void remove(unsigned long index)
    {
        m_list.erase(m_list.begin() + index);
    }

    void remove(SourceBuffer* buffer)
    {
        unsigned long size = m_list.size();
        unsigned long targetIdx = 0;
        for (targetIdx = 0; targetIdx < size; targetIdx++) {
            if (m_list[targetIdx] == buffer)
                break;
        }
        if (targetIdx < size) {
            remove(targetIdx);
        }
    }

    SourceBuffer* at(unsigned long index)
    {
        if (index >= m_list.size())
            return nullptr;
        return m_list[index];
    }

protected:
    std::vector<SourceBuffer*, gc_allocator<SourceBuffer*>> m_list;
};

}

#endif
