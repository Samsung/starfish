/* Copyright (c) 2026 Samsung Electronics Co., Ltd. */
#include "platform/multimedia/FFmpegAudioOutput.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

static bool decodeAndConvert()
{
    const AVCodec* decoder = avcodec_find_decoder(AV_CODEC_ID_PCM_S16LE);
    if (!decoder || !avcodec_find_decoder(AV_CODEC_ID_H264) ||
        !avcodec_find_decoder(AV_CODEC_ID_AAC)) {
        return false;
    }
    AVCodecContext* context = avcodec_alloc_context3(decoder);
    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    AVFormatContext* format = avformat_alloc_context();
    if (!context || !frame || !packet || !format) {
        return false;
    }
    context->sample_rate = 48000;
    av_channel_layout_default(&context->ch_layout, 1);
    int16_t samples[128];
    for (auto& sample : samples) {
        sample = 8192;
    }
    bool passed = avcodec_open2(context, decoder, nullptr) >= 0 &&
                  av_new_packet(packet, sizeof(samples)) >= 0;
    if (passed) {
        std::memcpy(packet->data, samples, sizeof(samples));
        passed = avcodec_send_packet(context, packet) >= 0 &&
                 avcodec_receive_frame(context, frame) >= 0 &&
                 frame->nb_samples == 128;
    }
    SwrContext* resampler = nullptr;
    float converted[128]{};
    if (passed) {
        passed = swr_alloc_set_opts2(
                     &resampler, &frame->ch_layout, AV_SAMPLE_FMT_FLT, 48000,
                     &frame->ch_layout, static_cast<AVSampleFormat>(frame->format),
                     48000, 0, nullptr) >= 0 && swr_init(resampler) >= 0;
    }
    if (passed) {
        uint8_t* planes[] = { reinterpret_cast<uint8_t*>(converted) };
        passed = swr_convert(resampler, planes, 128,
                             const_cast<const uint8_t**>(frame->extended_data),
                             frame->nb_samples) == 128 &&
                 converted[0] == 0.25f && converted[127] == 0.25f;
    }
    swr_free(&resampler);
    avformat_free_context(format);
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&context);
    return passed;
}

static bool convertVideo()
{
    const uint8_t rgb[] = { 255, 0, 0, 0, 255, 0,
                            0, 0, 255, 255, 255, 255 };
    uint8_t rgba[16]{};
    SwsContext* scaler = sws_getContext(2, 2, AV_PIX_FMT_RGB24, 2, 2,
                                        AV_PIX_FMT_RGBA, SWS_POINT, nullptr,
                                        nullptr, nullptr);
    if (!scaler) {
        return false;
    }
    const uint8_t* source[] = { rgb, nullptr, nullptr, nullptr };
    int sourceStride[] = { 6, 0, 0, 0 };
    uint8_t* output[] = { rgba, nullptr, nullptr, nullptr };
    int outputStride[] = { 8, 0, 0, 0 };
    bool passed = sws_scale(scaler, source, sourceStride, 0, 2,
                            output, outputStride) == 2 &&
                  rgba[0] == 255 && rgba[3] == 255 &&
                  rgba[5] == 255 && rgba[10] == 255;
    sws_freeContext(scaler);
    return passed;
}

static bool checkAudio()
{
    Starfish::FFmpegAudioOutput* output = Starfish::FFmpegAudioOutput::create();
    std::atomic<int> state{ 0 };
    bool passed = false;
    std::atomic<bool> interrupted{ false };
    std::thread writer([&]() {
        if (!output->open(1, 48000)) {
            state.store(-1);
            delete output;
            return;
        }
        std::vector<int16_t> silence(48000 * 10, 0);
        state.store(1);
        bool completed = output->write(
            reinterpret_cast<const uint8_t*>(silence.data()),
            silence.size() * sizeof(int16_t));
        passed = !completed;
        while (!interrupted.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        output->flush();
        int16_t brief[480]{};
        passed = passed && output->write(reinterpret_cast<uint8_t*>(brief),
                                         sizeof(brief));
        delete output;
    });
    while (state.load() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (state.load() == 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto start = std::chrono::steady_clock::now();
        output->interrupt();
        interrupted.store(true);
        writer.join();
        passed = passed && std::chrono::steady_clock::now() - start <
                               std::chrono::seconds(2);
        std::printf(passed ? "AUDIO_INTERRUPT_RESET_PASS\n"
                           : "AUDIO_INTERRUPT_RESET_FAIL\n");
    } else {
        writer.join();
        std::printf("AUDIO_NO_ENDPOINT (decoding still tested)\n");
        passed = true;
    }
    return passed;
}

int main(int argc, char** argv)
{
    bool passed = decodeAndConvert() && convertVideo();
    if (argc > 1 && std::strcmp(argv[1], "--audio") == 0) {
        passed = checkAudio() && passed;
    }
    std::printf("FFMPEG_%zuBIT_%s avcodec=%u avformat=%u avutil=%u swscale=%u swresample=%u\n",
                sizeof(void*) * 8, passed ? "PASS" : "FAIL",
                avcodec_version(), avformat_version(), avutil_version(),
                swscale_version(), swresample_version());
    return passed ? 0 : 1;
}
