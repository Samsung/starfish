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

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <vector>

#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libswresample/swresample.h>
}
#endif

namespace Starfish {

#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
namespace {

    // Decoded PCM is held twice while the AudioBufferData is built (decode
    // chunks plus output), so bound it well below the AudioBuffer limit.
    constexpr size_t MaxDecodedSamples = 32 * 1024 * 1024 / sizeof(float);
    // Frames per channel in one decode chunk. Fixed-size chunks avoid the
    // transient doubling of geometric vector growth.
    constexpr size_t ChunkFrames = 8192;
    // Output frames per resampler call, bounding the conversion buffer even
    // for extreme sample-rate ratios.
    constexpr int64_t ConversionFrames = 4096;
    constexpr int MaxChannels = 32;
    constexpr int MaxStreams = 16;

    // decodeAudioData is only required to support audio codecs; keep the
    // attack surface to the decoders behind the documented containers.
    // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-decodeaudiodata
    const char AudioCodecWhitelist[] =
        "aac,aac_fixed,mp3float,mp3,flac,vorbis,libvorbis,opus,libopus,"
        "pcm_alaw,pcm_mulaw,pcm_s8,pcm_u8,pcm_s16le,pcm_s16be,pcm_s24le,"
        "pcm_s24be,pcm_s32le,pcm_s32be,pcm_f32le,pcm_f32be,pcm_f64le,"
        "pcm_f64be,adpcm_ms,adpcm_ima_wav";

    struct BufferInput {
        BufferInput(const uint8_t* data, size_t size)
            : bytes(data)
            , length(size)
        {
        }

        const uint8_t* bytes;
        size_t length;
        size_t offset{ 0 };
    };

    int readPacket(void* opaque, uint8_t* output, int requested)
    {
        auto* input = static_cast<BufferInput*>(opaque);
        if (input->offset == input->length) {
            return AVERROR_EOF;
        }
        size_t count = std::min(static_cast<size_t>(requested),
                                input->length - input->offset);
        memcpy(output, input->bytes + input->offset, count);
        input->offset += count;
        return static_cast<int>(count);
    }

    int64_t seekPacket(void* opaque, int64_t offset, int whence)
    {
        auto* input = static_cast<BufferInput*>(opaque);
        if (whence == AVSEEK_SIZE) {
            return static_cast<int64_t>(input->length);
        }
        whence &= ~AVSEEK_FORCE;
        int64_t base = 0;
        if (whence == SEEK_CUR) {
            base = static_cast<int64_t>(input->offset);
        } else if (whence == SEEK_END) {
            base = static_cast<int64_t>(input->length);
        } else if (whence != SEEK_SET) {
            return AVERROR(EINVAL);
        }
        if ((offset > 0 && base > INT64_MAX - offset) ||
            (offset < 0 && base < INT64_MIN - offset)) {
            return AVERROR(EINVAL);
        }
        int64_t position = base + offset;
        if (position < 0 || static_cast<uint64_t>(position) > input->length) {
            return AVERROR(EINVAL);
        }
        input->offset = static_cast<size_t>(position);
        return position;
    }

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
            while ((status = av_read_frame(m_format, m_packet)) >= 0) {
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
                !flushResampler() || !m_frames) {
                return nullptr;
            }
            releaseDecoder();

            AudioBufferData* data =
                AudioBufferData::create(m_channels, m_frames);
            if (!data) {
                return nullptr;
            }
            size_t copied = 0;
            for (auto& chunk : m_chunks) {
                size_t count = std::min(ChunkFrames, m_frames - copied);
                for (size_t channel = 0; channel < m_channels; channel++) {
                    memcpy(data->channel(channel) + copied,
                           chunk.get() + channel * ChunkFrames,
                           count * sizeof(float));
                }
                copied += count;
                chunk.reset();
            }
            return data;
        }

    private:
        void releaseDecoder()
        {
            swr_free(&m_resampler);
            av_frame_free(&m_frame);
            av_packet_free(&m_packet);
            avcodec_free_context(&m_codec);
            avformat_close_input(&m_format);
            // libavformat may replace the caller-owned AVIO buffer.
            if (m_io) {
                av_freep(&m_io->buffer);
            }
            avio_context_free(&m_io);
            std::vector<float>().swap(m_conversionBuffer);
        }

