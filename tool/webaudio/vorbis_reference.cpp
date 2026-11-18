// Test-only reference: Xiph's decoder applies Ogg granules independently of
// FFmpeg's packet-duration/skip-sample handling, including sub-frame clips.
#include <vorbis/vorbisfile.h>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    OggVorbis_File file;
    if (ov_fopen(argv[1], &file)) {
        return 2;
    }
    int section = 0;
    float** pcm = nullptr;
    long count;
    std::vector<float> interleaved;
    while ((count = ov_read_float(&file, &pcm, 4096, &section)) > 0) {
        const vorbis_info* info = ov_info(&file, section);
        if (!info || info->channels <= 0 || info->channels > 32) {
            ov_clear(&file);
            return 3;
        }
        interleaved.resize(static_cast<size_t>(count) * info->channels);
        const int orders[6][8] = { { 0, 2, 1 },
                                   { 0, 1, 2, 3 },
                                   { 0, 2, 1, 3, 4 },
                                   { 0, 2, 1, 5, 3, 4 },
                                   { 0, 2, 1, 6, 5, 3, 4 },
                                   { 0, 2, 1, 7, 5, 6, 3, 4 } };
        for (long frame = 0; frame < count; frame++) {
            for (int channel = 0; channel < info->channels; channel++) {
                int source = info->channels >= 3 && info->channels <= 8
                                 ? orders[info->channels - 3][channel]
                                 : channel;
                interleaved[frame * info->channels + channel] =
                    pcm[source][frame];
            }
        }
        if (fwrite(interleaved.data(), sizeof(float), interleaved.size(),
                   stdout) != interleaved.size()) {
            ov_clear(&file);
            return 4;
        }
    }
    ov_clear(&file);
    return count < 0 ? 5 : 0;
}
