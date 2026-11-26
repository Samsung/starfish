#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "platform/multimedia/MP3MemoryDemuxer.h"
#include "platform/multimedia/AudioContainerReader.h"

#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
#include "core/modules/webaudio/render/AudioPCMResampler.h"
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

static std::vector<uint8_t> readFile(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
}

#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
static bool testResampling(unsigned inputRate, unsigned outputRate,
                           size_t frames, double frequency,
                           float expectedAmplitude)
{
    constexpr double twoPi = 6.28318530717958647692;
    Starfish::AudioBufferDataBuilder whole(2, 0);
    Starfish::AudioBufferDataBuilder chunked(2, 0);
    Starfish::AudioPCMResampler oneBlock(inputRate, outputRate, 2);
    Starfish::AudioPCMResampler smallBlocks(inputRate, outputRate, 2);

    auto append = [&](Starfish::AudioPCMResampler& resampler,
                      Starfish::AudioBufferDataBuilder& output, size_t offset,
                      size_t count) {
        float* planes[2];
        resampler.prepareInput(count, planes);
        for (size_t i = 0; i < count; ++i) {
            planes[0][i] =
                0.5 * std::sin(twoPi * frequency * (offset + i) / inputRate);
            planes[1][i] = -0.25f;
        }
        return resampler.append(count, output);
    };
    if (!append(oneBlock, whole, 0, frames) || !oneBlock.drain(whole)) {
        return false;
    }
    for (size_t offset = 0; offset < frames;) {
        size_t count = std::min<size_t>(7, frames - offset);
        if (!append(smallBlocks, chunked, offset, count)) {
            return false;
        }
        offset += count;
    }
    if (!smallBlocks.drain(chunked)) {
        return false;
    }

    auto release = [](Starfish::AudioBufferData* buffer) { buffer->release(); };
    std::unique_ptr<Starfish::AudioBufferData, decltype(release)> a(
        whole.finish(), release);
    std::unique_ptr<Starfish::AudioBufferData, decltype(release)> b(
        chunked.finish(), release);
    size_t expected = std::lround(double(frames) * outputRate / inputRate);
    if (!a || !b || a->frames() != expected || b->frames() != expected) {
        return false;
    }
    for (size_t i = 0; i < expected; ++i) {
        float sample = a->channel(0)[i];
        if (!std::isfinite(sample) || sample != b->channel(0)[i] ||
            a->channel(1)[i] != b->channel(1)[i] ||
            std::abs(a->channel(1)[i] + 0.25f) > 0.00001f) {
            return false;
        }
        if (i >= 64 && i + 64 < expected) {
            double ideal = expectedAmplitude *
                           std::sin(twoPi * frequency * i / outputRate);
            if (std::abs(sample - ideal) > 0.001) {
                return false;
            }
        }
    }
    return true;
}
#endif

int main(int argc, char** argv)
{
#if defined(STARFISH_USE_NATIVE_PCM_CONVERTER)
    if (argc == 1) {
        if (!testResampling(44100, 48000, 1001, 997, 0.5f) ||
            !testResampling(44101, 48000, 1001, 997, 0.5f) ||
            !testResampling(48000, 16000, 1001, 997, 0.5f) ||
            !testResampling(48000, 16000, 1001, 12000, 0) ||
            !testResampling(48000, 44100, 1, 997, 0.5f) ||
            !testResampling(768000, 3000, 4097, 997, 0.5f)) {
            std::fprintf(stderr, "Native PCM resampling failed\n");
            return 8;
        }
        std::puts("NATIVE_PCM_PASS");
        return 0;
    }
#endif
    if (argc != 5 && argc != 6) {
        return 1;
    }
    auto bytes = readFile(argv[1]);
    auto reference = readFile(argv[2]);
    int rate = std::atoi(argv[3]);
    size_t channels = std::atoi(argv[4]);
    if (argc == 6 && std::strcmp(argv[5], "corpus")) {
        auto exercise = [](const std::vector<uint8_t>& input, size_t length) {
            Starfish::AudioContainerReader reader;
            if (reader.open(input.data(), length)) {
                Starfish::AudioContainerReader::Packet packet;
                size_t packets = 0;
                while (reader.next(packet)) {
                    if (++packets > length + 1) {
                        std::abort();
                    }
                }
            }
        };
        size_t samples = bytes.size() > 256 * 1024 ? 8 : 64;
        for (size_t length = 0; length < bytes.size();
             length += std::max<size_t>(1, bytes.size() / samples)) {
            exercise(bytes, length);
        }
        for (size_t i = 0; i < samples && !bytes.empty(); i++) {
            auto damaged = bytes;
            size_t offset = (i * 997) % bytes.size();
            damaged[offset] ^= 255;
            exercise(damaged, damaged.size());
        }
    }
    if (argc == 5) {
        Starfish::MP3MemoryDemuxer demuxer;
        if (bytes.empty() || !demuxer.open(bytes.data(), bytes.size())) {
            std::fprintf(stderr, "native demuxer did not accept fixture\n");
            return 2;
        }
        Starfish::MP3MemoryDemuxer::Packet packet;
        size_t packets = 0;
        while (demuxer.next(packet)) {
            if (packet.data < bytes.data() ||
                packet.data + packet.size > bytes.data() + bytes.size()) {
                return 3;
            }
            packets++;
        }
        // Exercise every prefix, including partial ID3, Xing and frame headers.
        // Failure is permitted; accepted prefixes must still yield bounded
        // views.
        for (size_t length = 0; length < bytes.size(); length++) {
            if (demuxer.open(bytes.data(), length)) {
                while (demuxer.next(packet)) {
                    if (packet.data + packet.size > bytes.data() + length) {
                        return 4;
                    }
                }
            }
        }
        if (!packets) {
            return 5;
        }
    }
    auto* pcm =
        Starfish::decodeCompressedAudio(bytes.data(), bytes.size(), rate);
    if (!pcm) {
        return 5;
    }
    size_t expected = reference.size() / (sizeof(float) * channels);
    if (pcm->channels() != channels || pcm->frames() != expected) {
        std::fprintf(stderr, "frames: %zu expected: %zu\n", pcm->frames(),
                     expected);
        pcm->release();
        return 6;
    }
    for (size_t frame = 0; frame < expected; frame++) {
        for (size_t channel = 0; channel < channels; channel++) {
            float value;
            memcpy(&value,
                   reference.data() +
                       (frame * channels + channel) * sizeof(float),
                   sizeof(float));
            if (!std::isfinite(pcm->channel(channel)[frame]) ||
                std::abs(value - pcm->channel(channel)[frame]) > 0.00002f) {
                std::fprintf(
                    stderr,
                    "PCM mismatch at frame %zu channel %zu: %.9g != %.9g\n",
                    frame, channel, pcm->channel(channel)[frame], value);
                pcm->release();
                return 7;
            }
        }
    }
    pcm->release();
    std::puts("NATIVE_AUDIO_PASS");
    return 0;
}
