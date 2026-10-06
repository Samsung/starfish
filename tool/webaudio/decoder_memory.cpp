#include "core/modules/webaudio/render/AudioDecoder.h"
#include "core/modules/webaudio/render/AudioBufferData.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    if (bytes.empty()) {
        return 2;
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
