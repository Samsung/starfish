/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef __StarfishAudioContainerReader__
#define __StarfishAudioContainerReader__

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace Starfish {

// Codec-independent framing for complete, memory-backed audio resources.
// Packet views remain valid until next(); configuration contains only the
// small codec headers, never a copy of the resource or a per-packet index.
class AudioContainerReader {
public:
    enum class Codec {
        Unknown,
        AAC,
        ALAC,
        MP3,
        FLAC,
        Vorbis,
        Opus,
        ALaw,
        MuLaw,
        IMAADPCM,
        MSADPCM,
        PCM
    };
    struct Edit {
        uint64_t start;
        uint64_t length;
    };
    struct Configuration {
        Codec codec{ Codec::Unknown };
        uint32_t rate{ 0 };
        uint32_t channels{ 0 };
        uint32_t bits{ 0 };
        uint32_t blockAlign{ 0 };
        uint64_t samples{ 0 };
        uint32_t skip{ 0 };
        bool floating{ false };
        bool bigEndian{ false };
        bool signed8{ false };
        bool parseFrames{ false };
        bool oggGranules{ false };
        std::vector<uint8_t> extra;
        std::vector<Edit> edits;
    };
    struct Packet {
        const uint8_t* data;
        size_t size;
        uint32_t discard;
        bool reconfigure{ false };
        int64_t granule{ -1 };
        bool endOfStream{ false };
        uint64_t granuleOrigin{ 0 };
        Packet(const uint8_t* bytes = nullptr, size_t length = 0,
               uint32_t trim = 0)
            : data(bytes)
            , size(length)
            , discard(trim)
        {
        }
    };

    bool open(const uint8_t* bytes, size_t length)
    {
        *this = AudioContainerReader();
        m_bytes = bytes;
        m_end = length;
        size_t leading = 0;
        while (length - leading >= 10 && !memcmp(bytes + leading, "ID3", 3)) {
            const uint8_t* tag = bytes + leading;
            if (tag[3] < 2 || tag[3] > 4 ||
                ((tag[6] | tag[7] | tag[8] | tag[9]) & 128)) {
                return false;
            }
            size_t size = (size_t(tag[6]) << 21) | (size_t(tag[7]) << 14) |
                          (size_t(tag[8]) << 7) | tag[9];
            size += 10 + ((tag[3] == 4 && (tag[5] & 16)) ? 10 : 0);
            if (size > length - leading) {
                return false;
            }
            leading += size;
        }
        if (leading) {
            return open(bytes + leading, length - leading);
        }
        if (length >= 12 &&
            (!memcmp(bytes, "RIFF", 4) || !memcmp(bytes, "RIFX", 4) ||
             !memcmp(bytes, "RF64", 4) || !memcmp(bytes, "BW64", 4)) &&
            !memcmp(bytes + 8, "WAVE", 4)) {
            return openWave();
        }
        if (length >= 4 && !memcmp(bytes, "fLaC", 4)) {
            return openFLAC();
        }
        if (length >= 4 && !memcmp(bytes, "OggS", 4)) {
            return openOgg();
        }
        if (length >= 4 && be32(bytes) == 0x1a45dfa3) {
            return openWebM();
        }
        if (length >= 8 &&
            (!memcmp(bytes + 4, "ftyp", 4) || !memcmp(bytes + 4, "moov", 4) ||
             !memcmp(bytes + 4, "wide", 4) || !memcmp(bytes + 4, "free", 4) ||
             !memcmp(bytes + 4, "skip", 4) || !memcmp(bytes + 4, "mdat", 4))) {
            return openMP4();
        }
        // ADTS (ISO/IEC 14496-3): layer must be zero, frequency index valid.
        if (length >= 7 && bytes[0] == 0xff && (bytes[1] & 0xf6) == 0xf0) {
            m_kind = Kind::ADTS;
            m_config.codec = Codec::AAC;
            return true;
        }
        return false;
    }

    const Configuration& configuration() const
    {
        return m_config;
    }
    bool failed() const
    {
        return m_failed;
    }

    bool next(Packet& packet)
    {
        if (m_kind == Kind::WebM) {
            return !m_failed && webmPacket(packet);
        }
        if (m_kind == Kind::MP4) {
            return !m_failed && mp4Packet(packet);
        }
        if (m_failed || (m_kind != Kind::Ogg && m_offset == m_end)) {
            return false;
        }
        if (m_kind == Kind::Ogg) {
            if (!oggPacket(packet)) {
                if (!m_failed && m_following && m_chains < 16) {
                    const uint8_t* following = m_following;
                    size_t length = m_followingLength;
                    unsigned chains = m_chains + 1;
                    if (!open(following, length)) {
                        return fail();
                    }
                    m_chains = chains;
                    if (!next(packet)) {
                        return false;
                    }
                    packet.reconfigure = true;
                    return true;
                }
                if (m_following && m_chains >= 16) {
                    return fail();
                }
                return false;
            }
            return true;
        }
        size_t count = std::min<size_t>(32768, m_end - m_offset);
        if (m_kind == Kind::ADTS) {
            if (m_end - m_offset < 7) {
                return fail();
            }
            const uint8_t* p = m_bytes + m_offset;
            count =
                (size_t(p[3] & 3) << 11) | (size_t(p[4]) << 3) | (p[5] >> 5);
            if (p[0] != 0xff || (p[1] & 0xf6) != 0xf0 ||
                ((p[2] >> 2) & 15) >= 13 || count < ((p[1] & 1) ? 7u : 9u) ||
                count > m_end - m_offset) {
                return fail();
            }
        } else if (m_config.blockAlign) {
            count -= count % m_config.blockAlign;
            if (!count) {
                return fail();
            }
        }
        packet = { m_bytes + m_offset, count };
        m_offset += count;
        return true;
    }

private:
    enum class Kind { Raw, ADTS, Ogg, MP4, WebM };
    struct Element {
        uint64_t id{ 0 };
        size_t begin{ 0 };
        size_t end{ 0 };
        size_t size() const
        {
            return end - begin;
        }
    };
    bool vint(size_t& cursor, size_t end, uint64_t& value, bool id = false,
              unsigned* width = nullptr)
    {
        if (cursor >= end || !m_bytes[cursor]) {
            return false;
        }
        uint8_t mask = 128;
        unsigned count = 1;
        while (!(m_bytes[cursor] & mask)) {
            mask >>= 1;
            count++;
        }
        if (count > (id ? 4u : 8u) || count > end - cursor) {
            return false;
        }
        value = m_bytes[cursor++] & (id ? 255 : mask - 1);
        for (unsigned i = 1; i < count; i++) {
            value = (value << 8) | m_bytes[cursor++];
        }
        if (width) {
            *width = count;
        }
        return true;
    }
    bool element(size_t& cursor, size_t end, Element& result)
    {
        uint64_t size;
        unsigned width;
        if (!vint(cursor, end, result.id, true) ||
            !vint(cursor, end, size, false, &width)) {
            return fail();
        }
        if (size == (uint64_t(1) << (width * 7)) - 1) {
            size = end - cursor;
        }
        if (size > end - cursor) {
            return fail();
        }
        result.begin = cursor;
        result.end = cursor + static_cast<size_t>(size);
        cursor = result.end;
        return true;
    }
    Element findElement(const Element& parent, uint64_t id)
    {
        size_t cursor = parent.begin;
        Element found;
        while (cursor < parent.end && element(cursor, parent.end, found)) {
            if (found.id == id) {
                return found;
            }
        }
        return Element();
    }
    uint64_t integer(const Element& e)
    {
        if (!e.id || e.size() > 8) {
            fail();
            return 0;
        }
        uint64_t value = 0;
        for (size_t i = e.begin; i < e.end; i++) {
            value = (value << 8) | m_bytes[i];
        }
        return value;
    }
    bool openWebM()
    {
        m_kind = Kind::WebM;
        Element root;
        root.end = m_end;
        Element segment = findElement(root, 0x18538067);
        Element tracks = findElement(segment, 0x1654ae6b);
        if (!tracks.id) {
            return false;
        }
        size_t scan = tracks.begin;
        unsigned totalTracks = 0;
        Element candidate;
        while (scan < tracks.end && element(scan, tracks.end, candidate)) {
            if (candidate.id == 0xae && ++totalTracks > 16) {
                return false;
            }
        }
        size_t cursor = tracks.begin;
        Element entry;
        unsigned count = 0;
        while (cursor < tracks.end && element(cursor, tracks.end, entry)) {
            if (entry.id != 0xae) {
                continue;
            }
            if (++count > 16) {
                return false;
            }
            if (integer(findElement(entry, 0x83)) != 2) {
                continue;
            }
            m_webmTrack = integer(findElement(entry, 0xd7));
            Element codec = findElement(entry, 0x86);
            const uint8_t* p = m_bytes + codec.begin;
            if (codec.size() == 6 && !memcmp(p, "A_OPUS", 6)) {
                m_config.codec = Codec::Opus;
            } else if (codec.size() == 8 && !memcmp(p, "A_VORBIS", 8)) {
                m_config.codec = Codec::Vorbis;
            } else if (codec.size() == 5 && !memcmp(p, "A_AAC", 5)) {
                m_config.codec = Codec::AAC;
            } else if (codec.size() == 9 && !memcmp(p, "A_MPEG/L3", 9)) {
                m_config.codec = Codec::MP3;
            } else if (codec.size() == 6 && !memcmp(p, "A_FLAC", 6)) {
                m_config.codec = Codec::FLAC;
            } else {
                return false;
            }
            Element extra = findElement(entry, 0x63a2);
            if (extra.size() > 1024 * 1024) {
                return false;
            }
            if (extra.id) {
                m_config.extra.assign(m_bytes + extra.begin,
                                      m_bytes + extra.end);
                if (m_config.codec == Codec::FLAC &&
                    m_config.extra.size() >= 42 &&
                    !memcmp(m_config.extra.data(), "fLaC", 4)) {
                    m_config.extra =
                        std::vector<uint8_t>(m_config.extra.begin() + 8,
                                             m_config.extra.begin() + 42);
                }
            }
            Element audio = findElement(entry, 0xe1);
            Element channels = findElement(audio, 0x9f);
            m_config.channels = channels.id ? integer(channels) : 1;
            Element frequency = findElement(audio, 0x78b5);
            if (!frequency.id) {
                frequency = findElement(audio, 0xb5);
            }
            double rate = 8000;
            uint64_t bits = frequency.id ? integer(frequency) : 0;
            if (frequency.size() == 4) {
                uint32_t b = bits;
                float f;
                memcpy(&f, &b, 4);
                rate = f;
            } else if (frequency.size() == 8) {
                memcpy(&rate, &bits, 8);
            } else if (frequency.id) {
                return false;
            }
            if (!(rate >= 3000 && rate <= 768000)) {
                return false;
            }
            m_config.rate = m_config.codec == Codec::Opus
                                ? 48000
                                : static_cast<uint32_t>(rate);
            if (m_config.codec == Codec::Opus && m_config.extra.size() >= 12) {
                m_config.skip = le16(m_config.extra.data() + 10);
            }
            Element delay = findElement(entry, 0x56aa);
            if (delay.id) {
                uint64_t ns = integer(delay);
                if (ns > (UINT64_MAX - 500000000) / m_config.rate) {
                    return false;
                }
                uint64_t samples =
                    (ns * m_config.rate + 500000000) / 1000000000;
                if (samples > UINT32_MAX) {
                    return false;
                }
                m_config.skip = static_cast<uint32_t>(samples);
            }
            m_offset = segment.begin;
            m_end = segment.end;
            return m_webmTrack && m_config.channels &&
                   m_config.channels <= 32 && !m_failed;
        }
        return false;
    }
    bool startBlock(const Element& block, uint32_t discard)
    {
        size_t cursor = block.begin;
        uint64_t track;
        if (!vint(cursor, block.end, track) || block.end - cursor < 3) {
            return fail();
        }
        if (track != m_webmTrack) {
            return true;
        }
        unsigned lacing = (m_bytes[cursor + 2] >> 1) & 3;
        cursor += 3;
        unsigned count = 1;
        if (lacing) {
            if (cursor == block.end) {
                return fail();
            }
            count = unsigned(m_bytes[cursor++]) + 1;
        }
        m_laces.assign(count, 0);
        uint64_t total = 0;
        int64_t previous = 0;
        for (unsigned i = 0; i + 1 < count; i++) {
            uint64_t size = 0;
            if (lacing == 1) {
                uint8_t value;
                do {
                    if (cursor == block.end) {
                        return fail();
                    }
                    value = m_bytes[cursor++];
                    size += value;
                } while (value == 255);
            } else if (lacing == 2) {
                if ((block.end - cursor) % count) {
                    return fail();
                }
                size = (block.end - cursor) / count;
            } else if (lacing == 3) {
                unsigned width;
                if (!vint(cursor, block.end, size, false, &width)) {
                    return fail();
                }
                int64_t current = static_cast<int64_t>(size);
                if (i) {
                    current += previous - ((int64_t(1) << (width * 7 - 1)) - 1);
                }
                if (current < 0) {
                    return fail();
                }
                previous = current;
                size = current;
            }
            if (size > block.size() || total > block.size() - size) {
                return fail();
            }
            m_laces[i] = static_cast<size_t>(size);
            total += size;
        }
        if (total > block.end - cursor) {
            return fail();
        }
        m_laces.back() = block.end - cursor - static_cast<size_t>(total);
        m_laceIndex = 0;
        m_laceData = m_bytes + cursor;
        m_laceDiscard = discard;
        return true;
    }
    bool webmPacket(Packet& packet)
    {
        for (;;) {
            if (m_laceIndex < m_laces.size()) {
                size_t size = m_laces[m_laceIndex++];
                packet = { m_laceData, size,
                           m_laceIndex == m_laces.size() ? m_laceDiscard : 0 };
                m_laceData += size;
                return true;
            }
            if (m_offset == m_end) {
                return false;
            }
            Element e;
            if (m_offset >= m_clusterEnd) {
                if (!element(m_offset, m_end, e)) {
                    return false;
                }
                if (e.id == 0x1f43b675) {
                    m_clusterEnd = e.end;
                    m_offset = e.begin;
                }
                continue;
            }
            if (!element(m_offset, m_clusterEnd, e)) {
                return false;
            }
            if (e.id == 0x1f43b675) {
                // An unknown-sized Cluster ends when the next Cluster begins.
                m_clusterEnd = e.end;
                m_offset = e.begin;
                continue;
            }
            if (e.id == 0xa3) {
                if (!startBlock(e, 0)) {
                    return false;
                }
            } else if (e.id == 0xa0) {
                Element block = findElement(e, 0xa1);
                Element trim = findElement(e, 0x75a2);
                uint64_t ns = trim.id ? integer(trim) : 0;
                if (trim.id && trim.size() && (m_bytes[trim.begin] & 128)) {
                    return fail();
                }
                if (ns > (UINT64_MAX - 500000000) / m_config.rate) {
                    return fail();
                }
                uint64_t samples =
                    (ns * m_config.rate + 500000000) / 1000000000;
                if (samples > UINT32_MAX || !block.id ||
                    !startBlock(block, static_cast<uint32_t>(samples))) {
                    return fail();
                }
            }
        }
    }
    static uint32_t be32(const uint8_t* p)
    {
        return (be24(p) << 8) | p[3];
    }
    static uint64_t be64(const uint8_t* p)
    {
        return (uint64_t(be32(p)) << 32) | be32(p + 4);
    }
    struct Box {
        size_t begin{ 0 };
        size_t end{ 0 };
        const uint8_t* type{ nullptr };
        bool is(const char* value) const
        {
            return type && !memcmp(type, value, 4);
        }
        size_t size() const
        {
            return end - begin;
        }
    };
    bool box(size_t& cursor, size_t end, Box& result)
    {
        if (cursor > end || end - cursor < 8) {
            return fail();
        }
        const uint8_t* p = m_bytes + cursor;
        uint64_t size = be32(p);
        size_t header = 8;
        if (size == 1) {
            if (end - cursor < 16) {
                return fail();
            }
            size = be64(p + 8);
            header = 16;
        } else if (!size) {
            size = end - cursor;
        }
        if (size < header || size > end - cursor) {
            return fail();
        }
        result.begin = cursor + header;
        result.end = cursor + static_cast<size_t>(size);
        result.type = p + 4;
        cursor = result.end;
        return true;
    }
    Box child(const Box& parent, const char* name)
    {
        size_t cursor = parent.begin;
        Box result;
        while (cursor < parent.end && box(cursor, parent.end, result)) {
            if (result.is(name)) {
                return result;
            }
        }
        return Box();
    }
    bool table(const Box& b, size_t stride, uint32_t& count)
    {
        if (!b.type || b.size() < 8) {
            return false;
        }
        count = be32(m_bytes + b.begin + 4);
        return count <= (b.size() - 8) / stride;
    }
    bool descriptor(size_t& cursor, size_t end, unsigned& tag, size_t& stop)
    {
        if (cursor == end) {
            return false;
        }
        tag = m_bytes[cursor++];
        uint32_t size = 0;
        for (unsigned i = 0; i < 4 && cursor < end; i++) {
            uint8_t value = m_bytes[cursor++];
            size = (size << 7) | (value & 127);
            if (!(value & 128)) {
                if (size > end - cursor) {
                    return false;
                }
                stop = cursor + size;
                return true;
            }
        }
        return false;
    }
    bool mp4Description(const Box& stsd, uint32_t index = 1)
    {
        if (stsd.size() < 8 || !index ||
            index > be32(m_bytes + stsd.begin + 4)) {
            return false;
        }
        size_t cursor = stsd.begin + 8;
        Box entry;
        for (uint32_t i = 1; i < index; i++) {
            if (!box(cursor, stsd.end, entry)) {
                return false;
            }
        }
        if (!box(cursor, stsd.end, entry) || entry.size() < 28) {
            return false;
        }
        const uint8_t* p = m_bytes + entry.begin;
        unsigned version = (unsigned(p[8]) << 8) | p[9];
        if (version > 2 || entry.size() < (version == 2 ? 64u
                                           : version    ? 44u
                                                        : 28u)) {
            return false;
        }
        m_config.channels = (unsigned(p[16]) << 8) | p[17];
        m_config.rate = be32(p + 24) >> 16;
        m_config.floating = m_config.bigEndian = m_config.signed8 = false;
        m_config.bits = 0;
        Box children = entry;
        children.begin += version == 2 ? 64 : version ? 44 : 28;
        m_config.extra.clear();
        if (entry.is("mp4a")) {
            Box esds = child(children, "esds");
            if (!esds.type) {
                Box wave = child(children, "wave");
                if (wave.type) {
                    esds = child(wave, "esds");
                }
            }
            if (!esds.type || esds.size() < 4) {
                return false;
            }
            cursor = esds.begin + 4;
            size_t stop;
            unsigned tag;
            if (!descriptor(cursor, esds.end, tag, stop) || tag != 3 ||
                stop - cursor < 3) {
                return false;
            }
            unsigned flags = m_bytes[cursor + 2];
            cursor += 3;
            if (flags & 128) {
                cursor += 2;
            }
            if (flags & 64) {
                if (cursor >= stop) {
                    return false;
                }
                cursor += size_t(m_bytes[cursor]) + 1;
            }
            if (flags & 32) {
                cursor += 2;
            }
            if (cursor > stop || !descriptor(cursor, stop, tag, stop) ||
                tag != 4 || stop - cursor < 13) {
                return false;
            }
            if (m_bytes[cursor] == 0x6b || m_bytes[cursor] == 0x69) {
                m_config.codec = Codec::MP3;
                return m_config.channels && m_config.channels <= 32 &&
                       m_config.rate;
            }
            if (m_bytes[cursor] != 0x40 && m_bytes[cursor] != 0x66 &&
                m_bytes[cursor] != 0x67 && m_bytes[cursor] != 0x68) {
                return false;
            }
            cursor += 13;
            if (!descriptor(cursor, stop, tag, stop) || tag != 5 ||
                cursor == stop || stop - cursor > 1024 * 1024) {
                return false;
            }
            m_config.extra.assign(m_bytes + cursor, m_bytes + stop);
            m_config.codec = Codec::AAC;
        } else if (entry.is("Opus")) {
            Box dops = child(children, "dOps");
            if (!dops.type || dops.size() < 11 || dops.size() > 1024 * 1024) {
                return false;
            }
            p = m_bytes + dops.begin;
            m_config.codec = Codec::Opus;
            m_config.channels = p[1];
            m_config.rate = 48000;
            m_config.extra.assign({ 'O', 'p', 'u', 's', 'H', 'e', 'a', 'd', 1,
                                    p[1], p[3], p[2], p[7], p[6], p[5], p[4],
                                    p[9], p[8], p[10] });
            m_config.extra.insert(m_config.extra.end(), p + 11,
                                  m_bytes + dops.end);
        } else if (entry.is("alac")) {
            Box alac = child(children, "alac");
            if (!alac.type) {
                alac = child(child(children, "wave"), "alac");
            }
            if (!alac.type || alac.size() != 28) {
                return false;
            }
            m_config.codec = Codec::ALAC;
            m_config.extra.assign(m_bytes + alac.begin - 8, m_bytes + alac.end);
            m_config.channels = m_config.extra[21];
            m_config.bits = m_config.extra[17];
            m_config.rate = be32(m_config.extra.data() + 32);
        } else if (entry.is("fLaC")) {
            Box dfla = child(children, "dfLa");
            if (!dfla.type || dfla.size() < 42) {
                return false;
            }
            m_config.codec = Codec::FLAC;
            m_config.extra.assign(m_bytes + dfla.begin + 8,
                                  m_bytes + dfla.begin + 42);
            m_config.rate = be24(m_config.extra.data() + 10) >> 4;
        } else if (entry.is(".mp3") || entry.is("mp3 ")) {
            m_config.codec = Codec::MP3;
        } else if (entry.is("sowt") || entry.is("twos") || entry.is("raw ") ||
                   entry.is("fl32") || entry.is("fl64") || entry.is("in24") ||
                   entry.is("in32") || entry.is("lpcm")) {
            m_config.codec = Codec::PCM;
            m_config.bits = (unsigned(m_bytes[entry.begin + 18]) << 8) |
                            m_bytes[entry.begin + 19];
            m_config.floating = entry.is("fl32") || entry.is("fl64");
            m_config.bigEndian = !entry.is("sowt") && !entry.is("raw ");
            m_config.signed8 = entry.is("twos");
            if (version == 2) {
                uint64_t bits = be64(m_bytes + entry.begin + 32);
                double rate;
                memcpy(&rate, &bits, sizeof(rate));
                if (!(rate >= 1 && rate <= 768000)) {
                    return false;
                }
                m_config.rate = static_cast<uint32_t>(rate);
                m_config.channels = be32(m_bytes + entry.begin + 40);
                m_config.bits = be32(m_bytes + entry.begin + 48);
                uint32_t flags = be32(m_bytes + entry.begin + 52);
                if (flags & 32) {
                    return false;
                }
                m_config.floating = flags & 1;
                m_config.bigEndian = flags & 2;
                m_config.signed8 = flags & 4;
            }
        } else {
            return false;
        }
        return m_config.channels && m_config.channels <= 32 && m_config.rate;
    }
    bool openMP4()
    {
        m_kind = Kind::MP4;
        Box root;
        root.end = m_end;
        Box moov = child(root, "moov");
        if (!moov.type) {
            return false;
        }
        size_t scan = moov.begin;
        unsigned totalTracks = 0;
        Box candidate;
        while (scan < moov.end && box(scan, moov.end, candidate)) {
            if (candidate.is("trak") && ++totalTracks > 16) {
                return false;
            }
        }
        size_t cursor = moov.begin;
        Box track;
        unsigned tracks = 0;
        while (cursor < moov.end && box(cursor, moov.end, track)) {
            if (!track.is("trak")) {
                continue;
            }
            if (++tracks > 16) {
                return false;
            }
            Box mdia = child(track, "mdia");
            if (!mdia.type) {
                return false;
            }
            Box handler = child(mdia, "hdlr");
            if (!handler.type || handler.size() < 12) {
                return false;
            }
            if (memcmp(m_bytes + handler.begin + 8, "soun", 4)) {
                continue;
            }
            Box minf = child(mdia, "minf");
            Box stbl = child(minf, "stbl");
            m_stsd = child(stbl, "stsd");
            if (!stbl.type || !mp4Description(m_stsd)) {
                return false;
            }
            m_stsz = child(stbl, "stsz");
            if (!m_stsz.type) {
                m_stsz = child(stbl, "stz2");
                if (m_stsz.type && m_stsz.size() >= 12) {
                    m_sampleBits = m_bytes[m_stsz.begin + 7];
                    if (m_sampleBits != 4 && m_sampleBits != 8 &&
                        m_sampleBits != 16) {
                        return false;
                    }
                }
            }
            m_stsc = child(stbl, "stsc");
            m_stco = child(stbl, "stco");
            m_largeChunks = !m_stco.type;
            if (m_largeChunks) {
                m_stco = child(stbl, "co64");
            }
            if (!m_stsz.type || m_stsz.size() < 12 ||
                !table(m_stsc, 12, m_stscCount) ||
                !table(m_stco, m_largeChunks ? 8 : 4, m_chunkCount)) {
                return false;
            }
            m_uniformSize =
                m_stsz.is("stz2") ? 0 : be32(m_bytes + m_stsz.begin + 4);
            m_sampleCount = be32(m_bytes + m_stsz.begin + 8);
            if (!m_uniformSize && uint64_t(m_sampleCount) * m_sampleBits >
                                      uint64_t(m_stsz.size() - 12) * 8) {
                return false;
            }
            Box mdhd = child(mdia, "mdhd");
            if (!mdhd.type || mdhd.size() < 24) {
                return false;
            }
            const uint8_t* p = m_bytes + mdhd.begin;
            uint32_t scale;
            uint64_t duration;
            if (p[0] == 0) {
                scale = be32(p + 12);
                duration = be32(p + 16);
            } else if (p[0] == 1 && mdhd.size() >= 36) {
                scale = be32(p + 20);
                duration = be64(p + 24);
            } else {
                return false;
            }
            if (!scale || duration > UINT64_MAX / m_config.rate) {
                return false;
            }
            uint64_t skip = 0;
            Box edts = child(track, "edts");
            Box elst = child(edts, "elst");
            if (elst.type) {
                if (elst.size() < 8) {
                    return false;
                }
                unsigned version = m_bytes[elst.begin];
                size_t stride = version ? 20 : 12;
                uint32_t count = be32(m_bytes + elst.begin + 4);
                if (version > 1 || count > (elst.size() - 8) / stride ||
                    count > 1024) {
                    return false;
                }
                Box mvhd = child(moov, "mvhd");
                if (!mvhd.type || mvhd.size() < 24) {
                    return false;
                }
                unsigned mvVersion = m_bytes[mvhd.begin];
                uint32_t movieScale =
                    be32(m_bytes + mvhd.begin + (mvVersion ? 20 : 12));
                if (mvVersion > 1 || !movieScale) {
                    return false;
                }
                std::vector<Edit> edits;
                for (uint32_t i = 0; i < count; i++) {
                    p = m_bytes + elst.begin + 8 + i * stride;
                    uint64_t length = version ? be64(p) : be32(p);
                    uint64_t time = version ? be64(p + 8) : be32(p + 4);
                    if (be32(p + stride - 4) != 0x00010000) {
                        return false;
                    }
                    if (time == (version ? UINT64_MAX : UINT32_MAX)) {
                        // Empty edits offset the media track on the movie
                        // timeline; they contain no audio to decode.
                        continue;
                    }
                    if (time > UINT64_MAX / m_config.rate ||
                        length > UINT64_MAX / m_config.rate) {
                        return false;
                    }
                    if (edits.empty()) {
                        skip = time;
                    }
                    edits.push_back({ time * m_config.rate / scale,
                                      length * m_config.rate / movieScale });
                }
                if (edits.size() > 1) {
                    uint64_t previousEnd = 0;
                    for (const auto& edit : edits) {
                        if (!edit.length || edit.start < previousEnd ||
                            edit.length > UINT64_MAX - edit.start) {
                            return false;
                        }
                        previousEnd = edit.start + edit.length;
                    }
                    m_config.edits = std::move(edits);
                    skip = 0;
                }
            }
            // A short presentation (or an empty DASH initialization movie)
            // can be shorter than the codec's encoder delay.
            if (skip > UINT64_MAX / m_config.rate ||
                skip * uint64_t(m_config.rate) / scale > UINT32_MAX) {
                return false;
            }
            m_config.skip = skip * m_config.rate / scale;
            if (m_config.codec == Codec::Opus && m_config.extra.size() >= 12) {
                m_config.skip =
                    std::max(m_config.skip, le16(m_config.extra.data() + 10));
            }
            // Decode complete access units after the edit-list start trim;
            // media duration can omit tail samples emitted by the codec.
            m_config.samples = 0;
            m_offset = 0;
            if (!m_sampleCount) {
                Box tkhd = child(track, "tkhd");
                if (!tkhd.type || tkhd.size() < 24) {
                    return false;
                }
                p = m_bytes + tkhd.begin;
                m_trackID = be32(p + (p[0] ? 20 : 12));
                Box mvex = child(moov, "mvex");
                size_t c = mvex.begin;
                Box trex;
                while (c < mvex.end && box(c, mvex.end, trex)) {
                    if (trex.is("trex") && trex.size() >= 24 &&
                        be32(m_bytes + trex.begin + 4) == m_trackID) {
                        m_fragmentDefaultSize = be32(m_bytes + trex.begin + 16);
                    }
                }
                m_fragmented = true;
                return mvex.type && !m_failed;
            }
            return m_stscCount && !m_failed;
        }
        return false;
    }
    bool mp4Packet(Packet& packet)
    {
        bool changed = false;
        if (m_fragmented) {
            return fragmentPacket(packet);
        }
        if (m_sampleIndex == m_sampleCount) {
            return false;
        }
        if (!m_chunkRemaining) {
            if (m_chunkIndex >= m_chunkCount) {
                return fail();
            }
            uint32_t chunk = ++m_chunkIndex;
            while (m_stscIndex + 1 < m_stscCount &&
                   be32(m_bytes + m_stsc.begin + 8 + (m_stscIndex + 1) * 12) <=
                       chunk) {
                m_stscIndex++;
            }
            const uint8_t* p = m_bytes + m_stsc.begin + 8 + m_stscIndex * 12;
            if (be32(p) > chunk || !be32(p + 8)) {
                return fail();
            }
            uint32_t description = be32(p + 8);
            if (description != m_descriptionIndex) {
                Codec codec = m_config.codec;
                uint32_t rate = m_config.rate, channels = m_config.channels;
                uint32_t bits = m_config.bits;
                bool floating = m_config.floating, endian = m_config.bigEndian;
                bool signed8 = m_config.signed8;
                auto extra = std::move(m_config.extra);
                if (!mp4Description(m_stsd, description)) {
                    return fail();
                }
                m_descriptionIndex = description;
                changed =
                    codec != m_config.codec || rate != m_config.rate ||
                    channels != m_config.channels || bits != m_config.bits ||
                    floating != m_config.floating ||
                    endian != m_config.bigEndian ||
                    signed8 != m_config.signed8 || extra != m_config.extra;
                if (changed && m_sampleIndex) {
                    m_config.skip = 0;
                }
            }
            m_chunkRemaining = be32(p + 4);
            if (!m_chunkRemaining ||
                m_chunkRemaining > m_sampleCount - m_sampleIndex) {
                return fail();
            }
            p = m_bytes + m_stco.begin + 8 +
                (chunk - 1) * (m_largeChunks ? 8 : 4);
            uint64_t offset = m_largeChunks ? be64(p) : be32(p);
            if (offset > m_end) {
                return fail();
            }
            m_chunkOffset = static_cast<size_t>(offset);
        }
        uint32_t size = sampleSize(m_sampleIndex);
        uint32_t count = 1;
        if (m_config.codec == Codec::PCM && m_uniformSize) {
            // PCM sample tables often describe one sample per entry. Keep
            // decoder calls bounded by chunks, rather than the sample rate.
            count = std::min(m_chunkRemaining,
                             std::max<uint32_t>(1, 32768 / m_uniformSize));
            size *= count;
        }
        if (!size || size > m_end - m_chunkOffset) {
            return fail();
        }
        packet = { m_bytes + m_chunkOffset, size };
        packet.reconfigure = changed;
        m_chunkOffset += size;
        m_chunkRemaining -= count;
        m_sampleIndex += count;
        return true;
    }

    uint32_t sampleSize(uint32_t index) const
    {
        if (m_uniformSize) {
            return m_uniformSize;
        }
        const uint8_t* p = m_bytes + m_stsz.begin + 12;
        if (m_sampleBits == 4) {
            return (p[index / 2] >> ((index & 1) ? 0 : 4)) & 15;
        }
        if (m_sampleBits == 8) {
            return p[index];
        }
        if (m_sampleBits == 16) {
            return (uint32_t(p[size_t(index) * 2]) << 8) |
                   p[size_t(index) * 2 + 1];
        }
        return be32(p + size_t(index) * 4);
    }

    struct Run {
        size_t cursor;
        size_t end;
        size_t data;
        uint32_t count;
        uint32_t flags;
        uint32_t size;
    };
    bool loadFragment(const Box& moof)
    {
        size_t cursor = moof.begin;
        Box traf;
        while (cursor < moof.end && box(cursor, moof.end, traf)) {
            if (!traf.is("traf")) {
                continue;
            }
            Box tfhd = child(traf, "tfhd");
            if (!tfhd.type || tfhd.size() < 8) {
                return fail();
            }
            const uint8_t* p = m_bytes + tfhd.begin;
            if (be32(p + 4) != m_trackID) {
                continue;
            }
            uint32_t flags = be24(p + 1);
            size_t c = tfhd.begin + 8;
            uint64_t base = static_cast<size_t>(moof.type - m_bytes - 4);
            if (flags & 1) {
                if (tfhd.end - c < 8) {
                    return fail();
                }
                base = be64(m_bytes + c);
                c += 8;
            }
            if (flags & 2) {
                c += 4;
            }
            if (flags & 8) {
                c += 4;
            }
            uint32_t size = m_fragmentDefaultSize;
            if (c > tfhd.end) {
                return fail();
            }
            if (flags & 16) {
                if (tfhd.end - c < 4) {
                    return fail();
                }
                size = be32(m_bytes + c);
                c += 4;
            }
            if (flags & 32) {
                c += 4;
            }
            if (c > tfhd.end || base > m_end) {
                return fail();
            }
            size_t data = moof.end;
            Box mdat;
            size_t next = moof.end;
            if (next < m_end && box(next, m_end, mdat) && mdat.is("mdat")) {
                data = mdat.begin;
            }
            c = traf.begin;
            Box trun;
            while (c < traf.end && box(c, traf.end, trun)) {
                if (!trun.is("trun")) {
                    continue;
                }
                if (trun.size() < 8 || m_runs.size() >= 4096) {
                    return fail();
                }
                p = m_bytes + trun.begin;
                uint32_t f = be24(p + 1);
                uint32_t count = be32(p + 4);
                size_t entries = trun.begin + 8;
                if (f & 1) {
                    if (trun.end - entries < 4) {
                        return fail();
                    }
                    int64_t offset =
                        static_cast<int32_t>(be32(m_bytes + entries));
                    int64_t absolute = static_cast<int64_t>(base) + offset;
                    if (absolute < 0 || uint64_t(absolute) > m_end) {
                        return fail();
                    }
                    data = static_cast<size_t>(absolute);
                    entries += 4;
                }
                if (f & 4) {
                    entries += 4;
                }
                size_t stride = ((f & 256) ? 4 : 0) + ((f & 512) ? 4 : 0) +
                                ((f & 1024) ? 4 : 0) + ((f & 2048) ? 4 : 0);
                if (entries > trun.end ||
                    (stride && count > (trun.end - entries) / stride)) {
                    return fail();
                }
                m_runs.push_back({ entries, trun.end, data, count, f, size });
                // Locate the next run without retaining a sample index.
                for (uint32_t i = 0; i < count; i++) {
                    size_t field = entries + i * stride + ((f & 256) ? 4 : 0);
                    uint32_t sampleSize =
                        (f & 512) ? be32(m_bytes + field) : size;
                    if (!sampleSize || sampleSize > m_end - data) {
                        return fail();
                    }
                    data += sampleSize;
                }
            }
        }
        return !m_failed;
    }
    bool fragmentPacket(Packet& packet)
    {
        for (;;) {
            if (m_runIndex < m_runs.size()) {
                Run& run = m_runs[m_runIndex];
                if (!run.count) {
                    m_runIndex++;
                    continue;
                }
                size_t c = run.cursor;
                if (run.flags & 256) {
                    c += 4;
                }
                uint32_t size =
                    (run.flags & 512) ? be32(m_bytes + c) : run.size;
                if (!size || size > m_end - run.data) {
                    return fail();
                }
                packet = { m_bytes + run.data, size };
                run.data += size;
                run.cursor +=
                    ((run.flags & 256) ? 4 : 0) + ((run.flags & 512) ? 4 : 0) +
                    ((run.flags & 1024) ? 4 : 0) + ((run.flags & 2048) ? 4 : 0);
                run.count--;
                return true;
            }
            m_runs.clear();
            m_runIndex = 0;
            if (m_fragmentOffset == m_end) {
                return false;
            }
            Box b;
            if (!box(m_fragmentOffset, m_end, b)) {
                return false;
            }
            if (b.is("moof") && !loadFragment(b)) {
                return false;
            }
        }
    }
    static uint32_t le16(const uint8_t* p)
    {
        return p[0] | (uint32_t(p[1]) << 8);
    }
    static uint32_t le32(const uint8_t* p)
    {
        return le16(p) | (le16(p + 2) << 16);
    }
    static uint64_t le64(const uint8_t* p)
    {
        return le32(p) | (uint64_t(le32(p + 4)) << 32);
    }
    static uint32_t be24(const uint8_t* p)
    {
        return (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2];
    }
    bool fail()
    {
        m_failed = true;
        return false;
    }

    bool openFLAC()
    {
        // RFC 9639 section 8: STREAMINFO must be the first metadata block.
        size_t offset = 4;
        bool first = true;
        bool last = false;
        while (!last) {
            if (m_end - offset < 4) {
                return false;
            }
            const uint8_t* p = m_bytes + offset;
            last = p[0] & 0x80;
            size_t size = be24(p + 1);
            offset += 4;
            if (size > m_end - offset || (p[0] & 127) == 127) {
                return false;
            }
            if (first) {
                if ((p[0] & 127) || size != 34) {
                    return false;
                }
                const uint8_t* info = m_bytes + offset;
                m_config.extra.assign(info, info + size);
                m_config.rate = (be24(info + 10) >> 4);
                m_config.channels = ((info[12] >> 1) & 7) + 1;
                m_config.bits = (((info[12] & 1) << 4) | (info[13] >> 4)) + 1;
                m_config.samples = (uint64_t(info[13] & 15) << 32) |
                                   (uint64_t(be24(info + 14)) << 8) | info[17];
                first = false;
            } else if (!(p[0] & 127)) {
                return false;
            }
            offset += size;
        }
        m_config.codec = Codec::FLAC;
        m_config.parseFrames = true;
        m_offset = offset;
        return m_config.rate && offset < m_end;
    }

    bool openWave()
    {
        m_bigWave = !memcmp(m_bytes, "RIFX", 4);
        m_config.bigEndian = m_bigWave;
        bool large = !memcmp(m_bytes, "RF64", 4) || !memcmp(m_bytes, "BW64", 4);
        uint64_t largeData = 0;
        uint64_t end = uint64_t(wave32(m_bytes + 4)) + 8;
        if (large) {
            // RF64/BW64 ds64 replaces the 32-bit RIFF/data lengths. All
            // borrowed views are still bounded by the actual memory buffer.
            if (m_end < 48 || memcmp(m_bytes + 12, "ds64", 4) ||
                le32(m_bytes + 16) < 28 || le32(m_bytes + 16) > m_end - 20) {
                return false;
            }
            uint64_t riff = le64(m_bytes + 20);
            if (riff > m_end - 8) {
                return false;
            }
            end = riff + 8;
            largeData = le64(m_bytes + 28);
        }
        if (end < 12 || end > m_end) {
            return false;
        }
        bool format = false;
        size_t audio = 0;
        size_t audioSize = 0;
        for (size_t offset = 12; offset < end;) {
            if (end - offset < 8) {
                return false;
            }
            const uint8_t* p = m_bytes + offset;
            uint64_t length = wave32(p + 4);
            if (large && length == UINT32_MAX && !memcmp(p, "data", 4)) {
                length = largeData;
            }
            offset += 8;
            if (length > end - offset || (length & 1) > end - offset - length) {
                return false;
            }
            size_t size = static_cast<size_t>(length);
            if (!memcmp(p, "fmt ", 4) && !format) {
                if (size < 16) {
                    return false;
                }
                p = m_bytes + offset;
                unsigned tag = wave16(p);
                m_config.channels = wave16(p + 2);
                m_config.rate = wave32(p + 4);
                m_config.blockAlign = wave16(p + 12);
                m_config.bits = wave16(p + 14);
                if (tag == 0xfffe) {
                    const uint8_t guid[] = { 0, 0,    0x10, 0,    0x80, 0,
                                             0, 0xaa, 0,    0x38, 0x9b, 0x71 };
                    if (m_bigWave || size < 40 || le16(p + 16) < 22 ||
                        memcmp(p + 28, guid, sizeof(guid))) {
                        return false;
                    }
                    tag = le32(p + 24);
                } else if (size > 16) {
                    if (size < 18 || wave16(p + 16) > size - 18 ||
                        size > 1024 * 1024) {
                        return false;
                    }
                    m_config.extra.assign(p + 18, p + 18 + wave16(p + 16));
                }
                switch (tag) {
                case 1:
                    m_config.codec = Codec::PCM;
                    break;
                case 3:
                    m_config.codec = Codec::PCM;
                    m_config.floating = true;
                    break;
                case 6:
                    m_config.codec = Codec::ALaw;
                    break;
                case 7:
                    m_config.codec = Codec::MuLaw;
                    break;
                case 2:
                    m_config.codec = Codec::MSADPCM;
                    break;
                case 17:
                    m_config.codec = Codec::IMAADPCM;
                    break;
                case 0x55:
                    m_config.codec = Codec::MP3;
                    m_config.parseFrames = true;
                    // MPEG WAVE blockAlign describes codec blocking, not
                    // independent fixed-size byte packets.
                    m_config.blockAlign = 0;
                    break;
                default:
                    return false;
                }
                format = true;
            } else if (!memcmp(p, "data", 4) && !audio) {
                audio = offset;
                audioSize = size;
            } else if (!memcmp(p, "fact", 4) && size >= 4) {
                m_config.samples = wave32(m_bytes + offset);
            }
            offset += size + (size & 1);
        }
        m_offset = audio;
        m_end = audio + audioSize;
        // Preserve complete decoded ADPCM blocks; WAVE fact may describe
        // the unpadded source rather than the decoder's output sample count.
        if (m_config.codec == Codec::IMAADPCM ||
            m_config.codec == Codec::MSADPCM || m_config.codec == Codec::MP3) {
            if (m_bigWave) {
                return false;
            }
            m_config.samples = 0;
        }
        return format && audio && audioSize && m_config.channels &&
               m_config.channels <= 32 && m_config.rate &&
               (m_config.parseFrames ||
                (m_config.blockAlign && !(audioSize % m_config.blockAlign)));
    }

    uint32_t wave16(const uint8_t* p) const
    {
        return m_bigWave ? (uint32_t(p[0]) << 8) | p[1] : le16(p);
    }
    uint32_t wave32(const uint8_t* p) const
    {
        return m_bigWave ? be32(p) : le32(p);
    }

    bool readOggPage()
    {
        if (m_offset == m_end) {
            return false;
        }
        if (m_end - m_offset < 27) {
            return fail();
        }
        const uint8_t* p = m_bytes + m_offset;
        size_t segments = p[26];
        if (memcmp(p, "OggS", 4) || p[4] || (p[5] & ~7) ||
            segments > m_end - m_offset - 27) {
            return fail();
        }
        size_t size = 27 + segments;
        for (size_t i = 0; i < segments; i++) {
            size += p[27 + i];
        }
        if (size > m_end - m_offset) {
            return fail();
        }
        // RFC 3533 section 6: validate the entire page before trusting lacing.
        uint32_t crc = 0;
        for (size_t i = 0; i < size; i++) {
            crc ^= uint32_t(i >= 22 && i < 26 ? 0 : p[i]) << 24;
            for (unsigned bit = 0; bit < 8; bit++) {
                crc = (crc << 1) ^ ((crc & 0x80000000) ? 0x04c11db7 : 0);
            }
        }
        if (crc != le32(p + 22)) {
            return fail();
        }
        m_page = p;
        m_pageSegments = segments;
        m_segment = 0;
        m_payload = p + 27 + segments;
        m_offset += size;
        return true;
    }

    bool oggPacket(Packet& packet, bool metadata = false)
    {
        m_scratch.clear();
        bool continued = false;
        for (;;) {
            if (m_segment == m_pageSegments) {
                do {
                    if (!readOggPage()) {
                        if (continued) {
                            fail();
                        }
                        return false;
                    }
                } while (le32(m_page + 14) != m_serial || !m_pageSegments);
                if (bool(m_page[5] & 1) != continued) {
                    return fail();
                }
            }
            size_t size = m_page[27 + m_segment++];
            // Codec headers/packets are bounded independently of input size.
            if (!metadata && m_scratch.size() > 1024 * 1024 - size) {
                return fail();
            }
            size_t keep =
                metadata ? std::min<size_t>(size, 32 - m_scratch.size()) : size;
            m_scratch.insert(m_scratch.end(), m_payload, m_payload + keep);
            m_payload += size;
            if (size < 255) {
                packet = { m_scratch.data(), m_scratch.size() };
                bool last = true;
                for (size_t i = m_segment; i < m_pageSegments; i++) {
                    if (m_page[27 + i] < 255) {
                        last = false;
                        break;
                    }
                }
                if (last) {
                    packet.granule = static_cast<int64_t>(le64(m_page + 6));
                    packet.endOfStream = m_page[5] & 4;
                }
                if (!metadata && m_config.codec == Codec::Opus &&
                    !m_opusOriginKnown) {
                    if (!packet.size) {
                        return fail();
                    }
                    unsigned config = packet.data[0] >> 3;
                    unsigned samples = config >= 16   ? 120u << (config & 3)
                                       : config >= 12 ? 480u << (config & 1)
                                       : (config & 3) == 3
                                           ? 2880
                                           : 480u << (config & 3);
                    unsigned code = packet.data[0] & 3;
                    if (code == 3 && packet.size < 2) {
                        return fail();
                    }
                    unsigned count = code == 0   ? 1
                                     : code == 3 ? packet.data[1] & 63
                                                 : 2;
                    if (!count || samples * count > 5760) {
                        return fail();
                    }
                    m_opusPageSamples += samples * count;
                    if (packet.granule >= 0) {
                        uint64_t granule = packet.granule;
                        if (!packet.endOfStream &&
                            granule < m_opusPageSamples) {
                            return fail();
                        }
                        // RFC 7845 section 4.5: cropped/live captures can
                        // begin at a nonzero granule without changing pre-skip.
                        packet.granuleOrigin = granule > m_opusPageSamples
                                                   ? granule - m_opusPageSamples
                                                   : 0;
                        m_opusOriginKnown = true;
                    }
                }
                return true;
            }
            continued = true;
        }
    }

    bool openOgg()
    {
        m_kind = Kind::Ogg;
        // Xiph Ogg multiplexing: page serials identify independent streams.
        // Keep only their sequence counters, not a packet index or payloads.
        // https://www.xiph.org/ogg/doc/oggstream.html
        struct Stream {
            uint32_t serial;
            uint32_t sequence;
            bool ended;
        };
        std::vector<Stream> streams;
        bool selected = false;
        bool eos = false;
        uint64_t granule = 0;
        while (m_offset < m_end) {
            if (!readOggPage()) {
                return false;
            }
            uint32_t serial = le32(m_page + 14);
            auto stream = std::find_if(
                streams.begin(), streams.end(),
                [serial](const Stream& s) { return s.serial == serial; });
            if (stream == streams.end()) {
                if (!(m_page[5] & 2) || streams.size() == 16) {
                    return false;
                }
                streams.push_back({ serial, 0, false });
                stream = streams.end() - 1;
                size_t payload = 0;
                for (size_t i = 0; i < m_pageSegments; i++) {
                    payload += m_page[27 + i];
                }
                bool audio =
                    (payload >= 19 && !memcmp(m_payload, "OpusHead", 8)) ||
                    (payload >= 30 && m_payload[0] == 1 &&
                     !memcmp(m_payload + 1, "vorbis", 6)) ||
                    (payload >= 51 && m_payload[0] == 0x7f &&
                     !memcmp(m_payload + 1, "FLAC", 4));
                if (!selected && audio) {
                    m_serial = serial;
                    selected = true;
                }
            } else if ((m_page[5] & 2) || stream->ended) {
                return false;
            }
            if (le32(m_page + 18) != stream->sequence++) {
                return false;
            }
            stream->ended = m_page[5] & 4;
            if (selected && serial == m_serial && stream->ended) {
                eos = true;
                granule = le64(m_page + 6);
            }
            if (eos && std::all_of(streams.begin(), streams.end(),
                                   [](const Stream& s) { return s.ended; })) {
                if (m_offset < m_end) {
                    m_following = m_bytes + m_offset;
                    m_followingLength = m_end - m_offset;
                    m_end = m_offset;
                }
                break;
            }
        }
        if (!eos || granule > INT64_MAX) {
            return false;
        }
        m_offset = 0;
        m_config.oggGranules = true;
        m_pageSegments = m_segment = 0;
        Packet packet;
        if (!oggPacket(packet)) {
            return false;
        }
        if (packet.size >= 19 && !memcmp(packet.data, "OpusHead", 8)) {
            m_config.codec = Codec::Opus;
            m_config.rate = 48000;
            m_config.channels = packet.data[9];
            m_config.skip = le16(packet.data + 10);
            m_config.extra.assign(packet.data, packet.data + packet.size);
            if (!oggPacket(packet, true) || packet.size < 8 ||
                memcmp(packet.data, "OpusTags", 8)) {
                return false;
            }
            if (granule < m_config.skip) {
                return false;
            }
            m_config.samples = granule - m_config.skip;
        } else if (packet.size == 30 && packet.data[0] == 1 &&
                   !memcmp(packet.data + 1, "vorbis", 6)) {
            m_config.codec = Codec::Vorbis;
            m_config.channels = packet.data[11];
            m_config.rate = le32(packet.data + 12);
            std::vector<uint8_t> headers[3];
            headers[0].assign(packet.data, packet.data + packet.size);
            for (unsigned i = 1; i < 3; i++) {
                if (!oggPacket(packet, i == 1) ||
                    packet.size < (i == 1 ? 16u : 7u) ||
                    packet.data[0] != 1 + i * 2 ||
                    memcmp(packet.data + 1, "vorbis", 6)) {
                    return false;
                }
                if (i == 1) {
                    // Comments (including cover art) do not affect PCM.
                    // Supply an empty legal comment header to the codec
                    // instead of retaining arbitrarily large metadata.
                    headers[i] = { 3, 'v', 'o', 'r', 'b', 'i', 's', 0,
                                   0, 0,   0,   0,   0,   0,   0,   1 };
                } else {
                    headers[i].assign(packet.data, packet.data + packet.size);
                }
            }
            m_config.extra.push_back(2);
            for (unsigned i = 0; i < 2; i++) {
                size_t size = headers[i].size();
                while (size >= 255) {
                    m_config.extra.push_back(255);
                    size -= 255;
                }
                m_config.extra.push_back(static_cast<uint8_t>(size));
            }
            for (auto& header : headers) {
                m_config.extra.insert(m_config.extra.end(), header.begin(),
                                      header.end());
            }
            m_config.samples = granule;
        } else if (packet.size >= 51 && packet.data[0] == 0x7f &&
                   !memcmp(packet.data + 1, "FLAC", 4) &&
                   !memcmp(packet.data + 9, "fLaC", 4)) {
            m_config.codec = Codec::FLAC;
            m_config.extra.assign(packet.data + 17, packet.data + 51);
            const uint8_t* info = m_config.extra.data();
            m_config.rate = be24(info + 10) >> 4;
            m_config.channels = ((info[12] >> 1) & 7) + 1;
            m_config.samples = granule;
            unsigned headers = (unsigned(packet.data[7]) << 8) | packet.data[8];
            if (headers > 16) {
                return false;
            }
            while (headers--) {
                if (!oggPacket(packet, true)) {
                    return false;
                }
            }
        } else {
            return false;
        }
        return m_config.channels && m_config.channels <= 32 && m_config.rate;
    }

    Configuration m_config;
    const uint8_t* m_following{ nullptr };
    size_t m_followingLength{ 0 };
    unsigned m_chains{ 1 };
    uint64_t m_opusPageSamples{ 0 };
    bool m_opusOriginKnown{ false };
    bool m_fragmented{ false };
    uint32_t m_trackID{ 0 };
    uint32_t m_fragmentDefaultSize{ 0 };
    size_t m_fragmentOffset{ 0 };
    std::vector<Run> m_runs;
    size_t m_runIndex{ 0 };
    uint64_t m_webmTrack{ 0 };
    size_t m_clusterEnd{ 0 };
    std::vector<size_t> m_laces;
    size_t m_laceIndex{ 0 };
    const uint8_t* m_laceData{ nullptr };
    uint32_t m_laceDiscard{ 0 };
    Box m_stsz;
    Box m_stsd;
    uint32_t m_descriptionIndex{ 1 };
    unsigned m_sampleBits{ 32 };
    Box m_stsc;
    Box m_stco;
    uint32_t m_stscCount{ 0 };
    uint32_t m_stscIndex{ 0 };
    uint32_t m_chunkCount{ 0 };
    uint32_t m_chunkIndex{ 0 };
    uint32_t m_chunkRemaining{ 0 };
    size_t m_chunkOffset{ 0 };
    uint32_t m_sampleCount{ 0 };
    uint32_t m_sampleIndex{ 0 };
    uint32_t m_uniformSize{ 0 };
    bool m_largeChunks{ false };
    bool m_bigWave{ false };
    const uint8_t* m_bytes{ nullptr };
    size_t m_offset{ 0 };
    size_t m_end{ 0 };
    Kind m_kind{ Kind::Raw };
    bool m_failed{ false };
    const uint8_t* m_page{ nullptr };
    const uint8_t* m_payload{ nullptr };
    size_t m_segment{ 0 };
    size_t m_pageSegments{ 0 };
    uint32_t m_serial{ 0 };
    std::vector<uint8_t> m_scratch;
};

} // namespace Starfish
#endif
