/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)

#include "core/modules/webaudio/render/AudioPCMResampler.h"
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

#if defined(__ARM_NEON_FP)
#include <arm_neon.h>
#endif

namespace Starfish {
namespace {

    constexpr double Pi = 3.14159265358979323846;
    constexpr size_t ZeroCrossings = 16;
    constexpr size_t KernelStepsPerSample = 128;
    constexpr size_t KernelSize = ZeroCrossings * KernelStepsPerSample;
    constexpr size_t MaxCachedWeights = 16384;

    unsigned greatestCommonDivisor(unsigned a, unsigned b)
    {
        while (b) {
            unsigned remainder = a % b;
            a = b;
            b = remainder;
        }
        return a;
    }

    const std::array<float, KernelSize + 1>& sincKernel()
    {
        static const auto kernel = [] {
            std::array<float, KernelSize + 1> table{};
            table[0] = 1;
            for (size_t i = 1; i < KernelSize; ++i) {
                double x = static_cast<double>(i) / KernelStepsPerSample;
                double angle = Pi * x / ZeroCrossings;
                double window =
                    0.42 + 0.5 * std::cos(angle) + 0.08 * std::cos(2 * angle);
                table[i] = std::sin(Pi * x) / (Pi * x) * window;
            }
            return table;
        }();
        return kernel;
    }

