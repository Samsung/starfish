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

#ifndef __StarfishAudioDestinationNode__
#define __StarfishAudioDestinationNode__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class ExecutionContext;
class MediaPlayerAudio;

class AudioDestinationNode : public AudioNode {
public:
    AudioDestinationNode(ExecutionContext* executionContext,
                         BaseAudioContext* context);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioDestinationNode)

    DEFINE_GETTER_SETTER(uint32_t, maxChannelCount, MaxChannelCount)

    void setBuffer(uint8_t* buffer, uint32_t length);
    void play();

private:
    AudioDestinationNode(ExecutionContext* executionContext);

    uint32_t m_maxChannelCount{ 0 };
    MediaPlayerAudio* m_player{ nullptr };
};
} // namespace Starfish
#endif
#endif
