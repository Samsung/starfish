/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishStereoPannerNode__
#define __StarfishStereoPannerNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParam;

struct StereoPannerOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(float, pan, Pan)
    float m_pan{ 0 };
};

class StereoPannerNode : public AudioNode {
public:
    StereoPannerNode(ExecutionContext* executionContext,
                     BaseAudioContext* context,
                     StereoPannerOptions options = StereoPannerOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(StereoPannerNode)
    DEFINE_GETTER(AudioParam*, pan)

    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* mode) override;

private:
    AudioParam* m_pan{ nullptr };
};

} // namespace Starfish
#endif
#endif
