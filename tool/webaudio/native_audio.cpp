#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioBufferData.h"
#include "platform/multimedia/MP3MemoryDemuxer.h"
#include "platform/multimedia/AudioContainerReader.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

static std::vector<uint8_t> readFile(const char* path)
{
    std::ifstream file(path, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
}

int main(int argc, char** argv)
{
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
