/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioHandlers__
#define __StarfishAudioHandlers__

#include "core/modules/webaudio/render/AudioGraph.h"
#include "core/modules/webaudio/render/AudioParamTimeline.h"

#include <array>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <utility>

namespace Starfish {

class AudioBufferData;
class PeriodicWaveData;

// Native-only media clock and PCM. The player and graph each hold one ref;
// neither side retains a GC-managed object across the render boundary.
class MediaAudioPlaybackState {
public:
    MediaAudioPlaybackState();
    ~MediaAudioPlaybackState();
    void retain();
    void release();
    void setPCM(AudioBufferData* pcm);
    void setStreamingPosition(double position, bool playing);
    void appendStreamingPCM(const int16_t* samples, size_t frames,
                            size_t channels, uint32_t sampleRate,
                            double position);
    void clearStreamingPCM();
    void setPlaying(bool playing);
    void seek(double time);
    void setVolume(double volume);
    void setMuted(bool muted);
    void setLoop(bool loop);
    void setRate(double rate);
    void setOriginClean(bool clean);
    void setRouted(bool routed);
    bool routed() const;
    // A consistent view of the media clock for the graph's read cursor.
    struct Clock {
        double position;
        double rate;
        double length;
        uint64_t generation;
        bool playing;
        bool loop;
    };
    Clock clock() const;
    double currentTime() const;
    double playbackRate() const;
    double duration() const;
    bool ended() const;
    size_t channels() const;
    void render(AudioBus& bus, size_t frames, double outputRate,
                double position, bool forGraph) const;

private:
    struct StreamingData;
    double currentTimeLocked() const;
    void renderStreamingLocked(AudioBus& bus, size_t frames, double outputRate,
                               double position) const;

    std::atomic<unsigned> m_references{ 1 };
    mutable std::mutex m_mutex;
    AudioBufferData* m_pcm{ nullptr };
    std::unique_ptr<StreamingData> m_streaming;
    std::chrono::steady_clock::time_point m_startedAt;
    double m_positionAtStart{ 0 };
    double m_volume{ 1 };
    double m_rate{ 1 };
    // Bumped whenever the media timeline jumps (seek, play/pause, rate,
    // loop or source change), so the graph re-anchors its read cursor.
    uint64_t m_generation{ 0 };
    bool m_playing{ false };
    bool m_muted{ false };
    bool m_loop{ false };
    bool m_originClean{ false };
    bool m_routed{ false };
};

class MediaElementSourceHandler final : public AudioHandler {
public:
    explicit MediaElementSourceHandler(double sampleRate);
    ~MediaElementSourceHandler() override;
    void setPlaybackState(MediaAudioPlaybackState* state);
    // Once the media element is gone nothing advances its clock.
    bool hasPendingOutput() const override
    {
        return false;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    MediaAudioPlaybackState* m_state{ nullptr };
    double m_sampleRate;
    // Media time of the next frame to render. It advances by exactly one
    // quantum per quantum, so blocks are neither repeated nor skipped.
    double m_cursor{ 0 };
    uint64_t m_generation{ 0 };
    bool m_hasCursor{ false };
};

class AudioSilentSourceHandler final : public AudioHandler {
public:
    AudioSilentSourceHandler()
        : AudioHandler(0, 1, 1)
    {
    }

protected:
    void process(uint64_t, size_t) override
    {
    }
};

class AudioDestinationHandler final : public AudioHandler {
public:
    explicit AudioDestinationHandler(size_t channels);
    void configureChannels(size_t channels);

protected:
    void process(uint64_t frameStart, size_t frames) override;
};

// https://webaudio.github.io/web-audio-api/#ChannelMergerNode
// The output has numberOfInputs channels while any input is actively
// processing, otherwise a single channel of silence. AudioBus keeps its
// allocation when shrinking, so switching between the two does not allocate.
class ChannelMergerHandler final : public AudioHandler {
public:
    explicit ChannelMergerHandler(size_t inputs);

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    size_t m_inputCount;
};

class ChannelSplitterHandler final : public AudioHandler {
public:
    explicit ChannelSplitterHandler(size_t outputs);

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    size_t m_outputCount;
};

class GainHandler final : public AudioHandler {
public:
    explicit GainHandler(double sampleRate);
    AudioParamTimeline* gainTimeline()
    {
        return &m_gain;
    }
    void inputChannelsChanged(size_t index) override;
    size_t timelineCount() const override
    {
        return 1;
    }
    AudioParamTimeline* timeline(size_t) override
    {
        return &m_gain;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioParamTimeline m_gain;
    double m_sampleRate;
};

class DelayHandler final : public AudioHandler {
public:
    DelayHandler(double sampleRate, double maxDelayTime);
    bool isDelayNode() const override
    {
        return true;
    }
    void setFeedbackCycle(bool inCycle) override
    {
        m_inFeedbackCycle = inCycle;
    }
    void renderDelayRead(uint64_t frameStart, size_t frames) override;
    void renderDelayWrite(uint64_t frameStart, size_t frames) override;
    AudioParamTimeline* delayTimeline()
    {
        return &m_delayTime;
    }
    size_t timelineCount() const override
    {
        return 1;
    }
    AudioParamTimeline* timeline(size_t) override
    {
        return &m_delayTime;
    }
    bool hasPendingOutput() const override;

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    float sampleAt(int64_t frame, size_t channel, size_t outputChannels) const;
    size_t historyChannelsAt(int64_t frame) const;
    void writeInput(uint64_t frameStart, size_t frames);
    void readOutput(uint64_t frameStart, size_t frames);

