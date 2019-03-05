/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishMediaQueryListEvent__
#define __StarfishMediaQueryListEvent__

#include "Event.h"

namespace Starfish {

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
    MediaQueryListEvent(ExecutionContext* executionContext,
                        String* eventType = String::emptyString,
                        const MediaQueryListEventInit& init =
                            MediaQueryListEventInit(String::emptyString, false))
        : Event(executionContext, eventType)
        , m_media(init.media())
        , m_matches(init.matches())
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaQueryListEvent)

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
