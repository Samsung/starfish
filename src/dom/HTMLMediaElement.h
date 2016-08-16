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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && !defined (__StarFishHTMLMediaElement__)
#define __StarFishHTMLMediaElement__

#include "dom/Document.h"
#include "dom/HTMLElement.h"

namespace StarFish {

class TextTrack;
class TextTrackList;

class TimeRange {
    friend TimeRanges;
public:
    TimeRange(double start = 0, double end = 0)
        : m_start(start)
        , m_end(end)
    { }

private:
    double m_start;
    double m_end;
};

class TimeRanges : public ScriptWrappable {
public:
    TimeRanges()
        : ScriptWrappable(this)
    { }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    unsigned long length()
    {
        return m_list.size();
    }

    double start(unsigned long idx)
    {
        if (idx < m_list.size())
            return m_list[idx].m_start;
        return DBL_MAX;
    }

    double end(unsigned long idx)
    {
        if (idx < m_list.size())
            return m_list[idx].m_end;
        return DBL_MAX;
    }

private:
    std::vector<TimeRange> m_list;
};

class HTMLMediaElement : public HTMLElement {
public:
    enum NetState {
        NETWORK_EMPTY,
        NETWORK_IDLE,
        NETWORK_LOADING,
        NETWORK_NO_SOURCE,
    };

    enum ReadyState {
        HAVE_NOTHING,
        HAVE_METADATA,
        HAVE_CURRENT_DATA,
        HAVE_FUTURE_DATA,
        HAVE_ENOUGH_DATA,
    };

    HTMLMediaElement(Document* document);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isHTMLMediaElement() const
    {
        return true;
    }

    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);

    TextTrackList* textTracks()
    {
        return m_textTracks;
    }

    void addTextTrack(TextTrack* track);
    TextTrack* addTextTrack(String* kind, String* label, String* language);
    void removeTextTrack(TextTrack* track);

    void setSrc(String* src)
    {
        setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
    }

    String* src()
    {
        return getAttribute(document()->window()->starFish()->staticStrings()->m_src);
    }

    NetState networkState();
    String* preload();
    TimeRanges* buffered();
    void load();
    String* canPlayType(String* type);
    ReadyState readyState();
    bool seeking();
    double currentTime();
    double duration();
    bool paused();
    void pause();
    double defaultPlaybackRate();
    double playbackRate();
    TimeRanges* played();
    TimeRanges* seekable();
    bool ended();
    bool autoplay();
    bool loop();
    bool controls();
    bool volume();
    bool muted();

    void setPreload(String* preload);
    void setSeeking(bool seeking);
    void setCurrentTime(double currentTime);
    void setDefaultPlaybackRate(double defaultPlaybackRate);
    void setPlaybackRate(double playbackRate);
    void setAutoplay(bool autoplay);
    void setLoop(bool loop);
    void setControls(bool controls);
    void setVolume(bool volume);
    void setMuted(bool muted);

protected:
    TextTrackList* m_textTracks;
};
}

#endif
