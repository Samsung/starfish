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
#include "Starfish.h"
#include "core/modules/webaudio/render/AudioHandlers.h"
#include "core/modules/webaudio/render/PeriodicWaveData.h"
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <iterator>
#include <limits>

namespace Starfish {

struct MediaAudioPlaybackState::StreamingData {
    struct Chunk {
        std::vector<int16_t> samples;
        size_t channels;
        uint32_t sampleRate;
        double position;
    };
    std::deque<Chunk> chunks;
    size_t bytes{ 0 };
    size_t channels{ 1 };
};

MediaAudioPlaybackState::MediaAudioPlaybackState() = default;

void MediaAudioPlaybackState::setStreamingPosition(double position,
                                                   bool playing)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_streaming) {
        m_streaming.reset(new StreamingData);
    }
    m_positionAtStart = position;
    m_startedAt = std::chrono::steady_clock::now();
    // Periodic position reports only correct drift; a pause or resume is a
    // discontinuity the graph cursor must follow immediately.
    if (m_playing != playing) {
        m_generation++;
    }
    m_playing = playing;
}

void MediaAudioPlaybackState::appendStreamingPCM(const int16_t* samples,
                                                 size_t frames, size_t channels,
                                                 uint32_t sampleRate,
                                                 double position)
{
    constexpr size_t MaxBytes = 1024 * 1024;
    constexpr size_t MaxChunks = 64;
    if (channels == 0 || channels > 32 || sampleRate == 0 || frames == 0 ||
        frames > MaxBytes / (channels * sizeof(int16_t)) ||
        !std::isfinite(position)) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_streaming || !m_routed || !m_originClean) {
        return;
    }
    const size_t bytes = frames * channels * sizeof(int16_t);
    auto& stream = *m_streaming;
    while (!stream.chunks.empty() && (stream.bytes + bytes > MaxBytes ||
                                      stream.chunks.size() >= MaxChunks)) {
        stream.bytes -= stream.chunks.front().samples.size() * sizeof(int16_t);
        stream.chunks.pop_front();
    }
    StreamingData::Chunk chunk;
    chunk.samples.assign(samples, samples + frames * channels);
    chunk.channels = channels;
    chunk.sampleRate = sampleRate;
    chunk.position = position;
    stream.chunks.push_back(std::move(chunk));
    stream.bytes += bytes;
    stream.channels = channels;
}

void MediaAudioPlaybackState::clearStreamingPCM()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_streaming) {
        m_streaming->chunks.clear();
        m_streaming->bytes = 0;
    }
}

void MediaAudioPlaybackState::renderStreamingLocked(AudioBus& bus,
                                                    size_t frames,
                                                    double outputRate,
                                                    double position) const
{
    bool wrote = false;
    for (const auto& chunk : m_streaming->chunks) {
        const size_t available = chunk.samples.size() / chunk.channels;
        const double last = position + (frames - 1) * m_rate / outputRate;
        if (chunk.position > std::max(position, last) ||
            chunk.position +
                    available / static_cast<double>(chunk.sampleRate) <=
                std::min(position, last)) {
            continue;
        }
        for (size_t i = 0; i < frames; i++) {
            const double frame =
                (position + i * m_rate / outputRate - chunk.position) *
                chunk.sampleRate;
            if (frame < 0 || frame >= available) {
                continue;
            }
            const size_t first = static_cast<size_t>(frame);
            const size_t second = std::min(first + 1, available - 1);
            const double fraction = frame - first;
            for (size_t channel = 0; channel < bus.channels(); channel++) {
                if (channel >= chunk.channels) {
                    continue;
                }
                const float a = chunk.samples[first * chunk.channels + channel];
                const float b =
                    chunk.samples[second * chunk.channels + channel];
                bus.channel(channel)[i] = static_cast<float>(
                    (a + (b - a) * fraction) * m_volume / 32768.0);
            }
            wrote = true;
        }
    }
    bus.setSilent(!wrote);
}

MediaAudioPlaybackState::~MediaAudioPlaybackState()
{
    if (m_pcm) {
        m_pcm->release();
    }
}

void MediaAudioPlaybackState::retain()
{
    m_references.fetch_add(1, std::memory_order_relaxed);
}

void MediaAudioPlaybackState::release()
{
    if (m_references.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

void MediaAudioPlaybackState::setPCM(AudioBufferData* pcm)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pcm) {
        m_pcm->release();
    }
    m_pcm = pcm;
    m_positionAtStart = 0;
    m_startedAt = std::chrono::steady_clock::now();
    m_generation++;
}

double MediaAudioPlaybackState::currentTimeLocked() const
{
    double position = m_positionAtStart;
    if (m_playing) {
        position += std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - m_startedAt)
                        .count() *
                    m_rate;
    }
    if (m_streaming) {
        return std::max(0.0, position);
    }
    const double length = m_pcm ? m_pcm->frames() / 48000.0 : 0;
    if (m_loop && length > 0) {
        position = std::fmod(position, length);
        if (position < 0) {
            position += length;
        }
    }
    return std::max(0.0, std::min(position, length));
}

void MediaAudioPlaybackState::setPlaying(bool playing)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_playing == playing) {
        return;
    }
    m_positionAtStart = currentTimeLocked();
    if (playing && m_pcm && m_positionAtStart >= m_pcm->frames() / 48000.0) {
        m_positionAtStart = 0;
    }
    m_startedAt = std::chrono::steady_clock::now();
    m_playing = playing;
    m_generation++;
}

void MediaAudioPlaybackState::seek(double time)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const double length = m_pcm ? m_pcm->frames() / 48000.0 : 0;
    m_positionAtStart = std::max(0.0, std::min(time, length));
    m_startedAt = std::chrono::steady_clock::now();
    m_generation++;
}

void MediaAudioPlaybackState::setVolume(double volume)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_volume = volume;
}

void MediaAudioPlaybackState::setMuted(bool muted)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_muted = muted;
}

void MediaAudioPlaybackState::setLoop(bool loop)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_positionAtStart = currentTimeLocked();
    m_startedAt = std::chrono::steady_clock::now();
    m_loop = loop;
    m_generation++;
}

void MediaAudioPlaybackState::setRate(double rate)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_positionAtStart = currentTimeLocked();
    m_startedAt = std::chrono::steady_clock::now();
    m_rate = std::isfinite(rate) ? rate : 0;
    m_generation++;
}

void MediaAudioPlaybackState::setOriginClean(bool clean)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_originClean = clean;
}

void MediaAudioPlaybackState::setRouted(bool routed)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routed = routed;
}

bool MediaAudioPlaybackState::routed() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_routed;
}

MediaAudioPlaybackState::Clock MediaAudioPlaybackState::clock() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    Clock clock;
    clock.position = currentTimeLocked();
    clock.rate = m_rate;
    clock.length = !m_streaming && m_pcm ? m_pcm->frames() / 48000.0 : 0;
    clock.generation = m_generation;
    clock.playing = m_playing;
    clock.loop = m_loop;
    return clock;
}

double MediaAudioPlaybackState::currentTime() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return currentTimeLocked();
}

double MediaAudioPlaybackState::duration() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pcm ? m_pcm->frames() / 48000.0
                 : std::numeric_limits<double>::quiet_NaN();
}

double MediaAudioPlaybackState::playbackRate() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rate;
}

