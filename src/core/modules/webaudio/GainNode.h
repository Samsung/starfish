/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishGainNode__
#define __StarfishGainNode__

#include "core/modules/webaudio/AudioNode.h"
#include "core/modules/webaudio/AudioParam.h"

namespace Starfish {

struct GainOptions {
    DEFINE_GETTER_SETTER(float, gain, Gain)
    float m_gain{ 1.0f };
};

class GainNode : public AudioNode {
public:
    GainNode(ExecutionContext* executionContext, BaseAudioContext* context);
    GainNode(ExecutionContext* executionContext, BaseAudioContext* context,
             GainOptions options);
    virtual ~GainNode()
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(GainNode)

    AudioParam* gain() const
    {
        return m_gain;
    }

private:
    AudioParam* m_gain{ nullptr };
};

} // namespace Starfish

#endif
#endif
