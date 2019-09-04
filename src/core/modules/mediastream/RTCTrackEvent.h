/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishRTCTrackEvent__
#define __StarfishRTCTrackEvent__

#include "core/dom/Event.h"
#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"
#include "core/util/String.h"

namespace Starfish {
class ExecutionContext;
class MediaStream;
class MediaStreamTrack;

struct RTCTrackEventInit : public EventInit {
    friend class RTCTrackEvent;

public:
    RTCTrackEventInit()
        : EventInit()
    {
    }

    RTCTrackEventInit(MediaStreamTrack* track, GCVector<MediaStream*> streams)
        : EventInit()
        , m_track(track)
        , m_streams(streams)
    {
    }

    DEFINE_GETTER_SETTER(MediaStreamTrack*, track, Track);
    DEFINE_GETTER_SETTER(GCVector<MediaStream*>, streams, Streams);

private:
    MediaStreamTrack* m_track{ nullptr };
    GCVector<MediaStream*> m_streams;
};

class RTCTrackEvent : public Event {
public:
    RTCTrackEvent(ExecutionContext* executionContext)
        : Event(executionContext)
    {
    }

    RTCTrackEvent(ExecutionContext* executionContext, String* type,
                  RTCTrackEventInit eventInitDict)
        : Event(executionContext, type)
        , m_track(eventInitDict.m_track)
        , m_streams(eventInitDict.m_streams)
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCTrackEvent)

    DEFINE_GETTER(MediaStreamTrack*, track);
    DEFINE_GETTER(GCVector<MediaStream*>, streams);

private:
    MediaStreamTrack* m_track{ nullptr };
    GCVector<MediaStream*> m_streams;
};
}
#endif
#endif