bool MediaAudioPlaybackState::ended() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_playing || m_loop || !m_pcm || !m_pcm->frames()) {
        return false;
    }
    const double position =
        m_positionAtStart + std::chrono::duration<double>(
                                std::chrono::steady_clock::now() - m_startedAt)
                                    .count() *
                                m_rate;
    return m_rate >= 0 ? position >= m_pcm->frames() / 48000.0 : position <= 0;
}

size_t MediaAudioPlaybackState::channels() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_streaming) {
        return m_streaming->channels;
    }
    return m_pcm ? m_pcm->channels() : 1;
}

void MediaAudioPlaybackState::render(AudioBus& bus, size_t frames,
                                     double outputRate, double position,
                                     bool forGraph) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_playing || (!m_pcm && !m_streaming) || m_muted || m_volume == 0 ||
        (forGraph && !m_originClean)) {
        return;
    }
    if (m_streaming) {
        renderStreamingLocked(bus, frames, outputRate, position);
        return;
    }
    const double length = m_pcm->frames() / 48000.0;
    bool wrote = false;
    for (size_t i = 0; i < frames; i++) {
        double time = position + i * m_rate / outputRate;
        if (m_loop && length > 0) {
            time = std::fmod(time, length);
            if (time < 0) {
                time += length;
            }
        }
        if (time < 0 || time >= length) {
            continue;
        }
        const double frame = time * 48000.0;
        const size_t first = static_cast<size_t>(frame);
        const size_t second = std::min(first + 1, m_pcm->frames() - 1);
        const double fraction = frame - first;
        for (size_t channel = 0; channel < bus.channels(); channel++) {
            const float* samples =
                m_pcm->channel(std::min(channel, m_pcm->channels() - 1));
            bus.channel(channel)[i] = static_cast<float>(
                (samples[first] +
                 (samples[second] - samples[first]) * fraction) *
                m_volume);
        }
        wrote = true;
    }
    bus.setSilent(!wrote);
}

MediaElementSourceHandler::MediaElementSourceHandler(double sampleRate)
    : AudioHandler(0, 1, 1)
    , m_sampleRate(sampleRate)
{
}

MediaElementSourceHandler::~MediaElementSourceHandler()
{
    // Release only what this handler owns: during graph teardown the
    // connected handlers may already be destroyed, so the output must not
    // be reconfigured here.
    if (m_state) {
        m_state->setRouted(false);
        m_state->release();
    }
}

void MediaElementSourceHandler::setPlaybackState(MediaAudioPlaybackState* state)
{
    if (state) {
        state->retain();
    }
    if (m_state) {
        m_state->setRouted(false);
        m_state->release();
    }
    m_state = state;
    m_hasCursor = false;
    if (m_state) {
        m_state->setRouted(true);
    }
    output(0).configureChannels(m_state ? m_state->channels() : 1);
}

void MediaElementSourceHandler::process(uint64_t, size_t frames)
{
    if (!m_state) {
        return;
    }
    output(0).configureChannels(m_state->channels());
    // The media clock is wall-clock based and render quanta are not evenly
    // spaced in time, so sampling it per quantum would repeat or skip audio.
    // Follow it only across discontinuities or when drift becomes audible.
    constexpr double MaxDrift = 0.05;
    const MediaAudioPlaybackState::Clock clock = m_state->clock();
    double cursor = m_cursor;
    if (clock.loop && clock.length > 0) {
        cursor = std::fmod(cursor, clock.length);
        if (cursor < 0) {
            cursor += clock.length;
        }
    }
    if (!m_hasCursor || clock.generation != m_generation || !clock.playing ||
        std::abs(cursor - clock.position) > MaxDrift) {
        cursor = clock.position;
        m_generation = clock.generation;
        m_hasCursor = true;
    }
    m_state->render(output(0).bus(), frames, m_sampleRate, cursor, true);
    m_cursor = cursor;
    if (clock.playing) {
        m_cursor += frames * clock.rate / m_sampleRate;
    }
}

AudioDestinationHandler::AudioDestinationHandler(size_t channels)
    : AudioHandler(1, 1, channels)
{
    input(0).configure(channels, AudioInputChannelMode::Explicit, true);
}

void AudioDestinationHandler::configureChannels(size_t channels)
{
    input(0).configure(channels, AudioInputChannelMode::Explicit, true);
    output(0).configureChannels(channels);
}

void AudioDestinationHandler::process(uint64_t, size_t frames)
{
    output(0).bus().copyFrom(input(0).bus(), frames, true);
}

ChannelMergerHandler::ChannelMergerHandler(size_t inputs)
    : AudioHandler(inputs, 1, inputs)
    , m_inputCount(inputs)
{
    for (size_t index = 0; index < inputs; index++) {
        input(index).configure(1, AudioInputChannelMode::Explicit, true);
    }
}

void ChannelMergerHandler::process(uint64_t, size_t frames)
{
    bool active = false;
    for (size_t index = 0; index < m_inputCount; index++) {
        if (!input(index).bus().isSilent()) {
            active = true;
            break;
        }
    }
    output(0).configureChannels(active ? m_inputCount : 1);
    if (!active) {
        return;
    }
    AudioBus& result = output(0).bus();
    for (size_t index = 0; index < m_inputCount; index++) {
        const AudioBus& source = input(index).bus();
        if (!source.isSilent()) {
            std::copy_n(source.channel(0), frames, result.channel(index));
        }
    }
    result.setSilent(false);
}

ChannelSplitterHandler::ChannelSplitterHandler(size_t outputs)
    : AudioHandler(1, outputs, 1)
    , m_outputCount(outputs)
{
    input(0).configure(outputs, AudioInputChannelMode::Explicit, false);
}

void ChannelSplitterHandler::process(uint64_t, size_t frames)
{
    const AudioBus& source = input(0).bus();
    if (source.isSilent()) {
        return;
    }
    for (size_t index = 0; index < m_outputCount; index++) {
        AudioBus& result = output(index).bus();
        std::copy_n(source.channel(index), frames, result.channel(0));
        result.setSilent(false);
    }
}

GainHandler::GainHandler(double sampleRate)
    : AudioHandler(1, 1, 2)
    , m_gain(1)
    , m_sampleRate(sampleRate)
{
}

WaveShaperHandler::WaveShaperHandler()
    : AudioHandler(1, 1, 2)
{
}

float WaveShaperHandler::shape(float value) const
{
    if (m_curve.empty()) {
        return value;
    }
    // https://webaudio.github.io/web-audio-api/#dom-waveshapernode-curve
    // The curve mapping has no case for NaN (v would be NaN and k = floor(v)
    // undefined). Treat NaN like silence, i.e. x = 0, the curve's center,
    // instead of indexing the curve with an undefined position.
    if (std::isnan(value)) {
        value = 0;
    }
    const double position =
        (static_cast<double>(value) + 1) * (m_curve.size() - 1) / 2;
    if (position <= 0) {
        return m_curve.front();
    }
    if (position >= m_curve.size() - 1) {
        return m_curve.back();
    }
    const size_t index = static_cast<size_t>(position);
    const double fraction = position - index;
    return static_cast<float>((1 - fraction) * m_curve[index] +
                              fraction * m_curve[index + 1]);
}

