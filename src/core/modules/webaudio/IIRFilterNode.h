/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishIIRFilterNode__
#define __StarfishIIRFilterNode__

#include "core/modules/webaudio/AudioNode.h"

namespace Starfish {

struct IIRFilterOptions : public AudioNodeOptions {
    void setFeedforward(const GCAtomicVector<double>& values)
    {
        m_feedforward = values;
    }
    void setFeedback(const GCAtomicVector<double>& values)
    {
        m_feedback = values;
    }
    const GCAtomicVector<double>& feedforward() const
    {
        return m_feedforward;
    }
    const GCAtomicVector<double>& feedback() const
    {
        return m_feedback;
    }

private:
    GCAtomicVector<double> m_feedforward;
    GCAtomicVector<double> m_feedback;
};

class IIRFilterNode : public AudioNode {
public:
    IIRFilterNode(ExecutionContext* executionContext, BaseAudioContext* context,
                  IIRFilterOptions options);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(IIRFilterNode)

    void getFrequencyResponse(ScriptFloat32Array frequencyHz,
                              ScriptFloat32Array magResponse,
                              ScriptFloat32Array phaseResponse);
};

} // namespace Starfish
#endif
#endif
