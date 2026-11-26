/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#ifndef __StarfishAudioPCMResampler__
#define __StarfishAudioPCMResampler__

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Starfish {

class AudioBufferDataBuilder;

// Converts planar float PCM in bounded blocks. The caller fills the planes
// from prepareInput(), then commits the block with append().
class AudioPCMResampler {
public:
    AudioPCMResampler(unsigned inputRate, unsigned outputRate, size_t channels);
    void prepareInput(size_t frames, float** planes);
    bool append(size_t frames, AudioBufferDataBuilder& output);
    bool drain(AudioBufferDataBuilder& output);

private:
    void makeWeights(unsigned phase, float* weights) const;
    bool writeOutput(uint64_t end, AudioBufferDataBuilder& output);
    void discardConsumedInput();

    const unsigned m_step;
    const unsigned m_phaseCount;
    const double m_cutoff;
    const size_t m_radius;
    std::vector<std::vector<float>> m_input;
    std::vector<float> m_weights;
    unsigned m_phase{ 0 };
    uint64_t m_sourceFrame{ 0 };
    uint64_t m_inputStart{ 0 };
    uint64_t m_inputFrames{ 0 };
    uint64_t m_outputFrames{ 0 };
};

} // namespace Starfish

#endif
