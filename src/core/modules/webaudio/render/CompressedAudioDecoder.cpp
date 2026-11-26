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
#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "core/modules/webaudio/render/AudioBus.h"
#include "platform/multimedia/MP3MemoryDemuxer.h"
#include "platform/multimedia/AudioContainerReader.h"

// Linux decodes with FFmpeg only in the FFmpeg media player build; Tizen
// Web Audio builds always link the platform's FFmpeg.
#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER) || defined(STARFISH_TIZEN)
#define STARFISH_FFMPEG_AUDIO_DECODER
#endif

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <vector>

#if defined(STARFISH_FFMPEG_AUDIO_DECODER)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/vorbis_parser.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#if !defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
#include <libswresample/swresample.h>
#endif
}

#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
#include "core/modules/webaudio/render/AudioPCMResampler.h"
#endif
#endif

namespace Starfish {

#if defined(STARFISH_FFMPEG_AUDIO_DECODER)
namespace {

    // Output frames per resampler call, bounding the conversion buffer even
    // for extreme sample-rate ratios.
    constexpr int64_t ConversionFrames = 4096;
    constexpr int MaxChannels = 32;

#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
    std::vector<float> channelMix(const AVChannelLayout& input,
                                  const AVChannelLayout& output)
    {
        if (!av_channel_layout_compare(&input, &output)) {
            return {};
        }

        int channels = input.nb_channels;
        std::vector<float> matrix(channels * output.nb_channels, 0);
        if (input.order == AV_CHANNEL_ORDER_UNSPEC ||
            output.order == AV_CHANNEL_ORDER_UNSPEC) {
            for (int i = 0; i < std::min(channels, output.nb_channels); ++i) {
                matrix[i * channels + i] = 1;
            }
            return matrix;
        }

        constexpr float sqrtHalf = 0.7071067811865475f;
        for (int i = 0; i < channels; ++i) {
            auto route = [&](AVChannel speaker, float gain) {
                int target =
                    av_channel_layout_index_from_channel(&output, speaker);
                if (target < 0) {
                    return false;
                }
                matrix[target * channels + i] = gain;
                return true;
            };

            AVChannel speaker = av_channel_layout_channel_from_index(&input, i);
            if (route(speaker, 1)) {
                continue;
            }
            switch (speaker) {
            case AV_CHAN_FRONT_CENTER:
                route(AV_CHAN_FRONT_LEFT, sqrtHalf);
                route(AV_CHAN_FRONT_RIGHT, sqrtHalf);
                break;
            case AV_CHAN_FRONT_LEFT:
            case AV_CHAN_FRONT_RIGHT:
                route(AV_CHAN_FRONT_CENTER, sqrtHalf);
                break;
            case AV_CHAN_BACK_LEFT:
            case AV_CHAN_SIDE_LEFT:
                if (!route(AV_CHAN_BACK_LEFT, 1) &&
                    !route(AV_CHAN_SIDE_LEFT, 1) &&
                    !route(AV_CHAN_FRONT_LEFT, sqrtHalf)) {
                    route(AV_CHAN_FRONT_CENTER, 0.5f);
                }
                break;
            case AV_CHAN_BACK_RIGHT:
            case AV_CHAN_SIDE_RIGHT:
                if (!route(AV_CHAN_BACK_RIGHT, 1) &&
                    !route(AV_CHAN_SIDE_RIGHT, 1) &&
                    !route(AV_CHAN_FRONT_RIGHT, sqrtHalf)) {
                    route(AV_CHAN_FRONT_CENTER, 0.5f);
                }
                break;
            case AV_CHAN_BACK_CENTER:
                if (route(AV_CHAN_BACK_LEFT, sqrtHalf)) {
                    route(AV_CHAN_BACK_RIGHT, sqrtHalf);
                } else if (route(AV_CHAN_SIDE_LEFT, sqrtHalf)) {
                    route(AV_CHAN_SIDE_RIGHT, sqrtHalf);
                } else if (route(AV_CHAN_FRONT_LEFT, 0.5f)) {
                    route(AV_CHAN_FRONT_RIGHT, 0.5f);
                } else {
                    route(AV_CHAN_FRONT_CENTER, sqrtHalf);
                }
                break;
            default:
                // Unmatched LFE and other uncommon speakers are silent.
                break;
            }
        }
        return matrix;
    }