// https://webaudio.github.io/web-audio-api/#WaveShaperNode
// The spec leaves the resampling filters open; linear interpolation and a
// box downsampler keep the 2x/4x mode allocation-free on the render path.
void WaveShaperHandler::process(uint64_t, size_t frames)
{
    const AudioBus& source = input(0).bus();
    output(0).configureChannels(source.channels());
    AudioBus& target = output(0).bus();
    if (m_curve.empty()) {
        target.copyFrom(source, frames, true);
        return;
    }
    if (m_previousInput.size() < source.channels()) {
        m_previousInput.resize(source.channels());
    }
    bool active = false;
    for (size_t channel = 0; channel < source.channels(); channel++) {
        float previous = m_previousInput[channel];
        for (size_t frame = 0; frame < frames; frame++) {
            const float sample = source.channel(channel)[frame];
            float result = 0;
            for (size_t subframe = 0; subframe < m_oversample; subframe++) {
                const float fraction =
                    static_cast<float>(subframe + 1) / m_oversample;
                result += shape(previous + (sample - previous) * fraction);
            }
            result /= m_oversample;
            target.channel(channel)[frame] = result;
            active |= result != 0;
            previous = sample;
        }
        m_previousInput[channel] = previous;
    }
    target.setSilent(!active);
}

AnalyserHandler::AnalyserHandler()
    : AudioHandler(1, 1, 2)
{
}

void AnalyserHandler::setFftSize(size_t size)
{
    if (m_fftSize == size) {
        return;
    }
    if (m_history) {
        // Keep the newest samples that fit; older ones were not retained.
        std::unique_ptr<float[]> history(new float[size]());
        const size_t kept = std::min(size, m_fftSize);
        for (size_t i = 0; i < kept; i++) {
            history[(m_writtenFrames - kept + i) % size] =
                timeSample(m_fftSize - kept + i);
        }
        m_history = std::move(history);
    }
    m_fftSize = size;
    m_lastSpectrumFrame = static_cast<uint64_t>(-1);
    std::vector<float>().swap(m_smoothed);
    std::vector<float>().swap(m_frequencies);
}

void AnalyserHandler::process(uint64_t, size_t frames)
{
    const AudioBus& source = input(0).bus();
    output(0).configureChannels(source.channels());
    output(0).bus().copyFrom(source, frames, true);
    if (!m_history && source.isSilent()) {
        m_writtenFrames += frames;
        return;
    }
    m_mono.copyFrom(source, frames, true);
    if (!m_history) {
        m_history.reset(new float[m_fftSize]());
    }
    for (size_t frame = 0; frame < frames; frame++) {
        m_history[m_writtenFrames % m_fftSize] = m_mono.channel(0)[frame];
        m_writtenFrames++;
    }
}

float AnalyserHandler::timeSample(size_t index) const
{
    if (!m_history) {
        return 0;
    }
    // m_fftSize is a power of two, so the ring index stays continuous when
    // m_writtenFrames wraps.
    return m_history[(m_writtenFrames + index) % m_fftSize];
}

void AnalyserHandler::copyTimeDomain(float* destination, size_t length) const
{
    const size_t count = std::min(length, m_fftSize);
    for (size_t i = 0; i < count; i++) {
        destination[i] = timeSample(i);
    }
}

// https://webaudio.github.io/web-audio-api/#fft-windowing-and-smoothing-over-time
const std::vector<float>& AnalyserHandler::frequencyData()
{
    if (m_lastSpectrumFrame == m_writtenFrames) {
        return m_frequencies;
    }
    const size_t size = m_fftSize;
    if (m_smoothed.size() != size / 2) {
        m_smoothed.assign(size / 2, 0);
    }
    m_frequencies.resize(size / 2);
    // Scratch lives only for this call: the spectrum is recomputed at most
    // once per render quantum, and keeping it would cost 8 bytes per FFT
    // point per node. Twiddles come from double-precision trigonometry, so
    // single-precision butterflies stay well within the dB tolerances.
    std::unique_ptr<std::complex<float>[]> fft(new std::complex<float>[size]);
    std::unique_ptr<std::complex<float>[]> twiddles(
        new std::complex<float>[size / 2]);
    constexpr double twoPi = 6.28318530717958647692;
    for (size_t i = 0; i < size / 2; i++) {
        twiddles[i] = std::complex<float>(
            static_cast<float>(std::cos(twoPi * i / size)),
            static_cast<float>(-std::sin(twoPi * i / size)));
    }
    for (size_t i = 0; i < size; i++) {
        const double window = 0.42 - 0.5 * std::cos(twoPi * i / size) +
                              0.08 * std::cos(2 * twoPi * i / size);
        fft[i] = static_cast<float>(timeSample(i) * window);
    }
    for (size_t i = 1, j = 0; i < size; i++) {
        size_t bit = size >> 1;
        for (; j & bit; bit >>= 1) {
            j ^= bit;
        }
        j ^= bit;
        if (i < j) {
            std::swap(fft[i], fft[j]);
        }
    }
    for (size_t width = 2; width <= size; width <<= 1) {
        const size_t stride = size / width;
        for (size_t start = 0; start < size; start += width) {
            for (size_t i = 0; i < width / 2; i++) {
                const std::complex<float> odd =
                    twiddles[i * stride] * fft[start + i + width / 2];
                const std::complex<float> even = fft[start + i];
                fft[start + i] = even + odd;
                fft[start + i + width / 2] = even - odd;
            }
        }
    }
    for (size_t i = 0; i < size / 2; i++) {
        const double magnitude = std::abs(fft[i]) / static_cast<double>(size);
        double smoothed =
            m_smoothing * m_smoothed[i] + (1 - m_smoothing) * magnitude;
        if (!std::isfinite(smoothed)) {
            smoothed = 0;
        }
        m_smoothed[i] = static_cast<float>(smoothed);
        m_frequencies[i] = smoothed > 0
                               ? static_cast<float>(20 * std::log10(smoothed))
                               : -std::numeric_limits<float>::infinity();
    }
    m_lastSpectrumFrame = m_writtenFrames;
    return m_frequencies;
}

DynamicsCompressorHandler::DynamicsCompressorHandler(double sampleRate)
    : AudioHandler(1, 1, 2)
    , m_threshold(-24, -100, 0)
    , m_knee(30, 0, 40)
    , m_ratio(12, 1, 20)
    , m_attack(0.003f, 0, 1)
    , m_release(0.25f, 0, 1)
    , m_sampleRate(sampleRate)
    , m_delayFrames(sampleRate * 0.006)
{
    m_threshold.setFixedKRate();
    m_knee.setFixedKRate();
    m_ratio.setFixedKRate();
    m_attack.setFixedKRate();
    m_release.setFixedKRate();
}

