/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/webaudio/render/ConvolverHandler.h"

#include "core/modules/webaudio/render/AudioBufferData.h"

#include <algorithm>
#include <cmath>

namespace Starfish {

ConvolverHandler::ConvolverHandler()
    : AudioHandler(1, 1, 1)
{
}

void ConvolverHandler::fft(Spectrum& values, bool inverse)
{
    for (size_t i = 1, j = 0; i < FFTSize; ++i) {
        size_t bit = FFTSize >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(values[i], values[j]);
        }
    }
    for (size_t size = 2; size <= FFTSize; size <<= 1) {
        const float angle =
            (inverse ? 2.0f : -2.0f) * 3.14159265358979323846f / size;
        const std::complex<float> step(std::cos(angle), std::sin(angle));
        for (size_t offset = 0; offset < FFTSize; offset += size) {
            std::complex<float> rotation(1, 0);
            for (size_t k = 0; k < size / 2; ++k) {
                const auto even = values[offset + k];
                const auto odd = values[offset + k + size / 2] * rotation;
                values[offset + k] = even + odd;
                values[offset + k + size / 2] = even - odd;
                rotation *= step;
            }
        }
    }
    if (inverse) {
        for (auto& value : values) {
            value /= static_cast<float>(FFTSize);
        }
    }
}

void ConvolverHandler::resetHistory()
{
    m_cursor = 0;
    m_shortCursor = 0;
    m_hadStereoInput = input(0).bus().channels() == 2;
    for (auto& channel : m_shortHistory) {
        channel.fill(0);
    }
    std::vector<HalfSpectrum>().swap(m_inputSpectra[0]);
    std::vector<HalfSpectrum>().swap(m_inputSpectra[1]);
    for (auto& channel : m_overlap) {
        channel.fill(0);
    }
    m_inputSpectra[0].assign(m_partitions, HalfSpectrum{});
    std::vector<bool>().swap(m_silentPartitions);
    m_silentPartitions.assign(m_partitions, true);
    m_audiblePartitions = 0;
    if (m_responseChannels == 4 || input(0).bus().channels() == 2) {
        m_inputSpectra[1].assign(m_partitions, HalfSpectrum{});
    }
}

std::unique_ptr<ConvolverHandler::Response> ConvolverHandler::createResponse(
    const AudioBufferData* data, bool normalize, double sampleRate)
{
    std::unique_ptr<Response> response(new Response);
    response->channels = data->channels();
    response->length = data->frames();
    response->partitions = response->length > 32
                               ? (response->length + BlockSize - 1) / BlockSize
                               : 0;
    const size_t channels = response->channels;
    const size_t length = response->length;
    double scale = 1;
    if (normalize) {
        double squares = 0;
        for (size_t ch = 0; ch < channels; ++ch) {
            for (size_t frame = 0; frame < length; ++frame) {
                const double value = data->channel(ch)[frame];
                squares += value * value;
            }
        }
        double power = std::sqrt(squares / (channels * length));
        if (!std::isfinite(power) || power < 0.000125) {
            power = 0.000125;
        }
        scale = 0.00125 * 44100 / sampleRate / power;
        if (channels == 4) {
            scale *= 0.5;
        }
    }
    for (size_t ch = 0; ch < channels; ++ch) {
        if (!response->partitions) {
            auto& samples = response->shortResponse[ch];
            samples.resize(length);
            for (size_t i = 0; i < length; ++i) {
                samples[i] = static_cast<float>(data->channel(ch)[i] * scale);
            }
            continue;
        }
        auto& spectra = response->spectra[ch];
        spectra.resize(response->partitions);
        Spectrum spectrum;
        for (size_t part = 0; part < response->partitions; ++part) {
            spectrum.fill({ 0, 0 });
            for (size_t i = 0; i < BlockSize; ++i) {
                const size_t frame = part * BlockSize + i;
                if (frame < length) {
                    spectrum[i] =
                        static_cast<float>(data->channel(ch)[frame] * scale);
                }
            }
            fft(spectrum, false);
            std::copy_n(spectrum.begin(), SpectrumBins, spectra[part].begin());
        }
    }
    return response;
}

std::unique_ptr<ConvolverHandler::Response> ConvolverHandler::swapResponse(
    std::unique_ptr<Response> response)
{
    std::swap(m_response, response);
    m_responseChannels = m_response ? m_response->channels : 0;
    m_responseLength = m_response ? m_response->length : 0;
    m_partitions = m_response ? m_response->partitions : 0;
    resetHistory();
    updateOutputChannels();
    return response;
}

void ConvolverHandler::updateOutputChannels()
{
    const size_t channels = m_responseChannels == 1 && !m_hadStereoInput &&
                                    input(0).bus().channels() == 1
                                ? 1
                            : m_responseChannels ? 2
                                                 : 1;
    output(0).configureChannels(channels);
}

void ConvolverHandler::inputChannelsChanged(size_t)
{
    if (input(0).bus().channels() == 2) {
        if (m_partitions && m_inputSpectra[1].empty()) {
            m_inputSpectra[1].assign(m_partitions, HalfSpectrum{});
        }
        if (!m_hadStereoInput && m_speakers) {
            // Up-mix the still-ringing mono response when a second input
            // channel appears. The new right channel inherits its history.
            m_shortHistory[1] = m_shortHistory[0];
            m_inputSpectra[1] = m_inputSpectra[0];
            m_overlap[1] = m_overlap[0];
        }
        // A stereo input's response tail must remain stereo after the
        // upstream node changes back to mono.
        m_hadStereoInput = true;
    }
    updateOutputChannels();
}

