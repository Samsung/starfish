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
class RTCRtpReceiver;
class RTCRtpTransceiver;

struct RTCTrackEventInit : public EventInit {
    friend class RTCTrackEvent;

public:
    RTCTrackEventInit()
        : EventInit()
    {
    }

    RTCTrackEventInit(RTCRtpReceiver* receiver, MediaStreamTrack* track,
                      GCVector<MediaStream*> streams,
                      RTCRtpTransceiver* transceiver)
        : EventInit()
        , m_receiver(receiver)
        , m_track(track)
        , m_streams(streams)
        , m_transceiver(transceiver)
    {
    }

    DEFINE_GETTER_SETTER(RTCRtpReceiver*, receiver, Receiver);
    DEFINE_GETTER_SETTER(MediaStreamTrack*, track, Track);
    DEFINE_GETTER_SETTER(GCVector<MediaStream*>, streams, Streams);
    DEFINE_GETTER_SETTER(RTCRtpTransceiver*, transceiver, Transceiver);

private:
    RTCRtpReceiver* m_receiver{ nullptr };
    MediaStreamTrack* m_track{ nullptr };
    GCVector<MediaStream*> m_streams;
    RTCRtpTransceiver* m_transceiver{ nullptr };
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
        , m_receiver(eventInitDict.m_receiver)
        , m_track(eventInitDict.m_track)
        , m_streams(eventInitDict.m_streams)
        , m_transceiver(eventInitDict.m_transceiver)
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCTrackEvent)

    DEFINE_GETTER(RTCRtpReceiver*, receiver);
    DEFINE_GETTER(MediaStreamTrack*, track);
    DEFINE_GETTER(GCVector<MediaStream*>, streams);
    DEFINE_GETTER(RTCRtpTransceiver*, transceiver);

private:
    RTCRtpReceiver* m_receiver{ nullptr };
    MediaStreamTrack* m_track{ nullptr };
    GCVector<MediaStream*> m_streams;
    RTCRtpTransceiver* m_transceiver{ nullptr };
};
}
#endif
#endif