    template <typename Sample>
    void unpackPCM(const uint8_t* source, size_t step, size_t frames,
                   float* output, double gain, float offset)
    {
        for (size_t i = 0; i < frames; ++i) {
            Sample sample;
            memcpy(&sample, source + i * step, sizeof(sample));
            output[i] = static_cast<float>(sample * gain) + offset;
        }
    }
#endif

    struct BufferInput {
        BufferInput(const uint8_t* data, size_t size)
            : bytes(data)
            , length(size)
        {
        }

        const uint8_t* bytes;
        size_t length;
    };

    class CompressedDecoder {
    public:
        CompressedDecoder(const uint8_t* bytes, size_t length, int targetRate)
            : m_input{ bytes, length }
            , m_targetRate(targetRate)
        {
        }

        ~CompressedDecoder()
        {
            releaseDecoder();
            av_channel_layout_uninit(&m_inputLayout);
            av_channel_layout_uninit(&m_outputLayout);
        }

        AudioBufferData* decode()
        {
            if (!open()) {
                return nullptr;
            }
            int status = 0;
            while ((status = readPacket()) >= 0) {
                if (m_packet->stream_index == m_streamIndex) {
                    int submitted = avcodec_send_packet(m_codec, m_packet);
                    av_packet_unref(m_packet);
                    if (submitted < 0 || !receiveFrames()) {
                        return nullptr;
                    }
                } else {
                    av_packet_unref(m_packet);
                }
            }
            if (status != AVERROR_EOF ||
                avcodec_send_packet(m_codec, nullptr) < 0 || !receiveFrames() ||
                (m_limitSource && m_sourceRemaining) || !flushResampler()) {
                return nullptr;
            }
            releaseDecoder();
            return m_output->finish();
        }

    private:
        void releaseDecoder()
        {
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
            m_resampler.reset();
            std::vector<float>().swap(m_mixBuffer);
#else
            swr_free(&m_resampler);
            std::vector<float>().swap(m_conversionBuffer);
#endif
            av_frame_free(&m_frame);
            av_packet_free(&m_packet);
            avcodec_free_context(&m_codec);
            av_vorbis_parse_free(&m_vorbisGranules);
            av_parser_close(m_parser);
            m_parser = nullptr;
        }

        bool open()
        {
            m_nativeMP3 = m_mp3.open(m_input.bytes, m_input.length);
            if (m_nativeMP3) {
                const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_MP3);
                if (!decoder) {
                    return false;
                }
                m_codec = avcodec_alloc_context3(decoder);
                if (!m_codec) {
                    return false;
                }
                m_codec->pkt_timebase = { 1, static_cast<int>(
                                                 m_mp3.sampleRate()) };
                if (avcodec_open2(m_codec, decoder, nullptr) < 0) {
                    return false;
                }
                m_streamIndex = 0;
                m_packet = av_packet_alloc();
                m_frame = av_frame_alloc();
                return m_packet && m_frame;
            }
            m_nativeContainer = m_container.open(m_input.bytes, m_input.length);
            if (m_nativeContainer) {
                return openNativeContainer();
            }
            return false;
        }

        int readPacket()
        {
            if (m_nativeContainer) {
                return readContainerPacket();
            }
            if (!m_nativeMP3) {
                return AVERROR_INVALIDDATA;
            }
            MP3MemoryDemuxer::Packet packet;
            if (!m_mp3.next(packet)) {
                return AVERROR_EOF;
            }
            // libavcodec requires zero padding beyond each packet. Copy only
            // the current frame, never the complete resource.
            int status = av_new_packet(m_packet, static_cast<int>(packet.size));
            if (status < 0) {
                return status;
            }
            memcpy(m_packet->data, packet.data, packet.size);
            m_packet->stream_index = 0;
            m_packet->pts = m_packet->dts = packet.sampleOffset;
            m_packet->duration = packet.samples;
            if (packet.skip || packet.discard) {
                uint8_t* side = av_packet_new_side_data(
                    m_packet, AV_PKT_DATA_SKIP_SAMPLES, 10);
                if (!side) {
                    return AVERROR(ENOMEM);
                }
                for (unsigned i = 0; i < 4; i++) {
                    side[i] = static_cast<uint8_t>(packet.skip >> (i * 8));
                    side[i + 4] =
                        static_cast<uint8_t>(packet.discard >> (i * 8));
                }
                side[8] = side[9] = 0;
            }
            return 0;
        }

