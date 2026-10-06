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

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishMediaElementAudioSourceNode__
#define __StarfishMediaElementAudioSourceNode__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class ExecutionContext;
class HTMLMediaElement;
class AudioContext;
class MediaAudioPlaybackState;
class MediaElementSourceHandler;

struct MediaElementAudioSourceOptions {
    DEFINE_GETTER_SETTER(HTMLMediaElement*, mediaElement, MediaElement);

    HTMLMediaElement* m_mediaElement{ nullptr };
};

class MediaElementAudioSourceNode : public AudioNode {
public:
    MediaElementAudioSourceNode(ExecutionContext* executionContext,
                                AudioContext* context,
                                MediaElementAudioSourceOptions options);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(MediaElementAudioSourceNode);

    DEFINE_GETTER(HTMLMediaElement*, mediaElement);
    void setPlaybackState(MediaAudioPlaybackState* state);

private:
    HTMLMediaElement* m_mediaElement{ nullptr };
    MediaElementSourceHandler* m_sourceHandler{ nullptr };
};
} // namespace Starfish
#endif
#endif