// https://webaudio.github.io/web-audio-api/#dynamicscompressornode-processing
// The specification permits the soft-knee and envelope curves to be chosen by
// the UA. A quadratic knee and ratio-scaled one-pole envelope avoid tables.
void DynamicsCompressorHandler::process(uint64_t frameStart, size_t frames)
{
    const AudioBus& source = input(0).bus();
    output(0).configureChannels(source.channels());
    AudioBus& target = output(0).bus();
    m_silentFrames = source.isSilent() ? m_silentFrames + frames : 0;
    if (!m_delay.empty() && m_silentFrames > m_delay.size() + frames &&
        m_compressorGain > 1 - 1e-6) {
        // The look-ahead line holds only silence and the envelope has
        // recovered, so the tail has ended; drop the line until new input.
        std::vector<std::array<float, 2>>().swap(m_delay);
        m_compressorGain = 1;
    }
    if (!m_delay.empty() || !source.isSilent()) {
        if (m_delay.empty()) {
            m_delay.resize(static_cast<size_t>(std::ceil(m_delayFrames)) + 2);
        }
    } else {
        m_position += frames;
        m_reduction.store(0, std::memory_order_relaxed);
        return;
    }

    const double time = static_cast<double>(frameStart) / m_sampleRate;
    const double threshold = m_threshold.computedValueAt(time, 0);
    const double knee = m_knee.computedValueAt(time, 0);
    const double ratio = m_ratio.computedValueAt(time, 0);
    const double attack = m_attack.computedValueAt(time, 0);
    const double release = m_release.computedValueAt(time, 0);
    auto shapedDb = [threshold, knee, ratio](double inputDb) {
        if (inputDb <= threshold) {
            return inputDb;
        }
        if (knee > 0 && inputDb < threshold + knee) {
            const double position = inputDb - threshold;
            return inputDb + (1 / ratio - 1) * position * position / (2 * knee);
        }
        return threshold + knee * (1 + 1 / ratio) / 2 +
               (inputDb - threshold - knee) / ratio;
    };
    const double fullRangeGain = std::pow(10, shapedDb(0) / 20);
    const double makeupGain = std::pow(1 / fullRangeGain, 0.6);
    bool active = false;
    for (size_t frame = 0; frame < frames; frame++) {
        double peak = 0;
        for (size_t channel = 0; channel < source.channels(); channel++) {
            peak = std::max(peak, std::abs(static_cast<double>(
                                      source.channel(channel)[frame])));
        }
        double attenuation = 1;
        if (peak >= 0.0001) {
            const double inputDb = 20 * std::log10(peak);
            attenuation = std::pow(10, (shapedDb(inputDb) - inputDb) / 20);
        }
        const double duration =
            attenuation < m_compressorGain ? attack : release;
        const double gainRatio =
            m_compressorGain / std::max(attenuation, 1e-12);
        const double ratioScale = std::max(1.0, std::abs(std::log(gainRatio)));
        const double step =
            duration == 0
                ? 1
                : 1 - std::exp(-ratioScale / (duration * m_sampleRate));
        m_compressorGain += (attenuation - m_compressorGain) * step;
        const double readPosition =
            static_cast<double>(m_position) - m_delayFrames;
        const int64_t earlier = static_cast<int64_t>(std::floor(readPosition));
        const double fraction = readPosition - earlier;
        std::array<float, 2>& slot = m_delay[m_position % m_delay.size()];
        for (size_t channel = 0; channel < source.channels(); channel++) {
            slot[channel] = source.channel(channel)[frame];
        }
        for (size_t channel = 0; channel < source.channels(); channel++) {
            const float first = earlier >= 0
                                    ? m_delay[static_cast<uint64_t>(earlier) %
                                              m_delay.size()][channel]
                                    : 0;
            const float second =
                earlier + 1 >= 0 ? m_delay[static_cast<uint64_t>(earlier + 1) %
                                           m_delay.size()][channel]
                                 : 0;
            const float delayed =
                static_cast<float>(first * (1 - fraction) + second * fraction);
            const float output =
                static_cast<float>(delayed * m_compressorGain * makeupGain);
            target.channel(channel)[frame] = output;
            active |= output != 0;
        }
        m_position++;
    }
    target.setSilent(!active);
    m_reduction.store(static_cast<float>(20 * std::log10(m_compressorGain)),
                      std::memory_order_relaxed);
}

StereoPannerHandler::StereoPannerHandler(double sampleRate)
    : AudioHandler(1, 1, 2)
    , m_pan(0, -1, 1)
    , m_sampleRate(sampleRate)
{
    input(0).configure(2, AudioInputChannelMode::ClampedMax, true);
}

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm
void StereoPannerHandler::process(uint64_t frameStart, size_t frames)
{
    const AudioBus& source = input(0).bus();
    if (source.isSilent()) {
        return;
    }
    AudioBus& target = output(0).bus();
    const AudioBus* modulation = m_pan.modulation(frames);
    const bool isARate = m_pan.isARate();
    const float quantumPan =
        m_pan.computedValueAt(static_cast<double>(frameStart) / m_sampleRate,
                              modulation ? modulation->channel(0)[0] : 0);
    constexpr double halfPi = 1.57079632679489661923;
    for (size_t frame = 0; frame < frames; frame++) {
        const double pan =
            isARate
                ? m_pan.computedValueAt(
                      static_cast<double>(frameStart + frame) / m_sampleRate,
                      modulation ? modulation->channel(0)[frame] : 0)
                : quantumPan;
        const bool mono = source.channels() == 1;
        const double x = mono ? (pan + 1) * 0.5 : (pan <= 0 ? pan + 1 : pan);
        const float gainLeft = static_cast<float>(std::cos(x * halfPi));
        const float gainRight = static_cast<float>(std::sin(x * halfPi));
        const float left = source.channel(0)[frame];
        if (mono) {
            target.channel(0)[frame] = left * gainLeft;
            target.channel(1)[frame] = left * gainRight;
        } else {
            const float right = source.channel(1)[frame];
            if (pan <= 0) {
                target.channel(0)[frame] = left + right * gainLeft;
                target.channel(1)[frame] = right * gainRight;
            } else {
                target.channel(0)[frame] = left * gainLeft;
                target.channel(1)[frame] = right + left * gainRight;
            }
        }
    }
    target.setSilent(false);
}

void GainHandler::inputChannelsChanged(size_t)
{
    output(0).configureChannels(input(0).bus().channels());
}

void GainHandler::process(uint64_t frameStart, size_t frames)
{
    const AudioBus& source = input(0).bus();
    if (source.isSilent()) {
        return;
    }
    AudioBus& target = output(0).bus();
    const AudioBus* modulation = m_gain.modulation(frames);
    const bool isARate = m_gain.isARate();
    const float quantumGain =
        isARate ? 0
                : m_gain.computedValueAt(
                      static_cast<double>(frameStart) / m_sampleRate,
                      modulation ? modulation->channel(0)[0] : 0);
    for (size_t frame = 0; frame < frames; frame++) {
        const float gain =
            isARate
                ? m_gain.computedValueAt(
                      static_cast<double>(frameStart + frame) / m_sampleRate,
                      modulation ? modulation->channel(0)[frame] : 0)
                : quantumGain;
        for (size_t channel = 0; channel < source.channels(); channel++) {
            target.channel(channel)[frame] =
                source.channel(channel)[frame] * gain;
        }
    }
    target.setSilent(false);
}

DelayHandler::DelayHandler(double sampleRate, double maxDelayTime)
    : AudioHandler(1, 1, 1)
    , m_delayTime(0, 0, static_cast<float>(maxDelayTime))
    , m_sampleRate(sampleRate)
    , m_ringFrames(static_cast<uint64_t>(std::ceil(maxDelayTime * sampleRate)) +
                   2 + AudioBus::RenderQuantumFrames)
{
}

float DelayHandler::sampleAt(int64_t frame, size_t channel,
                             size_t outputChannels) const
{
    if (frame < 0 || !m_ring ||
        static_cast<uint64_t>(frame) >= m_writtenFrames ||
        static_cast<uint64_t>(frame) + m_ringFrames < m_writtenFrames) {
        return 0;
    }
    const size_t channels = historyChannelsAt(frame);
    const size_t offset = static_cast<size_t>(frame % m_ringFrames);
    auto sample = [this, offset](size_t index) {
        return m_ring[index * m_ringFrames + offset];
    };
    if (channels == outputChannels) {
        return sample(channel);
    }
    if (channels == 1 && outputChannels == 2) {
        return sample(0);
    }
    if (channels == 2 && outputChannels == 1) {
        return 0.5f * (sample(0) + sample(1));
    }
    return channel < channels ? sample(channel) : 0;
}