        bool open()
        {
            m_format = avformat_alloc_context();
            uint8_t* ioBuffer = static_cast<uint8_t*>(av_malloc(4096));
            if (!m_format || !ioBuffer) {
                av_free(ioBuffer);
                return false;
            }
            m_io = avio_alloc_context(ioBuffer, 4096, 0, &m_input, readPacket,
                                      nullptr, seekPacket);
            if (!m_io) {
                av_free(ioBuffer);
                return false;
            }
            m_format->pb = m_io;
            m_format->flags |= AVFMT_FLAG_CUSTOM_IO;
            // decodeAudioData receives a complete resource, not permission to
            // open files or URLs referenced by an attacker-controlled demuxer.
            // https://webaudio.github.io/web-audio-api/#dom-baseaudiocontext-decodeaudiodata
            m_format->io_open = [](AVFormatContext*, AVIOContext**, const char*,
                                   int,
                                   AVDictionary**) { return AVERROR(EACCES); };
            m_format->format_whitelist =
                av_strdup("aac,flac,matroska,webm,mov,mp3,ogg,wav");
            m_format->protocol_whitelist = av_strdup("");
            // Stream probing opens decoders too; restrict it to audio codecs.
            m_format->codec_whitelist = av_strdup(AudioCodecWhitelist);
            m_format->max_streams = MaxStreams;
            if (!m_format->format_whitelist || !m_format->protocol_whitelist ||
                !m_format->codec_whitelist) {
                return false;
            }
            if (avformat_open_input(&m_format, nullptr, nullptr, nullptr) < 0) {
                return false;
            }
            // Keep video, subtitle and data streams out of probing and
            // demuxing; only audio can contribute to the decoded buffer.
            for (unsigned i = 0; i < m_format->nb_streams; i++) {
                AVStream* stream = m_format->streams[i];
                if (stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
                    stream->discard = AVDISCARD_ALL;
                }
            }
            if (avformat_find_stream_info(m_format, nullptr) < 0) {
                return false;
            }
            m_streamIndex = av_find_best_stream(m_format, AVMEDIA_TYPE_AUDIO,
                                                -1, -1, nullptr, 0);
            if (m_streamIndex < 0) {
                return false;
            }
            for (unsigned i = 0; i < m_format->nb_streams; i++) {
                if (static_cast<int>(i) != m_streamIndex) {
                    m_format->streams[i]->discard = AVDISCARD_ALL;
                }
            }
            AVCodecParameters* parameters =
                m_format->streams[m_streamIndex]->codecpar;
            const AVCodec* decoder = avcodec_find_decoder(parameters->codec_id);
            if (!decoder || decoder->type != AVMEDIA_TYPE_AUDIO) {
                return false;
            }
            m_codec = avcodec_alloc_context3(decoder);
            if (!m_codec) {
                return false;
            }
            m_codec->codec_whitelist = av_strdup(AudioCodecWhitelist);
            if (!m_codec->codec_whitelist ||
                avcodec_parameters_to_context(m_codec, parameters) < 0 ||
                avcodec_open2(m_codec, decoder, nullptr) < 0) {
                return false;
            }
            m_packet = av_packet_alloc();
            m_frame = av_frame_alloc();
            return m_packet && m_frame;
        }

