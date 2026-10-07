#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
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
    if (argc != 3) {
        return 1;
    }
    std::vector<uint8_t> bytes = readFile(argv[1]);
    if (bytes.empty()) {
        return 2;
    }
    // A 100 s stereo track decodes to 38 MB of PCM. Only the AudioBuffer
    // limit may refuse it, not a tighter cap for decoder-side copies.
    std::vector<uint8_t> track = readFile(argv[2]);
    if (track.empty()) {
        return 7;
    }
    auto* music =
        Starfish::decodeCompressedAudio(track.data(), track.size(), 48000);
    if (!music) {
        return 8;
    }
    const size_t expectedFrames = 48000 * 100;
    bool complete = music->channels() == 2 &&
                    music->frames() + 4800 > expectedFrames &&
                    music->frames() < expectedFrames + 4800;
    // The tail must hold the tone, not unwritten capacity.
    float peak = 0;
    for (size_t frame = music->frames() - 48000;
         complete && frame < music->frames() - 47520; frame++) {
        peak = std::max(peak, std::abs(music->channel(1)[frame]));
    }
    music->release();
    if (!complete || peak < 0.01) {
        return 9;
    }
    const uint8_t invalid[] = { 0, 1, 2, 3 };
    const char playlist[] = "ffconcat version 1.0\nfile tone.wav\n";
    for (size_t iteration = 0; iteration < 16; iteration++) {
        auto* pcm =
            Starfish::decodeCompressedAudio(bytes.data(), bytes.size(), 48000);
        if (!pcm) {
            return 3;
        }
        bool valid = pcm->channels() == 1 && pcm->frames() == 4800 &&
                     std::abs(pcm->channel(0)[100]) > 0.01;
        pcm->release();
        if (!valid) {
            return 4;
        }
        pcm = Starfish::decodeCompressedAudio(invalid, sizeof(invalid), 48000);
        if (pcm) {
            pcm->release();
            return 5;
        }
        pcm = Starfish::decodeCompressedAudio(
            reinterpret_cast<const uint8_t*>(playlist), sizeof(playlist) - 1,
            48000);
        if (pcm) {
            pcm->release();
            return 6;
        }
    }
    std::puts("DECODER_MEMORY_PASS");
    return 0;
}