size_t DelayHandler::historyChannelsAt(int64_t frame) const
{
    if (frame < 0) {
        return 1;
    }
    const auto it = std::upper_bound(
        m_layoutHistory.begin(), m_layoutHistory.end(),
        static_cast<uint64_t>(frame),
        [](uint64_t value, const std::pair<uint64_t, size_t>& entry) {
            return value < entry.first;
        });
    return it == m_layoutHistory.begin() ? 1 : std::prev(it)->second;
}

bool DelayHandler::hasPendingOutput() const
{
    return AudioHandler::hasPendingOutput() || m_ring;
}

void DelayHandler::writeInput(uint64_t frameStart, size_t frames)
{
    const AudioBus& source = input(0).bus();
    const size_t channels = source.channels();
    const size_t quantumFrames = AudioBus::RenderQuantumFrames;
    if (!source.isSilent()) {
        m_lastAudibleFrame = frameStart + frames;
        if (!m_ring || m_ringChannels < channels) {
            // Allocates only on the first audible input or when the input
            // gains channels; the ring is reused afterwards. Eager
            // allocation at construction would cost maxDelayTime of storage
            // per channel for every DelayNode, audible or not.
            std::unique_ptr<float[]> ring(new float[channels * m_ringFrames]());
            if (m_ring) {
                std::copy_n(m_ring.get(), m_ringChannels * m_ringFrames,
                            ring.get());
            }
            m_ring = std::move(ring);
            m_ringChannels = channels;
            if (!m_writtenFrames) {
                m_writtenFrames = frameStart;
            }
        }
    } else if (m_ring && frameStart > m_lastAudibleFrame + m_ringFrames) {
        // Everything audible has been delayed out of the ring.
        m_ring.reset();
        m_ringChannels = 0;
        m_layoutHistory.clear();
        m_writtenFrames = 0;
    }
    if (!m_ring) {
        return;
    }
    if (m_layoutHistory.empty() || m_layoutHistory.back().second != channels) {
        m_layoutHistory.emplace_back(frameStart, channels);
    }
    // Quanta in which this node was not rendered hold silence.
    const uint64_t gapStart = std::max(
        m_writtenFrames, frameStart > m_ringFrames ? frameStart - m_ringFrames
                                                   : static_cast<uint64_t>(0));
    for (uint64_t frame = gapStart; frame < frameStart; frame++) {
        for (size_t channel = 0; channel < m_ringChannels; channel++) {
            m_ring[channel * m_ringFrames + frame % m_ringFrames] = 0;
        }
    }
    for (size_t frame = 0; frame < quantumFrames; frame++) {
        const size_t offset =
            static_cast<size_t>((frameStart + frame) % m_ringFrames);
        for (size_t channel = 0; channel < channels; channel++) {
            m_ring[channel * m_ringFrames + offset] =
                frame < frames ? source.channel(channel)[frame] : 0;
        }
    }
    m_writtenFrames = frameStart + quantumFrames;
    const uint64_t earliest =
        m_writtenFrames > m_ringFrames ? m_writtenFrames - m_ringFrames : 0;
    while (m_layoutHistory.size() > 1 && m_layoutHistory[1].first <= earliest) {
        m_layoutHistory.pop_front();
    }
}

void DelayHandler::readOutput(uint64_t frameStart, size_t frames)
{
    const AudioBus* modulation = m_delayTime.modulation(frames);
    double readFrames[AudioBus::RenderQuantumFrames];
    size_t outputChannels = 1;
    for (size_t i = 0; i < frames; i++) {
        const double time = static_cast<double>(frameStart + i) / m_sampleRate;
        const double delay = m_delayTime.computedValueAt(
            time, modulation ? modulation->channel(0)[i] : 0);
        // https://webaudio.github.io/web-audio-api/#dom-delaynode-delaytime
        // A cycle's delay line cannot read from the current render quantum.
        const double delayFrames =
            m_inFeedbackCycle
                ? std::max(delay * m_sampleRate,
                           static_cast<double>(AudioBus::RenderQuantumFrames))
                : delay * m_sampleRate;
        readFrames[i] = static_cast<double>(frameStart + i) - delayFrames;
        outputChannels = std::max(
            outputChannels,
            historyChannelsAt(static_cast<int64_t>(std::floor(readFrames[i]))));
    }
    output(0).configureChannels(outputChannels);
    AudioBus& target = output(0).bus();
    if (!m_ring) {
        target.setSilent(true);
        return;
    }
    bool wrote = false;
    for (size_t i = 0; i < frames; i++) {
        const int64_t lower = static_cast<int64_t>(std::floor(readFrames[i]));
        const double fraction = readFrames[i] - std::floor(readFrames[i]);
        for (size_t channel = 0; channel < outputChannels; channel++) {
            const float first = sampleAt(lower, channel, outputChannels);
            const float second = sampleAt(lower + 1, channel, outputChannels);
            const float value =
                static_cast<float>(first + (second - first) * fraction);
            target.channel(channel)[i] = value;
            wrote |= value != 0;
        }
    }
    target.setSilent(!wrote);
}

// https://webaudio.github.io/web-audio-api/#DelayNode
// In a cycle the DelayReader runs before the nodes it feeds and the
// DelayWriter after the nodes feeding it.
void DelayHandler::renderDelayRead(uint64_t frameStart, size_t frames)
{
    zeroOutputs();
    readOutput(frameStart, frames);
}

void DelayHandler::renderDelayWrite(uint64_t frameStart, size_t frames)
{
    input(0).pull(frames);
    writeInput(frameStart, frames);
}

void DelayHandler::process(uint64_t frameStart, size_t frames)
{
    writeInput(frameStart, frames);
    readOutput(frameStart, frames);
}

IIRFilterHandler::IIRFilterHandler(std::vector<double>&& feedforward,
                                   std::vector<double>&& feedback)
    : AudioHandler(1, 1, 1)
    , m_feedforward(std::move(feedforward))
    , m_feedback(std::move(feedback))
{
    const double scale = m_feedback[0];
    for (double& coefficient : m_feedforward) {
        coefficient /= scale;
    }
    for (double& coefficient : m_feedback) {
        coefficient /= scale;
    }
}

// https://webaudio.github.io/web-audio-api/#iirfilternode-filter-definition
void IIRFilterHandler::process(uint64_t, size_t frames)
{
    const AudioBus& source = input(0).bus();
    const size_t channels = source.channels();
    output(0).configureChannels(channels);
    AudioBus& target = output(0).bus();
    if (m_channels.size() < channels) {
        m_channels.resize(channels);
    }
    bool wrote = false;
    for (size_t channel = 0; channel < channels; channel++) {
        ChannelState& state = m_channels[channel];
        const float* inputSamples = source.channel(channel);
        float* outputSamples = target.channel(channel);
        for (size_t frame = 0; frame < frames; frame++) {
            state.cursor = (state.cursor + 19) % 20;
            state.inputs[state.cursor] = inputSamples[frame];
            double value = 0;
            for (size_t index = 0; index < m_feedforward.size(); index++) {
                value += m_feedforward[index] *
                         state.inputs[(state.cursor + index) % 20];
            }
            for (size_t index = 1; index < m_feedback.size(); index++) {
                value -= m_feedback[index] *
                         state.outputs[(state.cursor + index) % 20];
            }
            state.outputs[state.cursor] = value;
            outputSamples[frame] = static_cast<float>(value);
            wrote |= outputSamples[frame] != 0;
        }
    }
    target.setSilent(!wrote);
}