    AudioParamTimeline m_delayTime;
    double m_sampleRate;
    // Frames retained: the longest delay, interpolation and one quantum.
    uint64_t m_ringFrames;
    // Planar ring of m_ringChannels x m_ringFrames, allocated on the first
    // audible input and grown only when more channels arrive; it is freed
    // once all of its audio has been delayed out.
    std::unique_ptr<float[]> m_ring;
    size_t m_ringChannels{ 0 };
    uint64_t m_writtenFrames{ 0 };
    uint64_t m_lastAudibleFrame{ 0 };
    // (first frame, channel count) for each input layout still in the ring.
    std::deque<std::pair<uint64_t, size_t>> m_layoutHistory;
    bool m_inFeedbackCycle{ false };
};

class IIRFilterHandler final : public AudioHandler {
public:
    IIRFilterHandler(std::vector<double>&& feedforward,
                     std::vector<double>&& feedback);
    const std::vector<double>& feedforward() const
    {
        return m_feedforward;
    }
    const std::vector<double>& feedback() const
    {
        return m_feedback;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    struct ChannelState {
        std::array<double, 20> inputs{};
        std::array<double, 20> outputs{};
        size_t cursor{ 0 };
    };

    std::vector<double> m_feedforward;
    std::vector<double> m_feedback;
    std::vector<ChannelState> m_channels;
};

class BiquadFilterHandler final : public AudioHandler {
public:
    enum class Type {
        Lowpass,
        Highpass,
        Bandpass,
        Lowshelf,
        Highshelf,
        Peaking,
        Notch,
        Allpass
    };
    explicit BiquadFilterHandler(double sampleRate);
    void setType(Type type)
    {
        m_type = type;
    }
    AudioParamTimeline* frequencyTimeline()
    {
        return &m_frequency;
    }
    AudioParamTimeline* detuneTimeline()
    {
        return &m_detune;
    }
    AudioParamTimeline* QTimeline()
    {
        return &m_Q;
    }
    AudioParamTimeline* gainTimeline()
    {
        return &m_gain;
    }
    void getFrequencyResponse(const float* frequencyHz, float* magnitude,
                              float* phase, size_t length, double time) const;
    size_t timelineCount() const override
    {
        return 4;
    }
    AudioParamTimeline* timeline(size_t index) override
    {
        AudioParamTimeline* timelines[] = { &m_frequency, &m_detune, &m_Q,
                                            &m_gain };
        return timelines[index];
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    struct Coefficients {
        double b0;
        double b1;
        double b2;
        double a1;
        double a2;
    };
    struct ChannelState {
        double x1{ 0 };
        double x2{ 0 };
        double y1{ 0 };
        double y2{ 0 };
    };
    Coefficients coefficients(double frequency, double detune, double Q,
                              double gain) const;

    AudioParamTimeline m_frequency;
    AudioParamTimeline m_detune;
    AudioParamTimeline m_Q;
    AudioParamTimeline m_gain;
    Type m_type{ Type::Lowpass };
    double m_sampleRate;
    std::vector<ChannelState> m_channels;
};

class WaveShaperHandler final : public AudioHandler {
public:
    WaveShaperHandler();
    void setCurve(std::vector<float>&& curve)
    {
        m_curve = std::move(curve);
    }
    const std::vector<float>& curve() const
    {
        return m_curve;
    }
    void setOversample(size_t factor)
    {
        m_oversample = factor;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    float shape(float value) const;

    std::vector<float> m_curve;
    std::vector<float> m_previousInput;
    size_t m_oversample{ 1 };
};

class AnalyserHandler final : public AudioHandler {
public:
    AnalyserHandler();
    // Control thread, under the graph lock.
    void setFftSize(size_t size);
    void setSmoothing(double value)
    {
        m_smoothing = value;
    }
    size_t fftSize() const
    {
        return m_fftSize;
    }
    float timeSample(size_t index) const;
    void copyTimeDomain(float* destination, size_t length) const;
    const std::vector<float>& frequencyData();

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioBus m_mono{ 1 };
    // The most recent fftSize input frames; allocated on first audible input.
    std::unique_ptr<float[]> m_history;
    uint64_t m_writtenFrames{ 0 };
    uint64_t m_lastSpectrumFrame{ static_cast<uint64_t>(-1) };
    size_t m_fftSize{ 2048 };
    double m_smoothing{ 0.8 };
    std::vector<float> m_smoothed;
    std::vector<float> m_frequencies;
};

class DynamicsCompressorHandler final : public AudioHandler {
public:
    explicit DynamicsCompressorHandler(double sampleRate);
    AudioParamTimeline* thresholdTimeline()
    {
        return &m_threshold;
    }
    AudioParamTimeline* kneeTimeline()
    {
        return &m_knee;
    }
    AudioParamTimeline* ratioTimeline()
    {
        return &m_ratio;
    }
    AudioParamTimeline* attackTimeline()
    {
        return &m_attack;
    }
    AudioParamTimeline* releaseTimeline()
    {
        return &m_release;
    }
    float reduction() const
    {
        return m_reduction.load(std::memory_order_relaxed);
    }
    size_t timelineCount() const override
    {
        return 5;
    }
    AudioParamTimeline* timeline(size_t index) override
    {
        AudioParamTimeline* timelines[] = { &m_threshold, &m_knee, &m_ratio,
                                            &m_attack, &m_release };
        return timelines[index];
    }
    bool hasPendingOutput() const override
    {
        // The look-ahead delay line still holds input once output is silent.
        return AudioHandler::hasPendingOutput() || !m_delay.empty();
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioParamTimeline m_threshold;
    AudioParamTimeline m_knee;
    AudioParamTimeline m_ratio;
    AudioParamTimeline m_attack;
    AudioParamTimeline m_release;
    double m_sampleRate;
    double m_delayFrames;
    std::vector<std::array<float, 2>> m_delay;
    uint64_t m_position{ 0 };
    uint64_t m_silentFrames{ 0 };
    double m_compressorGain{ 1 };
    std::atomic<float> m_reduction{ 0 };
};

class StereoPannerHandler final : public AudioHandler {
public:
    explicit StereoPannerHandler(double sampleRate);
    AudioParamTimeline* panTimeline()
    {
        return &m_pan;
    }
    size_t timelineCount() const override
    {
        return 1;
    }
    AudioParamTimeline* timeline(size_t) override
    {
        return &m_pan;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioParamTimeline m_pan;
    double m_sampleRate;
};

class ConstantSourceHandler final : public AudioHandler {
public:
    explicit ConstantSourceHandler(double sampleRate);
    AudioParamTimeline* offsetTimeline()
    {
        return &m_offset;
    }
    size_t timelineCount() const override
    {
        return 1;
    }
    AudioParamTimeline* timeline(size_t) override
    {
        return &m_offset;
    }
    void start(double when);
    void stop(double when);
    bool finished() const override
    {
        return m_finished;
    }
    bool hasPendingOutput() const override
    {
        return m_started && !m_finished;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioParamTimeline m_offset;
    double m_sampleRate;
    double m_startTime{ 0 };
    double m_stopTime{ 0 };
    bool m_started{ false };
    bool m_hasStopTime{ false };
    bool m_finished{ false };
};

class OscillatorHandler final : public AudioHandler {
public:
    enum class Waveform { Sine, Square, Sawtooth, Triangle, Custom };