        bool openNativeContainer()
        {
            const auto& config = m_container.configuration();
            if (config.rate > 768000 || config.channels > MaxChannels) {
                return false;
            }
            using Codec = AudioContainerReader::Codec;
            AVCodecID id = AV_CODEC_ID_NONE;
            switch (config.codec) {
            case Codec::AAC:
                id = AV_CODEC_ID_AAC;
                break;
            case Codec::ALAC:
                id = AV_CODEC_ID_ALAC;
                break;
            case Codec::MP3:
                id = AV_CODEC_ID_MP3;
                break;
            case Codec::FLAC:
                id = AV_CODEC_ID_FLAC;
                break;
            case Codec::Vorbis:
                id = AV_CODEC_ID_VORBIS;
                break;
            case Codec::Opus:
                id = AV_CODEC_ID_OPUS;
                break;
            case Codec::ALaw:
                id = AV_CODEC_ID_PCM_ALAW;
                break;
            case Codec::MuLaw:
                id = AV_CODEC_ID_PCM_MULAW;
                break;
            case Codec::IMAADPCM:
                id = AV_CODEC_ID_ADPCM_IMA_WAV;
                break;
            case Codec::MSADPCM:
                id = AV_CODEC_ID_ADPCM_MS;
                break;
            case Codec::PCM:
                if (config.floating) {
                    id = config.bits == 32   ? AV_CODEC_ID_PCM_F32LE
                         : config.bits == 64 ? AV_CODEC_ID_PCM_F64LE
                                             : AV_CODEC_ID_NONE;
                } else {
                    switch (config.bits) {
                    case 8:
                        id = AV_CODEC_ID_PCM_U8;
                        break;
                    case 16:
                        id = AV_CODEC_ID_PCM_S16LE;
                        break;
                    case 24:
                        id = AV_CODEC_ID_PCM_S24LE;
                        break;
                    case 32:
                        id = AV_CODEC_ID_PCM_S32LE;
                        break;
                    }
                }
                break;
            default:
                break;
            }
            if (config.codec == Codec::PCM && config.bigEndian) {
                switch (id) {
                case AV_CODEC_ID_PCM_S16LE:
                    id = AV_CODEC_ID_PCM_S16BE;
                    break;
                case AV_CODEC_ID_PCM_S24LE:
                    id = AV_CODEC_ID_PCM_S24BE;
                    break;
                case AV_CODEC_ID_PCM_S32LE:
                    id = AV_CODEC_ID_PCM_S32BE;
                    break;
                case AV_CODEC_ID_PCM_F32LE:
                    id = AV_CODEC_ID_PCM_F32BE;
                    break;
                case AV_CODEC_ID_PCM_F64LE:
                    id = AV_CODEC_ID_PCM_F64BE;
                    break;
                default:
                    break;
                }
            }
            if (id == AV_CODEC_ID_PCM_U8 && config.signed8) {
                id = AV_CODEC_ID_PCM_S8;
            }
            const AVCodec* decoder = avcodec_find_decoder(id);
            if (!decoder) {
                return false;
            }
            m_codec = avcodec_alloc_context3(decoder);
            if (!m_codec) {
                return false;
            }
            m_codec->sample_rate = config.rate;
            if (config.channels) {
                av_channel_layout_default(&m_codec->ch_layout, config.channels);
            }
            m_codec->bits_per_coded_sample = config.bits;
            m_codec->block_align = config.blockAlign;
            if (!config.extra.empty()) {
                m_codec->extradata = static_cast<uint8_t*>(av_mallocz(
                    config.extra.size() + AV_INPUT_BUFFER_PADDING_SIZE));
                if (!m_codec->extradata) {
                    return false;
                }
                memcpy(m_codec->extradata, config.extra.data(),
                       config.extra.size());
                m_codec->extradata_size = config.extra.size();
                if (config.codec == Codec::Opus && config.extra.size() >= 12) {
                    // Container CodecDelay/editing can override OpusHead.
                    // Apply that delay ourselves instead of skipping twice.
                    m_codec->extradata[10] = m_codec->extradata[11] = 0;
                }
            }
            if (avcodec_open2(m_codec, decoder, nullptr) < 0) {
                return false;
            }
            if (config.parseFrames) {
                m_parser = av_parser_init(id);
                if (!m_parser) {
                    return false;
                }
            }
            m_startSkip = config.skip;
            m_skipSourceRate = config.rate;
            m_skipNeedsRate = true;
            av_vorbis_parse_free(&m_vorbisGranules);
            if (config.codec == Codec::Vorbis && config.oggGranules) {
                m_vorbisGranules = av_vorbis_parse_init(config.extra.data(),
                                                        config.extra.size());
                if (!m_vorbisGranules) {
                    return false;
                }
            }
            m_vorbisPageSamples = 0;
            m_vorbisFirstPacket = true;
            m_sourceRemaining = config.samples;
            m_limitSource = config.samples != 0;
            m_streamIndex = 0;
            if (!m_packet) {
                m_packet = av_packet_alloc();
            }
            if (!m_frame) {
                m_frame = av_frame_alloc();
            }
            return m_packet && m_frame;
        }

