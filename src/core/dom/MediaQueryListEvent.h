/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishMediaQueryListEvent__
#define __StarFishMediaQueryListEvent__

#include "Event.h"

namespace StarFish {

// https://drafts.csswg.org/cssom-view/#mediaquerylistevent
struct MediaQueryListEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    MediaQueryListEventInit(String* media = String::emptyString,
                            bool matches = false)
        : EventInit(false, false)
        , m_media(media)
        , m_matches(matches)
    {
    }

    String* media() const
    {
        return m_media;
    }

    void setMedia(String* media)
    {
        m_media = media;
    }

    bool matches() const
    {
        return m_matches;
    }

    void setMatches(bool matches)
    {
        m_matches = matches;
    }

private:
    String* m_media;
    bool m_matches;
};

class MediaQueryListEvent : public Event {
public:
    MediaQueryListEvent(Document* document,
                        String* eventType = String::emptyString,
                        const MediaQueryListEventInit& init =
                            MediaQueryListEventInit(String::emptyString, false))
        : Event(document, eventType)
        , m_media(init.media())
        , m_matches(init.matches())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isMediaQueryListEvent() const override;

    String* media() const
    {
        return m_media;
    }

    bool matches() const
    {
        return m_matches;
    }

private:
    String* m_media;
    bool m_matches;
};
}

#endif
