/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "core/modules/webaudio/render/PeriodicWaveData.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace Starfish {

PeriodicWaveData::PeriodicWaveData(const std::vector<Partial>& partials,
                                   bool disableNormalization)
    : m_partials(partials)
    , m_normalize(!disableNormalization)
{
}

static void inverseTransform(std::vector<std::complex<double>>& spectrum)
{
    const size_t size = spectrum.size();
    for (size_t i = 1, j = 0; i < size; i++) {
        size_t bit = size >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(spectrum[i], spectrum[j]);
        }
    }
    constexpr double twoPi = 6.28318530717958647692;
    for (size_t width = 2; width <= size; width <<= 1) {
        const std::complex<double> step =
            std::polar(1.0, twoPi / static_cast<double>(width));
        for (size_t start = 0; start < size; start += width) {
            std::complex<double> rotation(1, 0);
            for (size_t i = 0; i < width / 2; i++) {
                const std::complex<double> even = spectrum[start + i];
                const std::complex<double> odd =
                    rotation * spectrum[start + i + width / 2];
                spectrum[start + i] = even + odd;
                spectrum[start + i + width / 2] = even - odd;
                rotation *= step;
            }
        }
    }
}

void PeriodicWaveData::prepare()
{
    if (m_prepared) {
        return;
    }
    m_prepared = true;
    if (m_partials.empty()) {
        return;
    }
    if (m_partials.size() == 1) {
        const Partial& partial = m_partials[0];
        const double peak = std::hypot(partial.real, partial.imag);
        m_scale = m_normalize && peak > 0 ? 1 / peak : 1;
        return;
    }

    // The specification defines a fixed normalization factor sampled on a
    // power-of-two grid, independent of oscillator frequency.
    if (m_partials.size() > 32) {
        // A single full-spectrum table keeps dense waves cheap at low
        // frequencies. When higher frequencies omit harmonics, sample() uses
        // the sparse coefficient list to avoid aliasing instead.
        constexpr size_t tableSize = 32768;
        std::vector<std::complex<double>> spectrum(tableSize);
        for (const Partial& partial : m_partials) {
            spectrum[partial.harmonic] =
                std::complex<double>(partial.real, -partial.imag) * 0.5;
            spectrum[tableSize - partial.harmonic] =
                std::complex<double>(partial.real, partial.imag) * 0.5;
        }
        inverseTransform(spectrum);
        m_table.resize(tableSize);
        double peak = 0;
        for (size_t i = 0; i < tableSize; i++) {
            const double value = spectrum[i].real();
            m_table[i] = static_cast<float>(value);
            peak = std::max(peak, std::abs(value));
        }
        if (m_normalize && peak > 0) {
            m_scale = 1 / peak;
        }
        return;
    }
    if (!m_normalize) {
        return;
    }
    constexpr size_t gridSize = 16384;
    constexpr double twoPi = 6.28318530717958647692;
    std::vector<double> values(gridSize, 0);
    for (const Partial& partial : m_partials) {
        const double angle = twoPi * partial.harmonic / gridSize;
        const double stepReal = std::cos(angle);
        const double stepImag = std::sin(angle);
        double real = 1;
        double imag = 0;
        for (size_t i = 0; i < gridSize; i++) {
            values[i] += partial.real * real + partial.imag * imag;
            const double nextReal = real * stepReal - imag * stepImag;
            imag = real * stepImag + imag * stepReal;
            real = nextReal;
        }
    }
    double peak = 0;
    for (double value : values) {
        peak = std::max(peak, std::abs(value));
    }
    if (peak > 0) {
        m_scale = 1 / peak;
    }
}

void PeriodicWaveData::retain()
{
    m_references.fetch_add(1, std::memory_order_relaxed);
}

void PeriodicWaveData::release()
{
    if (m_references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

double PeriodicWaveData::sample(double phase, size_t maxHarmonic) const
{
    if (!m_table.empty() && maxHarmonic >= m_partials.back().harmonic) {
        const size_t size = m_table.size();
        const double position = phase * size;
        const size_t index = static_cast<size_t>(position) & (size - 1);
        const double fraction = position - std::floor(position);
        const double previous = m_table[(index + size - 1) & (size - 1)];
        const double current = m_table[index];
        const double next = m_table[(index + 1) & (size - 1)];
        const double after = m_table[(index + 2) & (size - 1)];
        const double squared = fraction * fraction;
        const double cubed = squared * fraction;
        const double value =
            0.5 * (2 * current + (-previous + next) * fraction +
                   (2 * previous - 5 * current + 4 * next - after) * squared +
                   (-previous + 3 * current - 3 * next + after) * cubed);
        return value * m_scale;
    }
    constexpr double twoPi = 6.28318530717958647692;
    double value = 0;
    for (const Partial& partial : m_partials) {
        if (partial.harmonic > maxHarmonic) {
            break;
        }
        const double angle = twoPi * partial.harmonic * phase;
        value +=
            partial.real * std::cos(angle) + partial.imag * std::sin(angle);
    }
    return value * m_scale;
}

} // namespace Starfish
#endif