        int copyPacket(const uint8_t* data, size_t size)
        {
            // Bound compressed-packet copies independently of the file size.
            if (!size || size > 16 * 1024 * 1024) {
                return AVERROR_INVALIDDATA;
            }
            int status = av_new_packet(m_packet, static_cast<int>(size));
            if (status < 0) {
                return status;
            }
            memcpy(m_packet->data, data, size);
            m_packet->stream_index = 0;
            return 0;
        }

        int readContainerPacket()
        {
            AudioContainerReader::Packet packet;
            if (!m_parser) {
                if (!m_container.next(packet)) {
                    return m_container.failed() ? AVERROR_INVALIDDATA
                                                : AVERROR_EOF;
                }
                if (packet.reconfigure) {
                    if (avcodec_send_packet(m_codec, nullptr) < 0 ||
                        !receiveFrames()) {
                        return AVERROR_INVALIDDATA;
                    }
                    avcodec_free_context(&m_codec);
                    if (!openNativeContainer()) {
                        return AVERROR_INVALIDDATA;
                    }
                }
                if (packet.granuleOrigin) {
                    if (packet.granuleOrigin > m_sourceRemaining) {
                        return AVERROR_INVALIDDATA;
                    }
                    m_sourceRemaining -= packet.granuleOrigin;
                }
                if (m_vorbisGranules) {
                    int duration = av_vorbis_parse_frame(
                        m_vorbisGranules, packet.data, packet.size);
                    if (duration < 0) {
                        return AVERROR_INVALIDDATA;
                    }
                    // The initial packet establishes overlap and emits no
                    // samples. Granules count PCM, not its nominal duration.
                    if (!m_vorbisFirstPacket) {
                        m_vorbisPageSamples += duration;
                    }
                    m_vorbisFirstPacket = false;
                    if (packet.granule >= 0) {
                        // Vorbis permits a negative initial PCM position.
                        // Infer that origin from a non-final first audio page;
                        // an EOS-only page instead uses its granule for trim.
                        // https://xiph.org/vorbis/doc/framing.html
                        if (!packet.endOfStream ||
                            uint64_t(packet.granule) > m_vorbisPageSamples) {
                            int64_t origin =
                                packet.granule -
                                static_cast<int64_t>(m_vorbisPageSamples);
                            if (origin > 0 &&
                                uint64_t(origin) > m_sourceRemaining) {
                                return AVERROR_INVALIDDATA;
                            }
                            if (origin >= 0) {
                                m_sourceRemaining -= origin;
                            } else {
                                m_sourceRemaining +=
                                    static_cast<uint64_t>(-origin);
                            }
                        }
                        av_vorbis_parse_free(&m_vorbisGranules);
                    }
                }
                int status = copyPacket(packet.data, packet.size);
                if (status >= 0 && packet.discard) {
                    uint8_t* side = av_packet_new_side_data(
                        m_packet, AV_PKT_DATA_SKIP_SAMPLES, 10);
                    if (!side) {
                        return AVERROR(ENOMEM);
                    }
                    memset(side, 0, 10);
                    for (unsigned i = 0; i < 4; i++) {
                        side[4 + i] =
                            static_cast<uint8_t>(packet.discard >> (i * 8));
                    }
                }
                return status;
            }
            for (;;) {
                if (m_parserOffset == m_parserLength && !m_parserEOF) {
                    if (m_container.next(packet)) {
                        m_parserBuffer.assign(packet.data,
                                              packet.data + packet.size);
                        m_parserLength = packet.size;
                        m_parserBuffer.resize(
                            packet.size + AV_INPUT_BUFFER_PADDING_SIZE, 0);
                    } else {
                        if (m_container.failed()) {
                            return AVERROR_INVALIDDATA;
                        }
                        m_parserEOF = true;
                        m_parserLength = 0;
                    }
                    m_parserOffset = 0;
                }
                if (m_parserFlushed) {
                    return AVERROR_EOF;
                }
                uint8_t* output = nullptr;
                int size = 0;
                int consumed = av_parser_parse2(
                    m_parser, m_codec, &output, &size,
                    m_parserEOF ? nullptr
                                : m_parserBuffer.data() + m_parserOffset,
                    m_parserLength - m_parserOffset, AV_NOPTS_VALUE,
                    AV_NOPTS_VALUE, 0);
                if (consumed < 0 || (!consumed && !size && !m_parserEOF)) {
                    return AVERROR_INVALIDDATA;
                }
                m_parserOffset += consumed;
                m_parserPending += consumed;
                if (m_parserPending > 16 * 1024 * 1024) {
                    return AVERROR_INVALIDDATA;
                }
                m_parserFlushed = m_parserEOF && !size;
                if (size) {
                    m_parserPending = 0;
                    return copyPacket(output, size);
                }
            }
        }

