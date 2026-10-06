/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishOscillatorNode__
#define __StarfishOscillatorNode__

#include "core/modules/webaudio/AudioScheduledSourceNode.h"

namespace Starfish {
class AudioParam;
class OscillatorHandler;
class PeriodicWave;

struct OscillatorOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(String*, type, Type)
    DEFINE_GETTER_SETTER(float, frequency, Frequency)
    DEFINE_GETTER_SETTER(float, detune, Detune)
    DEFINE_GETTER_SETTER(PeriodicWave*, periodicWave, PeriodicWave)

    String* m_type{ String::createASCIIString("sine") };
    float m_frequency{ 440 };
    float m_detune{ 0 };
    PeriodicWave* m_periodicWave{ nullptr };
};

class OscillatorNode : public AudioScheduledSourceNode {
public:
    OscillatorNode(ExecutionContext* executionContext,
                   BaseAudioContext* context,
                   OscillatorOptions options = OscillatorOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(OscillatorNode)
    DEFINE_GETTER(String*, type)
    void setType(String* type);
    DEFINE_GETTER(AudioParam*, frequency)
    DEFINE_GETTER(AudioParam*, detune)
    void setPeriodicWave(PeriodicWave* periodicWave);

    void start(double when = 0) override;
    void stop(double when = 0) override;
    bool renderScheduledQuantum() override;

private:
    String* m_type{ nullptr };
    AudioParam* m_frequency{ nullptr };
    AudioParam* m_detune{ nullptr };
    OscillatorHandler* m_sourceHandler{ nullptr };
    PeriodicWave* m_periodicWave{ nullptr };
};

} // namespace Starfish
#endif
#endif
