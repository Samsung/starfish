/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishChannelSplitterNode__
#define __StarfishChannelSplitterNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {

struct ChannelSplitterOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(uint32_t, numberOfOutputs, NumberOfOutputs)
    uint32_t m_numberOfOutputs{ 6 };
};

class ChannelSplitterNode : public AudioNode {
public:
    ChannelSplitterNode(
        ExecutionContext* executionContext, BaseAudioContext* context,
        ChannelSplitterOptions options = ChannelSplitterOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ChannelSplitterNode)

    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* mode) override;
    void setChannelInterpretationStr(String* interpretation) override;
};

} // namespace Starfish
#endif
#endif
