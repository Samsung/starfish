/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishDelayNode__
#define __StarfishDelayNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParam;

struct DelayOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(double, maxDelayTime, MaxDelayTime)
    DEFINE_GETTER_SETTER(double, delayTime, DelayTime)
    double m_maxDelayTime{ 1 };
    double m_delayTime{ 0 };
};

class DelayNode : public AudioNode {
public:
    DelayNode(ExecutionContext* executionContext, BaseAudioContext* context,
              DelayOptions options = DelayOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DelayNode)
    DEFINE_GETTER(AudioParam*, delayTime)

private:
    AudioParam* m_delayTime{ nullptr };
};

} // namespace Starfish
#endif
#endif