    float filterSample(const float* samples, const float* weights, size_t taps)
    {
        size_t i = 0;
#if defined(__ARM_NEON_FP)
        float32x4_t first = vdupq_n_f32(0);
        float32x4_t second = vdupq_n_f32(0);
        for (; i + 7 < taps; i += 8) {
            first = vmlaq_f32(first, vld1q_f32(samples + i),
                              vld1q_f32(weights + i));
            second = vmlaq_f32(second, vld1q_f32(samples + i + 4),
                               vld1q_f32(weights + i + 4));
        }
        if (i + 3 < taps) {
            first = vmlaq_f32(first, vld1q_f32(samples + i),
                              vld1q_f32(weights + i));
            i += 4;
        }
        float32x4_t sum = vaddq_f32(first, second);
        float32x2_t halves = vadd_f32(vget_low_f32(sum), vget_high_f32(sum));
        float result = vget_lane_f32(vpadd_f32(halves, halves), 0);
        for (; i < taps; ++i) {
            result += samples[i] * weights[i];
        }
        return result;
#else
        // Independent sums shorten the dependency chain of the dot product.
        float a = 0;
        float b = 0;
        float c = 0;
        float d = 0;
        for (; i + 3 < taps; i += 4) {
            a += samples[i] * weights[i];
            b += samples[i + 1] * weights[i + 1];
            c += samples[i + 2] * weights[i + 2];
            d += samples[i + 3] * weights[i + 3];
        }
        if (i < taps) {
            a += samples[i] * weights[i];
            b += samples[i + 1] * weights[i + 1];
        }
        return (a + b) + (c + d);
#endif
    }

} // namespace

AudioPCMResampler::AudioPCMResampler(unsigned inputRate, unsigned outputRate,
                                     size_t channels)
    : m_step(inputRate / greatestCommonDivisor(inputRate, outputRate))
    , m_phaseCount(outputRate / greatestCommonDivisor(inputRate, outputRate))
    // Lower the cutoff when decimating so frequencies above the new Nyquist
    // limit do not fold into the output. The margin lets the window roll off.
    , m_cutoff(0.95 * std::min(1.0, double(outputRate) / inputRate))
    , m_radius(std::ceil(ZeroCrossings / m_cutoff))
    , m_input(channels)
{
    const size_t taps = 2 * m_radius;
    // Common rational ratios reuse exact phases. Very large ratios use one
    // scratch row, keeping filter storage independent of the phase count.
    if (m_phaseCount > MaxCachedWeights / taps) {
        m_weights.resize(taps);
    } else {
        m_weights.resize(m_phaseCount * taps);
        for (unsigned phase = 0; phase < m_phaseCount; ++phase) {
            makeWeights(phase, m_weights.data() + phase * taps);
        }
    }
}

void AudioPCMResampler::makeWeights(unsigned phase, float* weights) const
{
    const auto& kernel = sincKernel();
    const double fraction = double(phase) / m_phaseCount;
    float sum = 0;
    for (size_t tap = 0; tap < 2 * m_radius; ++tap) {
        double offset = double(tap + 1) - m_radius - fraction;
        double x = std::abs(offset) * m_cutoff * KernelStepsPerSample;
        float value = 0;
        if (x < KernelSize) {
            size_t i = static_cast<size_t>(x);
            value = kernel[i] + (kernel[i + 1] - kernel[i]) * (x - i);
        }
        weights[tap] = value;
        sum += value;
    }
    // Keep a constant signal at the same gain for every fractional phase.
    float gain = 1.0f / sum;
    for (size_t tap = 0; tap < 2 * m_radius; ++tap) {
        weights[tap] *= gain;
    }
}

void AudioPCMResampler::prepareInput(size_t frames, float** planes)
{
    for (size_t channel = 0; channel < m_input.size(); ++channel) {
        auto& samples = m_input[channel];
        size_t start = samples.size();
        samples.resize(start + frames);
        planes[channel] = samples.data() + start;
    }
}

bool AudioPCMResampler::append(size_t frames, AudioBufferDataBuilder& output)
{
    m_inputFrames += frames;
    if (m_inputFrames <= m_radius) {
        return true;
    }
    // A frame is ready when all taps to the right of its center are present.
    uint64_t ready =
        ((m_inputFrames - m_radius) * m_phaseCount + m_step - 1) / m_step;
    if (!writeOutput(ready, output)) {
        return false;
    }
    discardConsumedInput();
    return true;
}

bool AudioPCMResampler::drain(AudioBufferDataBuilder& output)
{
    // The native WAVE decoder also rounds the duration to the nearest frame.
    uint64_t end = (m_inputFrames * m_phaseCount + m_step / 2) / m_step;
    return writeOutput(end, output);
}

bool AudioPCMResampler::writeOutput(uint64_t end,
                                    AudioBufferDataBuilder& output)
{
    if (end <= m_outputFrames) {
        return true;
    }
    uint64_t count = end - m_outputFrames;
    if (count > AudioBufferData::MaxSamples / m_input.size() ||
        !output.reserve(static_cast<size_t>(count))) {
        return false;
    }
    const size_t taps = 2 * m_radius;
    const bool cached = m_phaseCount == 1 || m_weights.size() > taps;
    const unsigned wholeStep = m_step / m_phaseCount;
    const unsigned fractionalStep = m_step % m_phaseCount;
    for (size_t frame = 0; frame < count; ++frame) {
        const float* weights = m_weights.data();
        if (cached) {
            weights += m_phase * taps;
        } else {
            makeWeights(m_phase, m_weights.data());
        }
        int64_t first = static_cast<int64_t>(m_sourceFrame) + 1 -
                        static_cast<int64_t>(m_radius);
        bool inside = first >= 0 && m_sourceFrame + m_radius < m_inputFrames;
        for (size_t channel = 0; channel < m_input.size(); ++channel) {
            const auto& samples = m_input[channel];
            float value;
            if (inside) {
                value = filterSample(samples.data() + (first - m_inputStart),
                                     weights, taps);
            } else {
                // Extend the endpoints for the filter's lookahead at EOF
                // and its history before the first sample.
                value = 0;
                for (size_t tap = 0; tap < taps; ++tap) {
                    int64_t index = std::max<int64_t>(0, first + int64_t(tap));
                    index = std::min<int64_t>(index, m_inputFrames - 1);
                    value += samples[index - m_inputStart] * weights[tap];
                }
            }
            output.end(channel)[frame] = value;
        }
        m_sourceFrame += wholeStep;
        m_phase += fractionalStep;
        if (m_phase >= m_phaseCount) {
            ++m_sourceFrame;
            m_phase -= m_phaseCount;
        }
    }
    output.commit(static_cast<size_t>(count));
    m_outputFrames = end;
    return true;
}

void AudioPCMResampler::discardConsumedInput()
{
    uint64_t next = std::max<uint64_t>(m_sourceFrame, m_radius) - m_radius;
    next = std::min(next, m_inputFrames);
    size_t discard = static_cast<size_t>(next - m_inputStart);
    if (!discard) {
        return;
    }
    for (auto& samples : m_input) {
        size_t keep = samples.size() - discard;
        if (keep) {
            std::memmove(samples.data(), samples.data() + discard,
                         keep * sizeof(float));
        }
        samples.resize(keep);
    }
    m_inputStart = next;
}

} // namespace Starfish

#endif