        bool configureResampler()
        {
            int channels = m_frame->ch_layout.nb_channels;
            if (channels <= 0 || channels > MaxChannels ||
                m_frame->sample_rate <= 0 || m_frame->format < 0) {
                return false;
            }
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
            if (m_frame->sample_rate > 768000) {
                return false;
            }
#endif
            AVChannelLayout layout{};
            if (av_channel_layout_check(&m_frame->ch_layout)) {
                if (av_channel_layout_copy(&layout, &m_frame->ch_layout) < 0) {
                    return false;
                }
            } else {
                av_channel_layout_default(&layout, channels);
            }
            if (m_output && m_inputFormat == m_frame->format &&
                m_inputRate == m_frame->sample_rate &&
                !av_channel_layout_compare(&m_inputLayout, &layout)) {
                av_channel_layout_uninit(&layout);
                return true;
            }
            if (m_output) {
                // Sample format, rate or layout may change mid-stream (e.g.
                // chained Ogg). Drain and rebuild the resampler so it never
                // reads planes with a stale layout; the output keeps the first
                // frame's layout.
                if (!flushResampler()) {
                    av_channel_layout_uninit(&layout);
                    return false;
                }
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
                m_resampler.reset();
#else
                swr_free(&m_resampler);
#endif
            } else {
                if (av_channel_layout_copy(&m_outputLayout, &layout) < 0) {
                    av_channel_layout_uninit(&layout);
                    return false;
                }
                m_channels = static_cast<size_t>(channels);
                m_output.reset(
                    new AudioBufferDataBuilder(m_channels, estimatedFrames()));
            }
            av_channel_layout_uninit(&m_inputLayout);
            m_inputLayout = layout;
            m_inputFormat = m_frame->format;
            m_inputRate = m_frame->sample_rate;
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
            m_channelMix = channelMix(m_inputLayout, m_outputLayout);
            if (m_inputRate != m_targetRate) {
                m_resampler.reset(new AudioPCMResampler(
                    m_inputRate, m_targetRate, m_channels));
            }
            return true;
#else
            int status = swr_alloc_set_opts2(
                &m_resampler, &m_outputLayout, AV_SAMPLE_FMT_FLTP, m_targetRate,
                &m_inputLayout, static_cast<AVSampleFormat>(m_inputFormat),
                m_inputRate, 0, nullptr);
            return status >= 0 && swr_init(m_resampler) >= 0;
#endif
        }

