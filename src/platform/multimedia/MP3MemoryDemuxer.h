/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef __StarfishMP3MemoryDemuxer__
#define __StarfishMP3MemoryDemuxer__

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace Starfish {

// A complete-file Layer III reader. Packet views borrow the input buffer;
// neither the file nor a packet index is copied. Unsupported layouts fail
// without opening another container backend.
class MP3MemoryDemuxer {
public:
    struct Packet {
        const uint8_t* data;
        size_t size;
        uint64_t sampleOffset;
        uint32_t samples;
        uint32_t skip;
        uint32_t discard;
    };

    bool open(const uint8_t* bytes, size_t length)
    {
        *this = MP3MemoryDemuxer();
        m_bytes = bytes;
        m_end = length;
        size_t start = 0;
        while (length - start >= 10 && !memcmp(bytes + start, "ID3", 3)) {
            const uint8_t* tag = bytes + start;
            if (tag[3] < 2 || tag[3] > 4 ||
                ((tag[6] | tag[7] | tag[8] | tag[9]) & 0x80)) {
                return false;
            }
            size_t size = (size_t(tag[6]) << 21) | (size_t(tag[7]) << 14) |
                          (size_t(tag[8]) << 7) | tag[9];
            size += 10 + ((tag[3] == 4 && (tag[5] & 0x10)) ? 10 : 0);
            if (size > length - start) {
                return false;
            }
            start += size;
        }
        if (m_end - start >= 128 && !memcmp(bytes + m_end - 128, "TAG", 3)) {
            m_end -= 128;
        }
        // APEv2's size includes the footer and items, but not its optional
        // 32-byte header. ReplayGain tags are common on otherwise plain MP3.
        if (m_end - start >= 32 && !memcmp(bytes + m_end - 32, "APETAGEX", 8)) {
            const uint8_t* footer = bytes + m_end - 32;
            uint32_t size = little32(footer + 12);
            if (size < 32 || size > m_end - start) {
                return false;
            }
            m_end -= size;
            if (little32(footer + 20) & 0x80000000u) {
                if (m_end - start < 32 ||
                    memcmp(bytes + m_end - 32, "APETAGEX", 8)) {
                    return false;
                }
                m_end -= 32;
            }
        }
        Header first;
        if (!header(start, first)) {
            // Never sync-probe through another container's headers: doing so
            // would bypass that container's track selection and edit lists.
            if (m_end - start >= 4) {
                uint32_t magic = read32(bytes + start);
                if (magic == 0x52494646 || magic == 0x52494658 ||
                    magic == 0x52463634 || magic == 0x42573634 ||
                    magic == 0x4f676753 || magic == 0x664c6143 ||
                    magic == 0x1a45dfa3) {
                    return false;
                }
            }
            if (m_end - start >= 8) {
                const uint8_t* type = bytes + start + 4;
                if (!memcmp(type, "ftyp", 4) || !memcmp(type, "moov", 4) ||
                    !memcmp(type, "mdat", 4) || !memcmp(type, "wide", 4) ||
                    !memcmp(type, "free", 4) || !memcmp(type, "skip", 4)) {
                    return false;
                }
            }
            // Require consecutive matching frame boundaries when probing past
            // a small preamble, so a stray sync word is never enough.
            size_t limit = start + std::min<size_t>(m_end - start, 65536);
            bool found = false;
            for (size_t probe = start; probe < limit; probe++) {
                Header second, third;
                if (header(probe, first) &&
                    header(probe + first.size, second) &&
                    header(probe + first.size + second.size, third) &&
                    first.rate == second.rate && first.rate == third.rate &&
                    first.samples == second.samples &&
                    first.samples == third.samples) {
                    start = probe;
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        m_rate = first.rate;
        m_samplesPerFrame = first.samples;
        // Xing/Info occupies a metadata frame, not an audio frame. LAME's
        // delay/padding describe the original samples; Layer III synthesis
        // additionally delays output by 529 samples. See the LAME tag format:
        // https://lame.sourceforge.io/tech-FAQ.txt
        size_t info = start + 4 +
                      (first.samples == 1152 ? (first.mono ? 17 : 32)
                                             : (first.mono ? 9 : 17));
        size_t frameEnd = start + first.size;
        uint32_t declaredFrames = 0;
        if (frameEnd - start >= 40 && !memcmp(bytes + start + 36, "VBRI", 4)) {
            start = frameEnd;
        }
        if (info <= frameEnd && frameEnd - info >= 8 &&
            (!memcmp(bytes + info, "Xing", 4) ||
             !memcmp(bytes + info, "Info", 4))) {
            uint32_t flags = read32(bytes + info + 4);
            if (flags & ~15u) {
                return false;
            }
            size_t cursor = info + 8;
            const size_t sizes[] = { 4, 4, 100, 4 };
            for (unsigned bit = 0; bit < 4; bit++) {
                if (flags & (1u << bit)) {
                    if (sizes[bit] > frameEnd - cursor) {
                        return false;
                    }
                    if (!bit) {
                        declaredFrames = read32(bytes + cursor);
                    }
                    cursor += sizes[bit];
                }
            }
            if (frameEnd - cursor >= 24 &&
                (!memcmp(bytes + cursor, "LAME", 4) ||
                 !memcmp(bytes + cursor, "Lavc", 4) ||
                 !memcmp(bytes + cursor, "Lavf", 4))) {
                uint32_t delay = (uint32_t(bytes[cursor + 21]) << 4) |
                                 (bytes[cursor + 22] >> 4);
                uint32_t padding =
                    ((bytes[cursor + 22] & 15) << 8) | bytes[cursor + 23];
                m_skip = delay + 529;
                m_discard = padding > 529 ? padding - 529 : 0;
            }
            start = frameEnd;
        }
        // Validate complete framing before decoding. Unsupported free-format
        // bitrates and changing stream properties do not have a safe fixed
        // frame stride in this reader.
        size_t offset = start;
        uint64_t frames = 0;
        while (offset < m_end) {
            Header current;
            if (!header(offset, current) || current.rate != m_rate ||
                current.samples != m_samplesPerFrame) {
                return false;
            }
            offset += current.size;
            frames++;
        }
        m_totalSamples = frames * m_samplesPerFrame;
        if (declaredFrames && declaredFrames != frames) {
            // Editors may leave the old Xing count after appending audio.
            // Actual frame boundaries are authoritative; old tail padding is
            // no longer meaningful for the appended stream.
            m_discard = 0;
        }
        if (!frames || uint64_t(m_skip) + m_discard >= m_totalSamples) {
            return false;
        }
        m_offset = start;
        return true;
    }

    bool next(Packet& packet)
    {
        Header current;
        if (!header(m_offset, current)) {
            return false;
        }
        uint64_t end = m_position + current.samples;
        uint64_t keepEnd = m_totalSamples - m_discard;
        packet.data = m_bytes + m_offset;
        packet.size = current.size;
        packet.sampleOffset = m_position;
        packet.samples = current.samples;
        packet.skip =
            m_position >= m_skip
                ? 0
                : static_cast<uint32_t>(end <= m_skip ? current.samples
                                                      : m_skip - m_position);
        packet.discard =
            end <= keepEnd
                ? 0
                : static_cast<uint32_t>(m_position >= keepEnd ? current.samples
                                                              : end - keepEnd);
        m_offset += current.size;
        m_position = end;
        return true;
    }

    uint32_t sampleRate() const
    {
        return m_rate;
    }
    uint64_t samples() const
    {
        return m_totalSamples - m_skip - m_discard;
    }

private:
    static uint32_t little32(const uint8_t* p)
    {
        return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) |
               (uint32_t(p[3]) << 24);
    }
    struct Header {
        size_t size;
        uint32_t rate;
        uint32_t samples;
        bool mono;
    };

    static uint32_t read32(const uint8_t* p)
    {
        return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
               (uint32_t(p[2]) << 8) | p[3];
    }

    bool header(size_t offset, Header& result) const
    {
        if (offset > m_end || m_end - offset < 4) {
            return false;
        }
        uint32_t h = read32(m_bytes + offset);
        unsigned version = (h >> 19) & 3;
        unsigned bitrate = (h >> 12) & 15;
        unsigned frequency = (h >> 10) & 3;
        if ((h & 0xffe00000) != 0xffe00000 || version == 1 ||
            ((h >> 17) & 3) != 1 || !bitrate || bitrate == 15 ||
            frequency == 3) {
            return false;
        }
        const uint32_t rates[] = { 44100, 48000, 32000 };
        const uint32_t high[] = { 0,   32,  40,  48,  56,  64,  80, 96,
                                  112, 128, 160, 192, 224, 256, 320 };
        const uint32_t low[] = { 0,  8,  16, 24,  32,  40,  48, 56,
                                 64, 80, 96, 112, 128, 144, 160 };
        result.rate = rates[frequency] >> (version == 3   ? 0
                                           : version == 2 ? 1
                                                          : 2);
        result.samples = version == 3 ? 1152 : 576;
        result.mono = ((h >> 6) & 3) == 3;
        result.size = (version == 3 ? 144000 : 72000) *
                          (version == 3 ? high[bitrate] : low[bitrate]) /
                          result.rate +
                      ((h >> 9) & 1);
        return result.size <= m_end - offset;
    }

    const uint8_t* m_bytes{ nullptr };
    size_t m_offset{ 0 };
    size_t m_end{ 0 };
    uint32_t m_rate{ 0 };
    uint32_t m_samplesPerFrame{ 0 };
    uint32_t m_skip{ 0 };
    uint32_t m_discard{ 0 };
    uint64_t m_position{ 0 };
    uint64_t m_totalSamples{ 0 };
};

} // namespace Starfish

#endif
