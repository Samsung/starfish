/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishConstantSourceNode__
#define __StarfishConstantSourceNode__

#include "core/modules/webaudio/AudioScheduledSourceNode.h"

namespace Starfish {
class AudioParam;
class ConstantSourceHandler;

struct ConstantSourceOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(float, offset, Offset)
    float m_offset{ 1 };
};

class ConstantSourceNode : public AudioScheduledSourceNode {
public:
    ConstantSourceNode(ExecutionContext* executionContext,
                       BaseAudioContext* context,
                       ConstantSourceOptions options = ConstantSourceOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(ConstantSourceNode)
    DEFINE_GETTER(AudioParam*, offset)

    void start(double when = 0) override;
    void stop(double when = 0) override;
    bool renderScheduledQuantum() override;

private:
    AudioParam* m_offset{ nullptr };
    ConstantSourceHandler* m_sourceHandler{ nullptr };
};

} // namespace Starfish
#endif
#endif
