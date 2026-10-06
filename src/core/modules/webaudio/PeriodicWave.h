/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishPeriodicWave__
#define __StarfishPeriodicWave__

#include "binding/ScriptWrappable.h"

namespace Starfish {
class BaseAudioContext;
class ExecutionContext;
class PeriodicWaveData;

struct PeriodicWaveConstraints {
    DEFINE_GETTER_SETTER(bool, disableNormalization, DisableNormalization)
    bool m_disableNormalization{ false };
};

struct PeriodicWaveOptions : public PeriodicWaveConstraints {
    void setReal(const GCAtomicVector<double>& real)
    {
        m_real = real;
        m_hasReal = true;
    }
    void setImag(const GCAtomicVector<double>& imag)
    {
        m_imag = imag;
        m_hasImag = true;
    }
    const GCAtomicVector<double>& real() const
    {
        return m_real;
    }
    const GCAtomicVector<double>& imag() const
    {
        return m_imag;
    }
    bool hasReal() const
    {
        return m_hasReal;
    }
    bool hasImag() const
    {
        return m_hasImag;
    }

private:
    GCAtomicVector<double> m_real;
    GCAtomicVector<double> m_imag;
    bool m_hasReal{ false };
    bool m_hasImag{ false };
};

class PeriodicWave : public ScriptWrappable {
public:
    PeriodicWave(ExecutionContext* executionContext, BaseAudioContext* context,
                 PeriodicWaveOptions options = PeriodicWaveOptions());
    ~PeriodicWave();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PeriodicWave)

    BaseAudioContext* context() const
    {
        return m_context;
    }
    PeriodicWaveData* data() const
    {
        return m_data;
    }

private:
    ExecutionContext* m_executionContext;
    BaseAudioContext* m_context;
    PeriodicWaveData* m_data{ nullptr };
};

} // namespace Starfish
#endif
#endif