        // The container duration sizes the first allocation. It may be
        // estimated from the bitrate; growth and the final trim absorb the
        // error.
        size_t estimatedFrames() const
        {
            if (m_nativeContainer) {
                const auto& config = m_container.configuration();
                double count = config.rate
                                   ? static_cast<double>(config.samples) *
                                         m_targetRate / config.rate
                                   : 0;
                return count < AudioBufferData::MaxSamples
                           ? static_cast<size_t>(count)
                           : 0;
            }
            if (m_nativeMP3) {
                double frames = static_cast<double>(m_mp3.samples()) *
                                m_targetRate / m_mp3.sampleRate();
                return frames < AudioBufferData::MaxSamples
                           ? static_cast<size_t>(frames)
                           : 0;
            }
            return 0;
        }

#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
        template <typename Sample>
        void convertPCM(const uint8_t* const* input, size_t frames,
                        float* const* output, bool planar, double gain,
                        float offset = 0)
        {
            size_t channels = m_inputLayout.nb_channels;
            size_t step = sizeof(Sample) * (planar ? 1 : channels);
            if (m_channelMix.empty()) {
                for (size_t channel = 0; channel < channels; ++channel) {
                    auto* source = planar ? input[channel]
                                          : input[0] + channel * sizeof(Sample);
                    unpackPCM<Sample>(source, step, frames, output[channel],
                                      gain, offset);
                }
                return;
            }

            for (size_t channel = 0; channel < m_channels; ++channel) {
                std::fill_n(output[channel], frames, 0);
            }
            m_mixBuffer.resize(frames);
            for (size_t channel = 0; channel < channels; ++channel) {
                auto* source = planar ? input[channel]
                                      : input[0] + channel * sizeof(Sample);
                unpackPCM<Sample>(source, step, frames, m_mixBuffer.data(),
                                  gain, offset);
                for (size_t target = 0; target < m_channels; ++target) {
                    float weight = m_channelMix[target * channels + channel];
                    if (weight) {
                        for (size_t i = 0; i < frames; ++i) {
                            output[target][i] += m_mixBuffer[i] * weight;
                        }
                    }
                }
            }
        }

        bool convertSamples(AVSampleFormat format, const uint8_t* const* input,
                            size_t frames, bool planar)
        {
            float* output[MaxChannels];
            if (m_resampler) {
                m_resampler->prepareInput(frames, output);
            } else {
                if (!m_output->reserve(frames)) {
                    return false;
                }
                for (size_t channel = 0; channel < m_channels; ++channel) {
                    output[channel] = m_output->end(channel);
                }
            }
            switch (av_get_packed_sample_fmt(format)) {
            case AV_SAMPLE_FMT_U8:
                convertPCM<uint8_t>(input, frames, output, planar, 1.0 / 128,
                                    -1);
                break;
            case AV_SAMPLE_FMT_S16:
                convertPCM<int16_t>(input, frames, output, planar, 1.0 / 32768);
                break;
            case AV_SAMPLE_FMT_S32:
                convertPCM<int32_t>(input, frames, output, planar,
                                    1.0 / (uint64_t(1) << 31));
                break;
            case AV_SAMPLE_FMT_S64:
                convertPCM<int64_t>(input, frames, output, planar,
                                    1.0 / (uint64_t(1) << 63));
                break;
            case AV_SAMPLE_FMT_FLT:
                convertPCM<float>(input, frames, output, planar, 1);
                break;
            case AV_SAMPLE_FMT_DBL:
                convertPCM<double>(input, frames, output, planar, 1);
                break;
            default:
                return false;
            }
            if (m_resampler) {
                return m_resampler->append(frames, *m_output);
            }
            m_output->commit(frames);
            return true;
        }
#else
        bool appendFrames(const float* planar, size_t stride, size_t frames)
        {
            // Short packets can be fully buffered by the resampling filter.
            if (!frames) {
                return true;
            }
            if (!m_output->reserve(frames)) {
                return false;
            }
            for (size_t channel = 0; channel < m_channels; channel++) {
                memcpy(m_output->end(channel), planar + channel * stride,
                       frames * sizeof(float));
            }
            m_output->commit(frames);
            return true;
        }

