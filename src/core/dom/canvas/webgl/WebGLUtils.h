/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarfishWebGLUtils__
#define __StarfishWebGLUtils__

#include <string>
#include "platform/canvas/gl/GLTypes.h"
#include "core/util/String.h"
#include "StarfishBase.h" // Optional

namespace Starfish {

struct CaseInsensitiveHash {
    size_t operator()(const std::string& str) const
    {
        size_t hash = 0;
        for (unsigned char c : str) {
            hash = hash * 31 + std::tolower(c);
        }
        return hash;
    }
};

struct CaseInsensitiveEqual {
    bool operator()(const std::string& a, const std::string& b) const
    {
        return StringUtils::equalsIgnoreCase(a, b);
    }
};

// Block layout of a compressed texture format, and the WebGL extension that
// must be enabled on a context before the format is accepted there.
struct CompressedFormatInfo {
    const char* extension;
    uint8_t blockWidth;
    uint8_t blockHeight;
    uint8_t bytesPerBlock;
};

class Pixel {
public:
    static Optional<CompressedFormatInfo> compressedFormatInfo(GLenum format);
    static bool isInternalFormatValid(GLint internalFormat, GLenum format,
                                      GLenum type = 0, int webGLVersion = 1);
    static bool isKnownInternalFormatWebGL2(GLint internalFormat);
    static bool isKnownFormatWebGL2(GLenum format);
    static bool isKnownTypeWebGL2(GLenum type);
    static bool isSizedInternalFormat(GLenum internalformat);
    static size_t getBytesPerPixel(GLenum format, GLenum type,
                                   int webGLVersion = 1);
    static bool isTwoBytesPerPixel(GLenum type);
    static GLushort makePixel5551(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static GLushort makePixel4444(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static GLushort makePixel565(uint8_t r, uint8_t g, uint8_t b);
};

std::string glValueString(uint32_t value);

} // namespace Starfish

#endif
