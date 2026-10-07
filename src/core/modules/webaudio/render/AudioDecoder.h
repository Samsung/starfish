/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 */

#if defined(STARFISH_ENABLE_WEBAUDIO)

#ifndef __StarfishAudioDecoder__
#define __StarfishAudioDecoder__

#include <cstddef>
#include <cstdint>

namespace Starfish {

class AudioBufferData;

// Non-WAVE input returns false. This checks container bounds, not codecs,
// so valid compressed WAVE may still be handled by the FFmpeg backend.
bool isMalformedWaveAudio(const uint8_t* bytes, size_t length);

// Returns an owned PCM buffer or nullptr for invalid/unsupported input.
// The parser only reads within [bytes, bytes + length), including on corrupt
// RIFF chunk lengths supplied by web content.
AudioBufferData* decodeWaveAudio(const uint8_t* bytes, size_t length,
                                 double targetSampleRate);

// Uses FFmpeg on Linux FFmpeg builds and on Tizen; returns nullptr for formats
// that cannot be decoded or exceed the AudioBuffer allocation limit. Blocks
// the calling thread until decoding ends, so call it off the main thread.
AudioBufferData* decodeCompressedAudio(const uint8_t* bytes, size_t length,
                                       double targetSampleRate);

} // namespace Starfish

#endif
#endif
