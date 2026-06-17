// Standalone equivalence test for cycle-32 (perf(demux): scan AVC sample
// NALUs once in MP4PacketGenerator). Reproduces MP4AVCParser::parseNext and
// BOTH the pre-change two-pass algorithm (OLD) and the post-change
// record/replay algorithm (NEW) exactly as in MP4PacketGenerator.cpp, then
// asserts byte-identical AnnexB output / resultSize / hasIdr over many
// randomized length-prefixed NALU samples.
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <utility>
#include <random>

// --- verbatim from MP4PacketGenerator.cpp ---
static const uint8_t NALUTypeIDRSlice = 5;
static const uint8_t NALUTypeSPS = 7;
static const uint8_t NALUTypePPS = 8;
static const uint8_t AnnexBHeader[] = { 0, 0, 0, 1 };
static const int AnnexBHeaderSize = 4;

struct NALUnit {
    uint8_t m_type;
    size_t m_size;
    size_t m_startPos;
};

static bool parseNext(const uint8_t* data, size_t dataSize, size_t& cursor,
                      unsigned char lengthSize, NALUnit& result)
{
    if (dataSize - cursor < lengthSize)
        return false;
    size_t naluSize = 0;
    for (size_t i = 0; i < lengthSize; i++)
        naluSize = (naluSize << 8) + data[cursor + i];
    if (naluSize == 0)
        return false;
    if (naluSize > dataSize - cursor - lengthSize)
        return false;
    result.m_type = data[cursor + lengthSize] & 0x1f;
    result.m_size = naluSize + lengthSize;
    result.m_startPos = cursor;
    cursor += result.m_size;
    return true;
}

struct Out {
    std::vector<uint8_t> data;
    bool hasIdr;
    bool ok;
};

// OLD: two parseNext passes (pre-cycle-32).
static Out genOld(const uint8_t* sample, size_t validLength,
                  unsigned char nalLen, const std::vector<uint8_t>& extra)
{
    Out o;
    o.ok = false;
    o.hasIdr = false;
    bool foundSPS = false, foundPPS = false;
    size_t naluCount = 0, cursor = 0, extraInsertPos = SIZE_MAX;
    bool hasIdr = false;
    while (cursor < validLength) {
        NALUnit nalu;
        if (!parseNext(sample, validLength, cursor, nalLen, nalu))
            return o;
        naluCount++;
        switch (nalu.m_type) {
        case NALUTypeSPS:
            foundSPS = true;
            break;
        case NALUTypePPS:
            foundPPS = true;
            break;
        case NALUTypeIDRSlice:
            if (!foundSPS && !foundPPS)
                extraInsertPos = nalu.m_startPos;
            hasIdr = true;
            break;
        default:
            break;
        }
    }
    if (cursor != validLength)
        return o;
    size_t resultSize = validLength;
    if (nalLen < AnnexBHeaderSize)
        resultSize += (naluCount * (AnnexBHeaderSize - nalLen));
    if (extraInsertPos != SIZE_MAX)
        resultSize += extra.size();
    o.data.resize(resultSize);
    uint8_t* result = o.data.data();
    size_t pos = 0;
    cursor = 0;
    for (size_t i = 0; i < naluCount; i++) {
        NALUnit nalu;
        if (!parseNext(sample, validLength, cursor, nalLen, nalu)) {
            o.data.clear();
            return o;
        }
        if (nalu.m_startPos == extraInsertPos) {
            memcpy(&result[pos], extra.data(), extra.size());
            pos += extra.size();
        }
        memcpy(&result[pos], AnnexBHeader, AnnexBHeaderSize);
        pos += AnnexBHeaderSize;
        size_t dataSize = nalu.m_size - nalLen;
        memcpy(&result[pos], &sample[nalu.m_startPos + nalLen], dataSize);
        pos += dataSize;
    }
    o.hasIdr = hasIdr;
    o.ok = true;
    return o;
}