    explicit OscillatorHandler(double sampleRate);
    ~OscillatorHandler() override;
    AudioParamTimeline* frequencyTimeline()
    {
        return &m_frequency;
    }
    AudioParamTimeline* detuneTimeline()
    {
        return &m_detune;
    }
    size_t timelineCount() const override
    {
        return 2;
    }
    AudioParamTimeline* timeline(size_t index) override
    {
        return index ? &m_detune : &m_frequency;
    }
    void setWaveform(Waveform waveform);
    void setPeriodicWave(PeriodicWaveData* wave);
    void start(double when);
    void stop(double when);
    bool finished() const override
    {
        return m_finished;
    }
    bool hasPendingOutput() const override
    {
        return m_started && !m_finished;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioParamTimeline m_frequency;
    AudioParamTimeline m_detune;
    double m_sampleRate;
    double m_startTime{ 0 };
    double m_stopTime{ 0 };
    double m_phase{ 0 };
    Waveform m_waveform{ Waveform::Sine };
    PeriodicWaveData* m_periodicWave{ nullptr };
    bool m_started{ false };
    bool m_playbackBegun{ false };
    bool m_hasStopTime{ false };
    bool m_finished{ false };
};

class AudioBufferSourceHandler final : public AudioHandler {
public:
    explicit AudioBufferSourceHandler(double sampleRate);
    ~AudioBufferSourceHandler() override;

