/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishPeriodicWaveData__
#define __StarfishPeriodicWaveData__

#include <atomic>
#include <cstddef>
#include <vector>

namespace Starfish {

// The render graph holds this non-GC object, never a PeriodicWave script
// object.
class PeriodicWaveData {
public:
    struct Partial {
        size_t harmonic;
        double real;
        double imag;
    };

    PeriodicWaveData(const std::vector<Partial>& partials,
                     bool disableNormalization);
    void retain();
    void release();
    void prepare();
    double sample(double phase, size_t maxHarmonic) const;

private:
    ~PeriodicWaveData() = default;

    std::atomic<unsigned> m_references{ 1 };
    std::vector<Partial> m_partials;
    std::vector<float> m_table;
    double m_scale{ 1 };
    bool m_normalize;
    bool m_prepared{ false };
};

} // namespace Starfish

#endif
#endif
