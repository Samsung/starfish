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

#ifndef __StarfishAudioBufferSourceNode__
#define __StarfishAudioBufferSourceNode__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"

#include "core/modules/webaudio/AudioScheduledSourceNode.h"

namespace Starfish {
class ExecutionContext;
class BaseAudioContext;
class AudioBuffer;

struct AudioBufferSourceOptions {
    DEFINE_GETTER_SETTER(AudioBuffer*, buffer, Buffer);

    AudioBuffer* m_buffer{ nullptr };
};

class AudioBufferSourceNode : public AudioScheduledSourceNode {
public:
    AudioBufferSourceNode(
        ExecutionContext* executionContext, BaseAudioContext* context,
        AudioBufferSourceOptions options = AudioBufferSourceOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AudioBufferSourceNode)

    DEFINE_GETTER(AudioBuffer*, buffer);
    void setBuffer(AudioBuffer* buffer);

    void start(double when = 0, double offset = 0, double duration = 0);

private:
    AudioBuffer* m_buffer{ nullptr };
    bool m_bufferSet{ false };
};
} // namespace Starfish
#endif
#endif
