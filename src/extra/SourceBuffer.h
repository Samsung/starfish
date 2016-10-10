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

namespace StarFish {

class AudioTrackList;
class VideoTrackList;
class TextTrackList;
class TimeRanges;
class MediaSource;

struct SourceBufferData : public gc {
    bool m_isProcessed;
    uint64_t m_groupStartTimestamp;
    uint64_t m_groupEndTimestamp;

    const uint8_t* m_data;
    unsigned long m_length;
    SourceBufferData(const uint8_t* data, unsigned long length)
        : m_isProcessed(false)
        , m_groupStartTimestamp(0)
        , m_groupEndTimestamp(0)
        , m_data(data)
        , m_length(length)
    {
    }
};

class SourceBuffer : public EventTarget {
public:
    friend class MediaSource;
    friend class SourceBufferList;
    enum UpdateState {
        Success,
        Error,
        Abort,
    };

    enum AppendMode {
        Segments,
        Sequence,
    };

    enum AppendState {
        WaitingForSegment,
        ParsingInitSegment,
        ParsingMediaSegment
    };

    SourceBuffer(StarFish* starFish, String* type);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::SourceBufferObject;
    }

    void appendBuffer(const uint8_t* data, unsigned long length);
    void abort();
    void remove(double start, double end);

    MediaSource* parentMediaSource() { return m_parentMediaSource; }

    void setMode(AppendMode mode)
    {
        m_mode = mode;
    }

    AppendMode mode()
    {
        return m_mode;
    }

protected:
    void setUpdating(bool flag, UpdateState state);

    void attachedToParent(MediaSource* ms)
    {
        STARFISH_ASSERT(m_isAttachedToParent == false);
        m_parentMediaSource = ms;
        m_isAttachedToParent = true;
    }

    void detachFromParent()
    {
        m_parentMediaSource = nullptr;
        m_isAttachedToParent = false;
    }

    void prepareAppend();
    void codedFrameEviction();
    void bufferAppend(SourceBufferData* data);

    AppendMode m_mode;
    AppendState m_state;
    bool m_isAttachedToParent;
    bool m_updating;
    StarFish* m_starFish;
    TimeRanges* buffered;
    double m_timestampOffset;
    AudioTrackList* m_audioTracks;
    VideoTrackList* m_videoTracks;
    TextTrackList* m_textTracks;
    double m_appendWindowStart;
    double m_appendWindowEnd;
    String* m_type;
    MediaSource* m_parentMediaSource;
    std::vector<SourceBufferData*, gc_allocator<SourceBufferData*>> m_sourceBufferDataList;
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

    void add(SourceBuffer* buffer, MediaSource* ms)
    {
        m_list.push_back(buffer);
        buffer->attachedToParent(ms);
    }

    void remove(unsigned long index)
    {
        SourceBuffer* buf = m_list[index];
        m_list.erase(m_list.begin() + index);
        buf->detachFromParent();
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
