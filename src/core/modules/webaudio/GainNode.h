/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishGainNode__
#define __StarfishGainNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParam;
class GainHandler;

struct GainOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(float, gain, Gain)
    float m_gain{ 1 };
};

class GainNode : public AudioNode {
public:
    GainNode(ExecutionContext* executionContext, BaseAudioContext* context,
             GainOptions options = GainOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(GainNode)
    DEFINE_GETTER(AudioParam*, gain)

private:
    AudioParam* m_gain{ nullptr };
};

} // namespace Starfish
#endif
#endif