    AudioParamTimeline* playbackRateTimeline()
    {
        return &m_playbackRate;
    }
    AudioParamTimeline* detuneTimeline()
    {
        return &m_detune;
    }
    size_t timelineCount() const override
    {
        return 2;
    }
    AudioParamTimeline* timeline(size_t index) override
    {
        return index ? &m_detune : &m_playbackRate;
    }
    bool hasPendingOutput() const override
    {
        return m_started && !m_finished;
    }

    void setBuffer(AudioBufferData* data, double sampleRate);
    void start(double when, double offset, bool hasDuration, double duration);
    void stop(double when);
    void setLoop(bool loop, double loopStart, double loopEnd)
    {
        m_loop = loop;
        m_loopStart = loopStart;
        m_loopEnd = loopEnd;
        if (!loop) {
            m_enteredLoop = false;
        }
    }
    bool finished() const override
    {
        return m_finished;
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    AudioParamTimeline m_playbackRate;
    AudioParamTimeline m_detune;
    AudioBufferData* m_data{ nullptr };
    double m_contextSampleRate;
    double m_bufferSampleRate{ 0 };
    double m_startTime{ 0 };
    double m_offset{ 0 };
    double m_duration{ 0 };
    double m_stopTime{ 0 };
    double m_position{ 0 };
    double m_elapsedBufferFrames{ 0 };
    bool m_started{ false };
    bool m_hasDuration{ false };
    bool m_hasStopTime{ false };
    bool m_finished{ false };
    bool m_loop{ false };
    bool m_enteredLoop{ false };
    bool m_playbackBegun{ false };
    double m_loopStart{ 0 };
    double m_loopEnd{ 0 };
};

} // namespace Starfish

#endif
#endif