        bool appendConverted(int capacity, const uint8_t** input,
                             int inputFrames)
        {
            if (capacity < 0 || static_cast<size_t>(capacity) >
                                    AudioBufferData::MaxSamples / m_channels) {
                return false;
            }
            if (!capacity) {
                return swr_convert(m_resampler, nullptr, 0, input,
                                   inputFrames) >= 0;
            }
            size_t stride = static_cast<size_t>(capacity);
            if (m_conversionBuffer.size() < m_channels * stride) {
                m_conversionBuffer.resize(m_channels * stride);
            }
            uint8_t* outputs[MaxChannels];
            for (size_t channel = 0; channel < m_channels; channel++) {
                outputs[channel] = reinterpret_cast<uint8_t*>(
                    m_conversionBuffer.data() + channel * stride);
            }
            int frames =
                swr_convert(m_resampler, outputs, capacity, input, inputFrames);
            return frames >= 0 &&
                   appendFrames(m_conversionBuffer.data(), stride,
                                static_cast<size_t>(frames));
        }
#endif

        bool convertFrame()
        {
            if (m_skipNeedsRate) {
                // Codec output can use an extension rate (e.g. HE-AAC SBR)
                // different from the container's core sampling frequency.
                if (m_skipSourceRate && m_startSkip) {
                    m_startSkip = av_rescale(m_startSkip, m_frame->sample_rate,
                                             m_skipSourceRate);
                }
                m_skipNeedsRate = false;
            }
            const auto& config = m_container.configuration();
            if (m_nativeContainer && !config.edits.empty()) {
                uint64_t first = m_editPosition;
                uint64_t last = first + m_frame->nb_samples;
                while (m_editIndex < config.edits.size()) {
                    const auto& edit = config.edits[m_editIndex];
                    if (edit.start > INT64_MAX || edit.length > INT64_MAX) {
                        return false;
                    }
                    uint64_t start = av_rescale(
                        edit.start, m_frame->sample_rate, config.rate);
                    uint64_t end =
                        start + av_rescale(edit.length, m_frame->sample_rate,
                                           config.rate);
                    start = std::max<uint64_t>(start, config.skip);
                    if (start >= last) {
                        break;
                    }
                    uint64_t begin = std::max(first, start);
                    uint64_t stop = std::min(last, end);
                    if (begin < stop &&
                        !convertFrameRange(begin - first, stop - first)) {
                        return false;
                    }
                    if (end > last) {
                        break;
                    }
                    m_editIndex++;
                }
                m_editPosition += m_frame->nb_samples;
                return true;
            }
            int start = static_cast<int>(
                std::min<uint64_t>(m_startSkip, m_frame->nb_samples));
            m_startSkip -= start;
            int end = m_frame->nb_samples;
            if (m_limitSource) {
                end = start + static_cast<int>(std::min<uint64_t>(
                                  m_sourceRemaining, end - start));
                m_sourceRemaining -= end - start;
            }
            return convertFrameRange(start, end);
        }

        bool convertFrameRange(int start, int end)
        {
            auto format = static_cast<AVSampleFormat>(m_frame->format);
            size_t channels =
                static_cast<size_t>(m_frame->ch_layout.nb_channels);
            bool planar = av_sample_fmt_is_planar(format);
            int bytes = av_get_bytes_per_sample(format);
            if (bytes <= 0) {
                return false;
            }
            size_t step = static_cast<size_t>(bytes) * (planar ? 1 : channels);
            size_t planes = planar ? channels : 1;
            int64_t slice = std::max<int64_t>(
                1, av_rescale(ConversionFrames, m_inputRate, m_targetRate));
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
            slice = std::min(slice, ConversionFrames);
#endif
            for (int offset = start; offset < end;) {
                int count =
                    static_cast<int>(std::min<int64_t>(slice, end - offset));
                const uint8_t* input[MaxChannels] = {};
                for (size_t plane = 0; plane < planes; plane++) {
                    input[plane] = m_frame->extended_data[plane] +
                                   static_cast<size_t>(offset) * step;
                }
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
                if (!convertSamples(format, input, count, planar)) {
                    return false;
                }
#else
                if (!appendConverted(swr_get_out_samples(m_resampler, count),
                                     input, count)) {
                    return false;
                }
#endif
                offset += count;
            }
            return true;
        }

