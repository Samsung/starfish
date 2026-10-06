/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishBiquadFilterNode__
#define __StarfishBiquadFilterNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AudioParam;
class BiquadFilterHandler;

struct BiquadFilterOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(String*, type, Type)
    DEFINE_GETTER_SETTER(float, Q, Q)
    DEFINE_GETTER_SETTER(float, detune, Detune)
    DEFINE_GETTER_SETTER(float, frequency, Frequency)
    DEFINE_GETTER_SETTER(float, gain, Gain)

    String* m_type{ String::createASCIIString("lowpass") };
    float m_Q{ 1 };
    float m_detune{ 0 };
    float m_frequency{ 350 };
    float m_gain{ 0 };
};

class BiquadFilterNode : public AudioNode {
public:
    BiquadFilterNode(ExecutionContext* executionContext,
                     BaseAudioContext* context,
                     BiquadFilterOptions options = BiquadFilterOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(BiquadFilterNode)
    DEFINE_GETTER(String*, type)
    void setType(String* type);
    DEFINE_GETTER(AudioParam*, frequency)
    DEFINE_GETTER(AudioParam*, detune)
    DEFINE_GETTER(AudioParam*, Q)
    DEFINE_GETTER(AudioParam*, gain)
    void getFrequencyResponse(ScriptFloat32Array frequencyHz,
                              ScriptFloat32Array magResponse,
                              ScriptFloat32Array phaseResponse);

private:
    String* m_type{ nullptr };
    AudioParam* m_frequency{ nullptr };
    AudioParam* m_detune{ nullptr };
    AudioParam* m_Q{ nullptr };
    AudioParam* m_gain{ nullptr };
    BiquadFilterHandler* m_filterHandler{ nullptr };
};

} // namespace Starfish
#endif
#endif