        bool configureResampler()
        {
            int channels = m_frame->ch_layout.nb_channels;
            if (channels <= 0 || channels > MaxChannels ||
                m_frame->sample_rate <= 0 || m_frame->format < 0) {
                return false;
            }
            AVChannelLayout layout{};
            if (av_channel_layout_check(&m_frame->ch_layout)) {
                if (av_channel_layout_copy(&layout, &m_frame->ch_layout) < 0) {
                    return false;
                }
            } else {
                av_channel_layout_default(&layout, channels);
            }
            if (m_resampler && m_inputFormat == m_frame->format &&
                m_inputRate == m_frame->sample_rate &&
                !av_channel_layout_compare(&m_inputLayout, &layout)) {
                av_channel_layout_uninit(&layout);
                return true;
            }
            if (m_resampler) {
                // Sample format, rate or layout may change mid-stream (e.g.
                // chained Ogg). Drain and rebuild the resampler so it never
                // reads planes with a stale layout; the output keeps the first
                // frame's layout, and swresample remixes into it.
                if (!flushResampler()) {
                    av_channel_layout_uninit(&layout);
                    return false;
                }
                swr_free(&m_resampler);
            } else {
                if (av_channel_layout_copy(&m_outputLayout, &layout) < 0) {
                    av_channel_layout_uninit(&layout);
                    return false;
                }
                m_channels = static_cast<size_t>(channels);
            }
            av_channel_layout_uninit(&m_inputLayout);
            m_inputLayout = layout;
            m_inputFormat = m_frame->format;
            m_inputRate = m_frame->sample_rate;
            int status = swr_alloc_set_opts2(
                &m_resampler, &m_outputLayout, AV_SAMPLE_FMT_FLTP, m_targetRate,
                &m_inputLayout, static_cast<AVSampleFormat>(m_inputFormat),
                m_inputRate, 0, nullptr);
            return status >= 0 && swr_init(m_resampler) >= 0;
        }

        bool appendFrames(const float* planar, size_t stride, size_t frames)
        {
            if (frames > MaxDecodedSamples / m_channels - m_frames) {
                return false;
            }
            for (size_t written = 0; written < frames;) {
                size_t offset = m_frames % ChunkFrames;
                if (!offset) {
                    std::unique_ptr<float[]> chunk(
                        new float[m_channels * ChunkFrames]);
                    m_chunks.push_back(std::move(chunk));
                }
                size_t count = std::min(ChunkFrames - offset, frames - written);
                float* chunk = m_chunks.back().get();
                for (size_t channel = 0; channel < m_channels; channel++) {
                    memcpy(chunk + channel * ChunkFrames + offset,
                           planar + channel * stride + written,
                           count * sizeof(float));
                }
                written += count;
                m_frames += count;
            }
            return true;
        }

        bool appendConverted(int capacity, const uint8_t** input,
                             int inputFrames)
        {
            if (capacity < 0 || static_cast<size_t>(capacity) >
                                    MaxDecodedSamples / m_channels) {
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

        bool convertFrame()
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
            for (int offset = 0; offset < m_frame->nb_samples;) {
                int count = static_cast<int>(
                    std::min<int64_t>(slice, m_frame->nb_samples - offset));
                const uint8_t* input[MaxChannels] = {};
                for (size_t plane = 0; plane < planes; plane++) {
                    input[plane] = m_frame->extended_data[plane] +
                                   static_cast<size_t>(offset) * step;
                }
                if (!appendConverted(swr_get_out_samples(m_resampler, count),
                                     input, count)) {
                    return false;
                }
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
            if (!m_resampler) {
                return false;
            }
            int capacity = swr_get_out_samples(m_resampler, 0);
            return appendConverted(capacity, nullptr, 0);
        }

        BufferInput m_input;
        int m_targetRate;
        int m_streamIndex{ -1 };
        AVFormatContext* m_format{ nullptr };
        AVIOContext* m_io{ nullptr };
        AVCodecContext* m_codec{ nullptr };
        AVPacket* m_packet{ nullptr };
        AVFrame* m_frame{ nullptr };
        SwrContext* m_resampler{ nullptr };
        int m_inputFormat{ -1 };
        int m_inputRate{ 0 };
        AVChannelLayout m_inputLayout{};
        AVChannelLayout m_outputLayout{};
        size_t m_channels{ 0 };
        size_t m_frames{ 0 };
        std::vector<float> m_conversionBuffer;
        std::vector<std::unique_ptr<float[]>> m_chunks;
    };

} // namespace
#endif

AudioBufferData* decodeCompressedAudio(const uint8_t* bytes, size_t length,
                                       double targetSampleRate)
{
#if defined(STARFISH_USE_FFMPEG_MEDIAPLAYER)
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
