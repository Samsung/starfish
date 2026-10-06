/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)
#ifndef __StarfishConvolverHandler__
#define __StarfishConvolverHandler__

#include "core/modules/webaudio/render/AudioGraph.h"

#include <array>
#include <complex>
#include <memory>
#include <vector>

namespace Starfish {
class AudioBufferData;

class ConvolverHandler final : public AudioHandler {
private:
    static constexpr size_t BlockSize = AudioBus::RenderQuantumFrames;
    static constexpr size_t FFTSize = BlockSize * 2;
    // Spectra of real signals are conjugate-symmetric; only bins 0..N/2 are
    // stored and multiplied.
    static constexpr size_t SpectrumBins = FFTSize / 2 + 1;
    using Spectrum = std::array<std::complex<float>, FFTSize>;
    using HalfSpectrum = std::array<std::complex<float>, SpectrumBins>;

public:
    // Prepared impulse response. Building it runs every FFT, so it is done
    // without the graph lock and installed with swapResponse().
    struct Response {
        size_t channels{ 0 };
        size_t length{ 0 };
        size_t partitions{ 0 };
        std::array<std::vector<float>, 4> shortResponse;
        std::array<std::vector<HalfSpectrum>, 4> spectra;
    };

    ConvolverHandler();
    // https://webaudio.github.io/web-audio-api/#dom-convolvernode-normalize
    static std::unique_ptr<Response> createResponse(const AudioBufferData* data,
                                                    bool normalize,
                                                    double sampleRate);
    // Under the graph lock. Returns the previous response so the caller can
    // free it after releasing the lock. A null response clears the buffer.
    std::unique_ptr<Response> swapResponse(std::unique_ptr<Response> response);
    void setSpeakers(bool speakers)
    {
        m_speakers = speakers;
    }
    void inputChannelsChanged(size_t index) override;
    bool hasPendingOutput() const override
    {
        return m_audiblePartitions || AudioHandler::hasPendingOutput();
    }

protected:
    void process(uint64_t frameStart, size_t frames) override;

private:
    static void fft(Spectrum& values, bool inverse);
    void processShort(size_t frames);
    void processPartitioned(size_t frames);
    void resetHistory();
    void updateOutputChannels();

    std::unique_ptr<Response> m_response;
    size_t m_responseChannels{ 0 };
    size_t m_responseLength{ 0 };
    size_t m_partitions{ 0 };
    size_t m_cursor{ 0 };
    size_t m_shortCursor{ 0 };
    bool m_speakers{ true };
    bool m_hadStereoInput{ false };
    std::array<std::array<float, 32>, 2> m_shortHistory{};
    std::array<std::vector<HalfSpectrum>, 2> m_inputSpectra;
    // Input partitions that hold silence contribute nothing to the sum.
    std::vector<bool> m_silentPartitions;
    size_t m_audiblePartitions{ 0 };
    std::array<std::array<float, BlockSize>, 2> m_overlap{};
};
} // namespace Starfish
#endif
#endif