        bool receiveFrames()
        {
            for (;;) {
                int status = avcodec_receive_frame(m_codec, m_frame);
                if (status == AVERROR(EAGAIN) || status == AVERROR_EOF) {
                    return true;
                }
                if (status < 0 || !configureResampler() || !convertFrame()) {
                    return false;
                }
                av_frame_unref(m_frame);
            }
        }

        bool flushResampler()
        {
            if (!m_output) {
                return false;
            }
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
            return !m_resampler || m_resampler->drain(*m_output);
#else
            int capacity = swr_get_out_samples(m_resampler, 0);
            return appendConverted(capacity, nullptr, 0);
#endif
        }

        BufferInput m_input;
        MP3MemoryDemuxer m_mp3;
        bool m_nativeMP3{ false };
        AudioContainerReader m_container;
        bool m_nativeContainer{ false };
        AVCodecParserContext* m_parser{ nullptr };
        std::vector<uint8_t> m_parserBuffer;
        size_t m_parserLength{ 0 };
        size_t m_parserOffset{ 0 };
        size_t m_parserPending{ 0 };
        bool m_parserEOF{ false };
        bool m_parserFlushed{ false };
        uint64_t m_startSkip{ 0 };
        uint32_t m_skipSourceRate{ 0 };
        bool m_skipNeedsRate{ false };
        uint64_t m_sourceRemaining{ 0 };
        bool m_limitSource{ false };
        AVVorbisParseContext* m_vorbisGranules{ nullptr };
        uint64_t m_vorbisPageSamples{ 0 };
        bool m_vorbisFirstPacket{ true };
        uint64_t m_editPosition{ 0 };
        size_t m_editIndex{ 0 };
        int m_targetRate;
        int m_streamIndex{ -1 };
        AVCodecContext* m_codec{ nullptr };
        AVPacket* m_packet{ nullptr };
        AVFrame* m_frame{ nullptr };
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
        std::unique_ptr<AudioPCMResampler> m_resampler;
        std::vector<float> m_channelMix;
        std::vector<float> m_mixBuffer;
#else
        SwrContext* m_resampler{ nullptr };
        std::vector<float> m_conversionBuffer;
#endif
        int m_inputFormat{ -1 };
        int m_inputRate{ 0 };
        AVChannelLayout m_inputLayout{};
        AVChannelLayout m_outputLayout{};
        size_t m_channels{ 0 };
        std::unique_ptr<AudioBufferDataBuilder> m_output;
    };

} // namespace
#endif

AudioBufferData* decodeCompressedAudio(const uint8_t* bytes, size_t length,
                                       double targetSampleRate)
{
#if defined(STARFISH_FFMPEG_AUDIO_DECODER)
    if (!bytes || !length ||
        length > static_cast<size_t>(std::numeric_limits<int64_t>::max()) ||
        !std::isfinite(targetSampleRate) || targetSampleRate < 3000 ||
        targetSampleRate > 768000) {
        return nullptr;
    }
    // decodeAudioData consumes complete files, not streaming WAV headers.
    // Do not let FFmpeg's truncated-file recovery bypass RIFF bounds checks
    // shared by Web Audio and media-element decoding. Valid compressed WAVE
    // codecs still reach FFmpeg.
    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-decodeaudiodata
    if (isMalformedWaveAudio(bytes, length)) {
        return nullptr;
    }
    try {
        CompressedDecoder decoder(
            bytes, length, static_cast<int>(std::round(targetSampleRate)));
        return decoder.decode();
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
#else
    return nullptr;
#endif
}

} // namespace Starfish
#endif