BiquadFilterHandler::BiquadFilterHandler(double sampleRate)
    : AudioHandler(1, 1, 1)
    , m_frequency(350, 0, static_cast<float>(sampleRate / 2))
    , m_detune(0, -153600, 153600)
    , m_Q(1, -std::numeric_limits<float>::max(),
          std::numeric_limits<float>::max())
    , m_gain(0, -std::numeric_limits<float>::max(),
             static_cast<float>(40 * static_cast<float>(std::log10(
                                         std::numeric_limits<float>::max()))))
    , m_sampleRate(sampleRate)
{
}

// https://webaudio.github.io/web-audio-api/#biquadfilternode-filters-characteristics
BiquadFilterHandler::Coefficients BiquadFilterHandler::coefficients(
    double frequency, double detune, double Q, double gain) const
{
    constexpr double pi = 3.14159265358979323846;
    const double nyquist = m_sampleRate / 2;
    const double computedFrequency =
        std::max(0.0, std::min(nyquist, frequency * std::exp2(detune / 1200)));
    if (m_type == Type::Bandpass) {
        // The limiting transfer function at Q = 0 is unity inside the
        // band, while cutoff frequencies at either endpoint are silent.
        // https://webaudio.github.io/web-audio-api/#biquadfilternode-filters-characteristics
        if (computedFrequency == 0 || computedFrequency == nyquist) {
            return { 0, 0, 0, 0, 0 };
        }
        if (Q <= 0) {
            return { 1, 0, 0, 0, 0 };
        }
    }
    const double omega = 2 * pi * computedFrequency / m_sampleRate;
    const double sine = std::sin(omega);
    const double cosine = std::cos(omega);
    const double A = std::pow(10.0, gain / 40);
    const double alphaQ = sine / (2 * std::max(Q, 1e-10));
    const double alphaQdB = sine / (2 * std::pow(10.0, Q / 20));
    const double alphaS = sine / std::sqrt(2.0);
    const double rootA = std::sqrt(A);
    double b0 = 1;
    double b1 = 0;
    double b2 = 0;
    double a0 = 1;
    double a1 = 0;
    double a2 = 0;
    switch (m_type) {
    case Type::Lowpass:
        b0 = (1 - cosine) / 2;
        b1 = 1 - cosine;
        b2 = b0;
        a0 = 1 + alphaQdB;
        a1 = -2 * cosine;
        a2 = 1 - alphaQdB;
        break;
    case Type::Highpass:
        b0 = (1 + cosine) / 2;
        b1 = -(1 + cosine);
        b2 = b0;
        a0 = 1 + alphaQdB;
        a1 = -2 * cosine;
        a2 = 1 - alphaQdB;
        break;
    case Type::Bandpass:
        b0 = alphaQ;
        b1 = 0;
        b2 = -alphaQ;
        a0 = 1 + alphaQ;
        a1 = -2 * cosine;
        a2 = 1 - alphaQ;
        break;
    case Type::Notch:
        b0 = 1;
        b1 = -2 * cosine;
        b2 = 1;
        a0 = 1 + alphaQ;
        a1 = -2 * cosine;
        a2 = 1 - alphaQ;
        break;
    case Type::Allpass:
        b0 = 1 - alphaQ;
        b1 = -2 * cosine;
        b2 = 1 + alphaQ;
        a0 = 1 + alphaQ;
        a1 = -2 * cosine;
        a2 = 1 - alphaQ;
        break;
    case Type::Peaking:
        b0 = 1 + alphaQ * A;
        b1 = -2 * cosine;
        b2 = 1 - alphaQ * A;
        a0 = 1 + alphaQ / A;
        a1 = -2 * cosine;
        a2 = 1 - alphaQ / A;
        break;
    case Type::Lowshelf:
        b0 = A * ((A + 1) - (A - 1) * cosine + 2 * alphaS * rootA);
        b1 = 2 * A * ((A - 1) - (A + 1) * cosine);
        b2 = A * ((A + 1) - (A - 1) * cosine - 2 * alphaS * rootA);
        a0 = (A + 1) + (A - 1) * cosine + 2 * alphaS * rootA;
        a1 = -2 * ((A - 1) + (A + 1) * cosine);
        a2 = (A + 1) + (A - 1) * cosine - 2 * alphaS * rootA;
        break;
    case Type::Highshelf:
        b0 = A * ((A + 1) + (A - 1) * cosine + 2 * alphaS * rootA);
        b1 = -2 * A * ((A - 1) + (A + 1) * cosine);
        b2 = A * ((A + 1) + (A - 1) * cosine - 2 * alphaS * rootA);
        a0 = (A + 1) - (A - 1) * cosine + 2 * alphaS * rootA;
        a1 = 2 * ((A - 1) - (A + 1) * cosine);
        a2 = (A + 1) - (A - 1) * cosine - 2 * alphaS * rootA;
        break;
    }
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}

void BiquadFilterHandler::getFrequencyResponse(const float* frequencyHz,
                                               float* magnitude, float* phase,
                                               size_t length, double time) const
{
    const Coefficients c = coefficients(
        m_frequency.computedValueAt(time, 0), m_detune.computedValueAt(time, 0),
        m_Q.computedValueAt(time, 0), m_gain.computedValueAt(time, 0));
    constexpr double pi = 3.14159265358979323846;
    const double nyquist = m_sampleRate / 2;
    for (size_t i = 0; i < length; i++) {
        if (!std::isfinite(frequencyHz[i]) || frequencyHz[i] < 0 ||
            frequencyHz[i] > nyquist) {
            magnitude[i] = std::numeric_limits<float>::quiet_NaN();
            phase[i] = std::numeric_limits<float>::quiet_NaN();
            continue;
        }
        const double angle = -pi * frequencyHz[i] / nyquist;
        const std::complex<double> z(std::cos(angle), std::sin(angle));
        const std::complex<double> z2 = z * z;
        const std::complex<double> response =
            (c.b0 + c.b1 * z + c.b2 * z2) / (1.0 + c.a1 * z + c.a2 * z2);
        magnitude[i] = static_cast<float>(std::abs(response));
        phase[i] = static_cast<float>(std::arg(response));
    }
}

void BiquadFilterHandler::process(uint64_t frameStart, size_t frames)
{
    const AudioBus& source = input(0).bus();
    const size_t channels = source.channels();
    output(0).configureChannels(channels);
    AudioBus& target = output(0).bus();
    if (m_channels.size() < channels) {
        m_channels.resize(channels);
    }
    const AudioBus* frequencyMod = m_frequency.modulation(frames);
    const AudioBus* detuneMod = m_detune.modulation(frames);
    const AudioBus* QMod = m_Q.modulation(frames);
    const AudioBus* gainMod = m_gain.modulation(frames);
    Coefficients c{ 1, 0, 0, 0, 0 };
    double lastFrequency = std::numeric_limits<double>::quiet_NaN();
    double lastDetune = lastFrequency;
    double lastQ = lastFrequency;
    double lastGain = lastFrequency;
    bool wrote = false;
    for (size_t frame = 0; frame < frames; frame++) {
        const double time =
            static_cast<double>(frameStart + frame) / m_sampleRate;
        const double frequency = m_frequency.computedValueAt(
            time, frequencyMod ? frequencyMod->channel(0)[frame] : 0);
        const double detune = m_detune.computedValueAt(
            time, detuneMod ? detuneMod->channel(0)[frame] : 0);
        const double Q =
            m_Q.computedValueAt(time, QMod ? QMod->channel(0)[frame] : 0);
        const double gain = m_gain.computedValueAt(
            time, gainMod ? gainMod->channel(0)[frame] : 0);
        if (frequency != lastFrequency || detune != lastDetune || Q != lastQ ||
            gain != lastGain) {
            c = coefficients(frequency, detune, Q, gain);
            lastFrequency = frequency;
            lastDetune = detune;
            lastQ = Q;
            lastGain = gain;
        }
        for (size_t channel = 0; channel < channels; channel++) {
            ChannelState& state = m_channels[channel];
            const double inputValue = source.channel(channel)[frame];
            const double value = c.b0 * inputValue + c.b1 * state.x1 +
                                 c.b2 * state.x2 - c.a1 * state.y1 -
                                 c.a2 * state.y2;
            state.x2 = state.x1;
            state.x1 = inputValue;
            state.y2 = state.y1;
            state.y1 = value;
            target.channel(channel)[frame] = static_cast<float>(value);
            wrote |= target.channel(channel)[frame] != 0;
        }
    }
    target.setSilent(!wrote);
}

