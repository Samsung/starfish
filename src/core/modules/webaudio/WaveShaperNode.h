/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishWaveShaperNode__
#define __StarfishWaveShaperNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {
class WaveShaperHandler;

struct WaveShaperOptions : public AudioNodeOptions {
    void setCurve(const GCAtomicVector<double>& curve)
    {
        m_curve = curve;
        m_hasCurve = true;
    }
    const GCAtomicVector<double>& curve() const
    {
        return m_curve;
    }
    bool hasCurve() const
    {
        return m_hasCurve;
    }
    DEFINE_GETTER_SETTER(String*, oversample, Oversample)

private:
    GCAtomicVector<double> m_curve;
    bool m_hasCurve{ false };
    String* m_oversample{ String::createASCIIString("none") };
};

class WaveShaperNode : public AudioNode {
public:
    WaveShaperNode(ExecutionContext* executionContext,
                   BaseAudioContext* context,
                   WaveShaperOptions options = WaveShaperOptions());

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(WaveShaperNode)
    DEFINE_GETTER(ScriptFloat32Array, curve)
    void setCurve(ScriptFloat32Array curve);
    DEFINE_GETTER(String*, oversample)
    void setOversample(String* oversample);

private:
    ScriptFloat32Array m_curve{ nullptr };
    String* m_oversample{ String::createASCIIString("none") };
    bool m_curveSet{ false };
    WaveShaperHandler* m_shaperHandler{ nullptr };
};

} // namespace Starfish
#endif
#endif