// NEW: record/replay (post-cycle-32).
static Out genNew(const uint8_t* sample, size_t validLength,
                  unsigned char nalLen, const std::vector<uint8_t>& extra)
{
    Out o;
    o.ok = false;
    o.hasIdr = false;
    bool foundSPS = false, foundPPS = false;
    size_t cursor = 0, extraInsertPos = SIZE_MAX;
    bool hasIdr = false;
    std::vector<std::pair<size_t, size_t>> scratch;
    while (cursor < validLength) {
        NALUnit nalu;
        if (!parseNext(sample, validLength, cursor, nalLen, nalu))
            return o;
        scratch.push_back(std::make_pair(nalu.m_startPos, nalu.m_size));
        switch (nalu.m_type) {
        case NALUTypeSPS:
            foundSPS = true;
            break;
        case NALUTypePPS:
            foundPPS = true;
            break;
        case NALUTypeIDRSlice:
            if (!foundSPS && !foundPPS)
                extraInsertPos = nalu.m_startPos;
            hasIdr = true;
            break;
        default:
            break;
        }
    }
    if (cursor != validLength)
        return o;
    size_t resultSize = validLength;
    if (nalLen < AnnexBHeaderSize)
        resultSize += (scratch.size() * (AnnexBHeaderSize - nalLen));
    if (extraInsertPos != SIZE_MAX)
        resultSize += extra.size();
    o.data.resize(resultSize);
    uint8_t* result = o.data.data();
    size_t pos = 0;
    for (size_t i = 0; i < scratch.size(); i++) {
        size_t naluStart = scratch[i].first, naluSize = scratch[i].second;
        if (naluStart == extraInsertPos) {
            memcpy(&result[pos], extra.data(), extra.size());
            pos += extra.size();
        }
        memcpy(&result[pos], AnnexBHeader, AnnexBHeaderSize);
        pos += AnnexBHeaderSize;
        size_t dataSize = naluSize - nalLen;
        memcpy(&result[pos], &sample[naluStart + nalLen], dataSize);
        pos += dataSize;
    }
    o.hasIdr = hasIdr;
    o.ok = true;
    return o;
}

// Build a random valid length-prefixed NALU sample.
static std::vector<uint8_t> buildSample(std::mt19937& rng, unsigned char nalLen)
{
    std::vector<uint8_t> buf;
    int nNalu = 1 + (rng() % 8);
    for (int n = 0; n < nNalu; n++) {
        size_t payload = 1 + (rng() % 40); // >=1 so naluSize>=1
        size_t naluSize = payload;         // bytes after the length field
        // write length field (big-endian, nalLen bytes)
        for (int b = nalLen - 1; b >= 0; b--)
            buf.push_back((naluSize >> (8 * b)) & 0xFF);
        // first byte carries type in low 5 bits
        uint8_t types[] = { 1, 5, 7, 8, 6, 9 };
        uint8_t t = types[rng() % 6];
        buf.push_back(t);
        for (size_t k = 1; k < payload; k++)
            buf.push_back(rng() & 0xFF);
    }
    return buf;
}

int main()
{
    std::mt19937 rng(12345);
    unsigned char nalLens[] = { 1, 2, 4 };
    long cases = 0, mism = 0;
    for (int iter = 0; iter < 200000; iter++) {
        unsigned char nalLen = nalLens[rng() % 3];
        std::vector<uint8_t> sample = buildSample(rng, nalLen);
        std::vector<uint8_t> extra;
        if (rng() % 2) {
            size_t e = rng() % 20;
            for (size_t k = 0; k < e; k++)
                extra.push_back(rng() & 0xFF);
        }
        Out a = genOld(sample.data(), sample.size(), nalLen, extra);
        Out b = genNew(sample.data(), sample.size(), nalLen, extra);
        cases++;
        if (a.ok != b.ok || a.hasIdr != b.hasIdr || a.data != b.data) {
            mism++;
            if (mism <= 5)
                printf(
                    "MISMATCH iter=%d nalLen=%d ok(%d/%d) hasIdr(%d/%d) "
                    "sz(%zu/%zu)\n",
                    iter, nalLen, a.ok, b.ok, a.hasIdr, b.hasIdr, a.data.size(),
                    b.data.size());
        }
    }
    printf("cases=%ld mismatches=%ld\n", cases, mism);
    return mism ? 1 : 0;
}