ConstantSourceHandler::ConstantSourceHandler(double sampleRate)
    : AudioHandler(0, 1, 1)
    , m_offset(1)
    , m_sampleRate(sampleRate)
{
}

void ConstantSourceHandler::start(double when)
{
    m_startTime = when;
    m_started = true;
}

void ConstantSourceHandler::stop(double when)
{
    m_stopTime = when;
    m_hasStopTime = true;
}

// https://webaudio.github.io/web-audio-api/#ConstantSourceNode
void ConstantSourceHandler::process(uint64_t frameStart, size_t frames)
{
    if (!m_started || m_finished) {
        return;
    }
    const double startFrame = std::ceil(m_startTime * m_sampleRate);
    const double stopFrame = m_hasStopTime
                                 ? std::ceil(m_stopTime * m_sampleRate)
                                 : std::numeric_limits<double>::infinity();
    AudioBus& bus = output(0).bus();
    const AudioBus* modulation = m_offset.modulation(frames);
    const bool isARate = m_offset.isARate();
    const float quantumOffset =
        isARate ? 0
                : m_offset.computedValueAt(
                      static_cast<double>(frameStart) / m_sampleRate,
                      modulation ? modulation->channel(0)[0] : 0);
    bool wrote = false;
    for (size_t i = 0; i < frames; i++) {
        const double frame = static_cast<double>(frameStart + i);
        if (frame < startFrame || frame >= stopFrame) {
            continue;
        }
        bus.channel(0)[i] =
            isARate ? m_offset.computedValueAt(
                          frame / m_sampleRate,
                          modulation ? modulation->channel(0)[i] : 0)
                    : quantumOffset;
        wrote = true;
    }
    bus.setSilent(!wrote);
    m_finished = m_hasStopTime && static_cast<double>(frameStart + frames) >=
                                      std::max(startFrame, stopFrame);
}

OscillatorHandler::OscillatorHandler(double sampleRate)
    : AudioHandler(0, 1, 1)
    , m_frequency(440, static_cast<float>(-sampleRate / 2),
                  static_cast<float>(sampleRate / 2))
    , m_detune(0, -153600, 153600)
    , m_sampleRate(sampleRate)
{
}

OscillatorHandler::~OscillatorHandler()
{
    if (m_periodicWave) {
        m_periodicWave->release();
    }
}

void OscillatorHandler::setWaveform(Waveform waveform)
{
    m_waveform = waveform;
    if (m_periodicWave) {
        m_periodicWave->release();
        m_periodicWave = nullptr;
    }
}

void OscillatorHandler::setPeriodicWave(PeriodicWaveData* wave)
{
    wave->prepare();
    wave->retain();
    if (m_periodicWave) {
        m_periodicWave->release();
    }
    m_periodicWave = wave;
    m_waveform = Waveform::Custom;
}

void OscillatorHandler::start(double when)
{
    m_startTime = when;
    m_started = true;
    m_finished = false;
    m_playbackBegun = false;
    m_phase = 0;
}

void OscillatorHandler::stop(double when)
{
    m_stopTime = when;
    m_hasStopTime = true;
}

static double oscillatorPolyBlep(double phase, double step)
{
    if (step <= 0) {
        return 0;
    }
    if (phase < step) {
        const double x = phase / step;
        return x + x - x * x - 1;
    }
    if (phase > 1 - step) {
        const double x = (phase - 1) / step;
        return x * x + x + x + 1;
    }
    return 0;
}

// https://webaudio.github.io/web-audio-api/#OscillatorNode
void OscillatorHandler::process(uint64_t frameStart, size_t frames)
{
    if (!m_started || m_finished) {
        return;
    }
    constexpr double twoPi = 6.28318530717958647692;
    const double startFrame = std::ceil(m_startTime * m_sampleRate);
    const double stopFrame = m_hasStopTime
                                 ? std::ceil(m_stopTime * m_sampleRate)
                                 : std::numeric_limits<double>::infinity();
    const AudioBus* frequencyModulation = m_frequency.modulation(frames);
    const AudioBus* detuneModulation = m_detune.modulation(frames);
    AudioBus& bus = output(0).bus();
    bool wrote = false;
    for (size_t i = 0; i < frames; i++) {
        const double frame = static_cast<double>(frameStart + i);
        if (frame < startFrame || frame >= stopFrame) {
            continue;
        }
        const double time = frame / m_sampleRate;
        const double frequency = m_frequency.computedValueAt(
            time, frequencyModulation ? frequencyModulation->channel(0)[i] : 0);
        const double detune = m_detune.computedValueAt(
            time, detuneModulation ? detuneModulation->channel(0)[i] : 0);
        const double computedFrequency =
            frequency * std::pow(2.0, detune / 1200.0);
        const bool audible = std::isfinite(computedFrequency) &&
                             std::abs(computedFrequency) < m_sampleRate / 2;
        const double step = audible ? computedFrequency / m_sampleRate : 0;
        if (!m_playbackBegun) {
            // The first frame may follow a fractional start time.
            m_phase = step * (frame - m_startTime * m_sampleRate);
            m_phase -= std::floor(m_phase);
            m_playbackBegun = true;
        }
        double sample = 0;
        if (audible) {
            const double phase = m_phase;
            const double width = std::abs(step);
            switch (m_waveform) {
            case Waveform::Sine:
                sample = std::sin(twoPi * phase);
                break;
            case Waveform::Square:
                sample = phase < 0.5 ? 1 : -1;
                sample += oscillatorPolyBlep(phase, width);
                sample -= oscillatorPolyBlep(
                    phase < 0.5 ? phase + 0.5 : phase - 0.5, width);
                break;
            case Waveform::Sawtooth: {
                const double shifted = phase < 0.5 ? phase + 0.5 : phase - 0.5;
                sample = 2 * shifted - 1 - oscillatorPolyBlep(shifted, width);
                break;
            }
            case Waveform::Triangle:
                sample = (2.0 / 3.14159265358979323846) *
                         std::asin(std::sin(twoPi * phase));
                break;
            case Waveform::Custom: {
                const size_t maxHarmonic =
                    step == 0 ? 8191
                              : static_cast<size_t>(std::floor(0.5 / width));
                sample = m_periodicWave->sample(phase, maxHarmonic);
                break;
            }
            }
        }
        bus.channel(0)[i] = static_cast<float>(sample);
        wrote = true;
        m_phase += step;
        m_phase -= std::floor(m_phase);
    }
    bus.setSilent(!wrote);
    m_finished = m_hasStopTime && static_cast<double>(frameStart + frames) >=
                                      std::max(startFrame, stopFrame);
}

