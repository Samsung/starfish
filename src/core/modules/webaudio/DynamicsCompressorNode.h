/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishDynamicsCompressorNode__
#define __StarfishDynamicsCompressorNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParam;
class DynamicsCompressorHandler;

struct DynamicsCompressorOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(float, attack, Attack)
    DEFINE_GETTER_SETTER(float, knee, Knee)
    DEFINE_GETTER_SETTER(float, ratio, Ratio)
    DEFINE_GETTER_SETTER(float, release, Release)
    DEFINE_GETTER_SETTER(float, threshold, Threshold)

    float m_attack{ 0.003f };
    float m_knee{ 30 };
    float m_ratio{ 12 };
    float m_release{ 0.25f };
    float m_threshold{ -24 };
};

class DynamicsCompressorNode : public AudioNode {
public:
    DynamicsCompressorNode(
        ExecutionContext* executionContext, BaseAudioContext* context,
        DynamicsCompressorOptions options = DynamicsCompressorOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DynamicsCompressorNode)
    DEFINE_GETTER(AudioParam*, threshold)
    DEFINE_GETTER(AudioParam*, knee)
    DEFINE_GETTER(AudioParam*, ratio)
    float reduction() const;
    DEFINE_GETTER(AudioParam*, attack)
    DEFINE_GETTER(AudioParam*, release)
    void setChannelCount(uint32_t value) override;
    void setChannelCountModeStr(String* mode) override;

private:
    AudioParam* m_threshold{ nullptr };
    AudioParam* m_knee{ nullptr };
    AudioParam* m_ratio{ nullptr };
    AudioParam* m_attack{ nullptr };
    AudioParam* m_release{ nullptr };
    DynamicsCompressorHandler* m_compressorHandler{ nullptr };
};

} // namespace Starfish
#endif
#endif
