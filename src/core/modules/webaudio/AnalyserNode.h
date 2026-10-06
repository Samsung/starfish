/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAnalyserNode__
#define __StarfishAnalyserNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class AnalyserHandler;

struct AnalyserOptions : public AudioNodeOptions {
    DEFINE_GETTER_SETTER(uint32_t, fftSize, FftSize)
    DEFINE_GETTER_SETTER(double, maxDecibels, MaxDecibels)
    DEFINE_GETTER_SETTER(double, minDecibels, MinDecibels)
    DEFINE_GETTER_SETTER(double, smoothingTimeConstant, SmoothingTimeConstant)

    uint32_t m_fftSize{ 2048 };
    double m_maxDecibels{ -30 };
    double m_minDecibels{ -100 };
    double m_smoothingTimeConstant{ 0.8 };
};

class AnalyserNode : public AudioNode {
public:
    AnalyserNode(ExecutionContext* executionContext, BaseAudioContext* context,
                 AnalyserOptions options = AnalyserOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AnalyserNode)
    void getFloatFrequencyData(ScriptFloat32Array array);
    void getByteFrequencyData(ScriptUint8Array array);
    void getFloatTimeDomainData(ScriptFloat32Array array);
    void getByteTimeDomainData(ScriptUint8Array array);

    DEFINE_GETTER(uint32_t, fftSize)
    void setFftSize(uint32_t value);
    uint32_t frequencyBinCount() const
    {
        return m_fftSize / 2;
    }
    DEFINE_GETTER(double, minDecibels)
    void setMinDecibels(double value);
    DEFINE_GETTER(double, maxDecibels)
    void setMaxDecibels(double value);
    DEFINE_GETTER(double, smoothingTimeConstant)
    void setSmoothingTimeConstant(double value);

private:
    AnalyserHandler* m_analyserHandler{ nullptr };
    uint32_t m_fftSize{ 2048 };
    double m_minDecibels{ -100 };
    double m_maxDecibels{ -30 };
    double m_smoothingTimeConstant{ 0.8 };
};

} // namespace Starfish
#endif
#endif