AudioBufferSourceHandler::AudioBufferSourceHandler(double sampleRate)
    : AudioHandler(0, 1, 1)
    , m_playbackRate(1)
    , m_detune(0)
    , m_contextSampleRate(sampleRate)
{
}

AudioBufferSourceHandler::~AudioBufferSourceHandler()
{
    if (m_data) {
        m_data->release();
    }
}

void AudioBufferSourceHandler::setBuffer(AudioBufferData* data,
                                         double sampleRate)
{
    // https://webaudio.github.io/web-audio-api/#acquire-the-content
    // Share the acquired PCM; AudioBuffer copies it only on later mutation.
    if (data) {
        data->retain();
    }
    if (m_data) {
        m_data->release();
    }
    m_data = data;
    m_bufferSampleRate = sampleRate;
    if (m_started && m_elapsedBufferFrames == 0) {
        m_position = m_offset * m_bufferSampleRate;
    }
    output(0).configureChannels(data ? data->channels() : 1);
}

void AudioBufferSourceHandler::start(double when, double offset,
                                     bool hasDuration, double duration)
{
    m_startTime = when;
    m_offset = offset;
    m_hasDuration = hasDuration;
    m_duration = duration;
    m_started = true;
    m_finished = false;
    m_position = m_offset * m_bufferSampleRate;
    m_elapsedBufferFrames = 0;
    m_enteredLoop = false;
    m_playbackBegun = false;
}

void AudioBufferSourceHandler::stop(double when)
{
    m_stopTime = when;
    m_hasStopTime = true;
}

// https://webaudio.github.io/web-audio-api/#playback-AudioBufferSourceNode
void AudioBufferSourceHandler::process(uint64_t frameStart, size_t frames)
{
    if (!m_started || m_finished) {
        return;
    }
    // With no content at the first render quantum, the source has no audio
    // to schedule and its ended event must not wait for a future start time.
    if (!m_data) {
        m_finished = true;
        return;
    }

    AudioBus& bus = output(0).bus();
    const double sourceFrames = m_data ? m_data->frames() : 0;
    const double startFrame = std::ceil(m_startTime * m_contextSampleRate);
    const double stopFrame = m_hasStopTime
                                 ? std::ceil(m_stopTime * m_contextSampleRate)
                                 : std::numeric_limits<double>::infinity();
    const double durationFrames = m_duration * m_bufferSampleRate;
    const AudioBus* rateModulation = m_playbackRate.modulation(frames);
    const AudioBus* detuneModulation = m_detune.modulation(frames);
    const double quantumTime =
        static_cast<double>(frameStart) / m_contextSampleRate;
    const double playbackRate = m_playbackRate.computedValueAt(
        quantumTime, rateModulation ? rateModulation->channel(0)[0] : 0);
    const double detune = m_detune.computedValueAt(
        quantumTime, detuneModulation ? detuneModulation->channel(0)[0] : 0);
    // https://webaudio.github.io/web-audio-api/#computed-playback-rate
    double rate = playbackRate * std::pow(2.0, detune / 1200.0) *
                  m_bufferSampleRate / m_contextSampleRate;
    if (!std::isfinite(rate)) {
        rate = 0;
    }
    if (m_data) {
        m_position = std::min(m_position, sourceFrames);
    }
    double loopStartFrame = 0;
    double loopEndFrame = sourceFrames;
    if (m_loop && m_data && m_loopStart >= 0 && m_loopEnd > 0 &&
        m_loopStart < m_loopEnd &&
        m_loopStart * m_bufferSampleRate < sourceFrames) {
        loopStartFrame = m_loopStart * m_bufferSampleRate;
        loopEndFrame = std::min(m_loopEnd * m_bufferSampleRate, sourceFrames);
    }
    bool wrote = false;
    for (size_t i = 0; i < frames; i++) {
        const double contextFrame = static_cast<double>(frameStart + i);
        if (contextFrame >= stopFrame) {
            m_finished = true;
            break;
        }
        if (contextFrame < startFrame) {
            continue;
        }
        if (!m_data ||
            (m_hasDuration && m_elapsedBufferFrames >= durationFrames)) {
            m_finished = true;
            break;
        }
        if (!m_playbackBegun) {
            m_playbackBegun = true;
            if (m_loop && rate >= 0 && m_position >= loopEndFrame) {
                m_position = loopEndFrame;
            } else if (m_loop && rate < 0 && m_position < loopStartFrame) {
                m_position = loopStartFrame;
            }
        }
        if (m_loop) {
            if (!m_enteredLoop) {
                const double initialPosition = m_offset * m_bufferSampleRate;
                if (rate >= 0) {
                    m_enteredLoop = initialPosition >= loopEndFrame
                                        ? m_position >= loopEndFrame
                                        : m_position >= loopStartFrame;
                } else {
                    m_enteredLoop = initialPosition >= loopEndFrame
                                        ? m_position < loopEndFrame
                                        : m_position >= loopStartFrame;
                }
            }
            if (m_enteredLoop &&
                (m_position < loopStartFrame || m_position >= loopEndFrame)) {
                const double span = loopEndFrame - loopStartFrame;
                m_position =
                    loopStartFrame +
                    std::fmod(std::fmod(m_position - loopStartFrame, span) +
                                  span,
                              span);
            }
        }
        if (m_position >= sourceFrames && rate < 0) {
            m_position += rate;
            m_elapsedBufferFrames += std::abs(rate);
            continue;
        }
        if (m_position < 0 || m_position >= sourceFrames) {
            m_finished = true;
            break;
        }
        const size_t first = static_cast<size_t>(m_position);
        const size_t second = std::min(first + 1, m_data->frames() - 1);
        const bool crossesLoopEnd =
            m_loop && m_enteredLoop && first + 1 >= loopEndFrame;
        const float fraction = static_cast<float>(
            crossesLoopEnd ? (m_position - first) / (loopEndFrame - first)
                           : m_position - first);
        for (size_t channel = 0; channel < m_data->channels(); channel++) {
            const float* samples = m_data->channel(channel);
            float next = samples[second];
            if (!crossesLoopEnd && second == first && first > 0) {
                // Extend the last two samples' slope so resampling does not
                // insert a flat segment at a non-looped buffer boundary.
                next += samples[first] - samples[first - 1];
            }
            if (crossesLoopEnd) {
                const size_t loopFirst = static_cast<size_t>(loopStartFrame);
                const size_t loopSecond =
                    std::min(loopFirst + 1, m_data->frames() - 1);
                const float loopFraction =
                    static_cast<float>(loopStartFrame - loopFirst);
                next =
                    samples[loopFirst] +
                    (samples[loopSecond] - samples[loopFirst]) * loopFraction;
            }
            bus.channel(channel)[i] =
                samples[first] + (next - samples[first]) * fraction;
        }
        wrote = true;
        m_position += rate;
        m_elapsedBufferFrames += std::abs(rate);
    }
    bus.setSilent(!wrote);
    if (m_data && !m_loop &&
        static_cast<double>(frameStart + frames) > startFrame &&
        (m_position < 0 || m_position >= sourceFrames ||
         (m_hasDuration && m_elapsedBufferFrames >= durationFrames))) {
        m_finished = true;
    }
    if (m_finished && m_data) {
        m_data->release();
        m_data = nullptr;
    }
}

} // namespace Starfish

#endif