void ConvolverHandler::processShort(size_t frames)
{
    const AudioBus& source = input(0).bus();
    AudioBus& destination = output(0).bus();
    const size_t sourceChannels = source.channels();
    const size_t matrixInputs =
        m_responseChannels == 4 ||
                (m_responseChannels == 1 && destination.channels() == 2)
            ? 2
            : sourceChannels;
    const size_t outputChannels = destination.channels();
    bool wrote = false;
    for (size_t i = 0; i < frames; ++i) {
        for (size_t ch = 0; ch < matrixInputs; ++ch) {
            m_shortHistory[ch][m_shortCursor] =
                ch < sourceChannels ? source.channel(ch)[i]
                : m_speakers        ? source.channel(0)[i]
                                    : 0;
        }
        for (size_t out = 0; out < outputChannels; ++out) {
            float value = 0;
            for (size_t in = 0; in < matrixInputs; ++in) {
                if (m_responseChannels == 1 && outputChannels == 2 &&
                    in != out) {
                    continue;
                }
                if (m_responseChannels == 2 && sourceChannels == 2 &&
                    in != out) {
                    continue;
                }
                const size_t responseChannel = m_responseChannels == 4
                                                   ? in * 2 + out
                                               : m_responseChannels == 2 ? out
                                                                         : 0;
                const auto& response =
                    m_response->shortResponse[responseChannel];
                for (size_t tap = 0; tap < m_responseLength; ++tap) {
                    value +=
                        m_shortHistory[in][(m_shortCursor + 32 - tap) % 32] *
                        response[tap];
                }
            }
            destination.channel(out)[i] = value;
            wrote = wrote || value != 0;
        }
        m_shortCursor = (m_shortCursor + 1) % 32;
    }
    destination.setSilent(!wrote);
}

void ConvolverHandler::processPartitioned(size_t frames)
{
    const AudioBus& source = input(0).bus();
    AudioBus& destination = output(0).bus();
    const size_t sourceChannels = source.channels();
    const size_t matrixInputs =
        m_responseChannels == 4 ||
                (m_responseChannels == 1 && destination.channels() == 2)
            ? 2
            : sourceChannels;
    const bool silent = source.isSilent();
    if (m_silentPartitions[m_cursor] != silent) {
        m_silentPartitions[m_cursor] = silent;
        if (silent) {
            --m_audiblePartitions;
        } else {
            ++m_audiblePartitions;
        }
    }
    if (!silent) {
        Spectrum spectrum;
        for (size_t ch = 0; ch < sourceChannels; ++ch) {
            spectrum.fill({ 0, 0 });
            for (size_t i = 0; i < frames; ++i) {
                spectrum[i] = source.channel(ch)[i];
            }
            fft(spectrum, false);
            std::copy_n(spectrum.begin(), SpectrumBins,
                        m_inputSpectra[ch][m_cursor].begin());
        }
        if (matrixInputs == 2 && sourceChannels == 1) {
            m_inputSpectra[1][m_cursor] =
                m_speakers ? m_inputSpectra[0][m_cursor] : HalfSpectrum{};
        }
    }
    m_cursor = (m_cursor + 1) % m_partitions;
    if (!m_audiblePartitions) {
        // Only the overlap of the last audible block remains; once that has
        // been emitted, the response tail has ended.
        // https://webaudio.github.io/web-audio-api/#tail-time
        bool wrote = false;
        for (size_t out = 0; out < destination.channels(); ++out) {
            for (size_t i = 0; i < frames; ++i) {
                destination.channel(out)[i] = m_overlap[out][i];
                wrote = wrote || m_overlap[out][i] != 0;
            }
            m_overlap[out].fill(0);
        }
        destination.setSilent(!wrote);
        return;
    }
    // The cursor has advanced, so the current block sits one slot behind it.
    const size_t current = (m_cursor + m_partitions - 1) % m_partitions;
    for (size_t out = 0; out < destination.channels(); ++out) {
        HalfSpectrum sum{};
        for (size_t in = 0; in < matrixInputs; ++in) {
            if (m_responseChannels == 1 && destination.channels() == 2 &&
                in != out) {
                continue;
            }
            if (m_responseChannels == 2 && sourceChannels == 2 && in != out) {
                continue;
            }
            const size_t responseChannel = m_responseChannels == 4
                                               ? in * 2 + out
                                           : m_responseChannels == 2 ? out
                                                                     : 0;
            for (size_t part = 0; part < m_partitions; ++part) {
                const size_t slot =
                    (current + m_partitions - part) % m_partitions;
                if (m_silentPartitions[slot]) {
                    continue;
                }
                const HalfSpectrum& impulse =
                    m_response->spectra[responseChannel][part];
                const HalfSpectrum& history = m_inputSpectra[in][slot];
                for (size_t bin = 0; bin < SpectrumBins; ++bin) {
                    sum[bin] += history[bin] * impulse[bin];
                }
            }
        }
        // Rebuild the upper half from conjugate symmetry for the inverse.
        Spectrum full;
        std::copy_n(sum.begin(), SpectrumBins, full.begin());
        for (size_t bin = 1; bin < FFTSize / 2; ++bin) {
            full[FFTSize - bin] = std::conj(sum[bin]);
        }
        fft(full, true);
        for (size_t i = 0; i < frames; ++i) {
            destination.channel(out)[i] = full[i].real() + m_overlap[out][i];
        }
        for (size_t i = 0; i < BlockSize; ++i) {
            m_overlap[out][i] = full[BlockSize + i].real();
        }
    }
    destination.setSilent(false);
}

void ConvolverHandler::process(uint64_t, size_t frames)
{
    if (!m_responseChannels) {
        return;
    }
    if (m_partitions) {
        processPartitioned(frames);
    } else {
        processShort(frames);
    }
}

} // namespace Starfish
#endif
