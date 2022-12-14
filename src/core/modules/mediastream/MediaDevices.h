/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishMediaDevices__
#define __StarfishMediaDevices__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"
#include "binding/generated/booleanOrMediaTrackConstraintsUnion.h"

namespace Starfish {
class ExecutionContext;

struct MediaStreamConstraints {
    DEFINE_GETTER_SETTER(booleanOrMediaTrackConstraints, video, Video)
    DEFINE_GETTER_SETTER(booleanOrMediaTrackConstraints, audio, Audio)

    booleanOrMediaTrackConstraints m_video;
    booleanOrMediaTrackConstraints m_audio;
};

class MediaDevices : public EventTarget, public DocumentHoldable {
public:
    MediaDevices(ExecutionContext* executionContext, Document* document);
    virtual ~MediaDevices();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaDevices)
    virtual ExecutionContext* executionContext() const override;

    Promise* getUserMedia(
        MediaStreamConstraints constraints = MediaStreamConstraints());

private:
    ExecutionContext* m_executionContext{ nullptr };
};
} // namespace Starfish

#endif
#endif
