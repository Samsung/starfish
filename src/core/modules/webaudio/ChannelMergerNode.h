/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishChannelMergerNode__
#define __StarfishChannelMergerNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {

struct ChannelMergerOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(uint32_t, numberOfInputs, NumberOfInputs)
    uint32_t m_numberOfInputs{ 6 };
};

class ChannelMergerNode : public AudioNode {
public:
    ChannelMergerNode(ExecutionContext* executionContext,
                      BaseAudioContext* context,
                      ChannelMergerOptions options = ChannelMergerOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ChannelMergerNode)

    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* mode) override;
};

} // namespace Starfish
#endif
#endif
