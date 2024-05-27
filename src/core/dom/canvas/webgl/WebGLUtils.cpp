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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLUtils.h"
#include "platform/canvas/gl/IncludeGL.h"

namespace Starfish {

const char* webglErrorString(unsigned int code)
{
    switch (code) {
    case GL_NO_ERROR:
        return "NO_ERROR";
    case GL_INVALID_ENUM:
        return "INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    default:
        return "UNDEFINED_ERROR";
    }
    return "";
}

size_t Pixel::getBytesPerPixel(GLenum format, GLenum type)
{
    // Format      Type                Bytes per Pixel
    // ------------------------------------------------
    // RGBA        UNSIGNED_BYTE            4
    // RGB         UNSIGNED_BYTE            3
    // RGBA        UNSIGNED_SHORT_4_4_4_4   2
    // RGBA        UNSIGNED_SHORT_5_5_5_1   2
    // RGB         UNSIGNED_SHORT_5_6_5     2
    // LUMINANCE_ALPHA  UNSIGNED_BYTE       2
    // LUMINANCE   UNSIGNED_BYTE            1
    // ALPHA       UNSIGNED_BYTE            1
    //
    // Refs: Table 3.4: Valid pixel format and type combinations.
    // https://registry.khronos.org/OpenGL/specs/es/2.0/es_full_spec_2.0.pdf

    if (type == GL_UNSIGNED_BYTE || type == GL_FLOAT) {
        if (format == GL_RGBA || format == GL_BGRA_EXT) {
            return 4;
        } else if (format == GL_RGB) {
            return 3;
        } else if (format == GL_LUMINANCE_ALPHA) {
            return 2;
        } else if (format == GL_LUMINANCE || format == GL_ALPHA) {
            return 1;
        }
    } else if (type == GL_UNSIGNED_SHORT_4_4_4_4) {
        if (format == GL_RGBA || format == GL_BGRA_EXT) {
            return 2;
        }
    } else if (type == GL_UNSIGNED_SHORT_5_5_5_1) {
        if (format == GL_RGBA || format == GL_BGRA_EXT) {
            return 2;
        }
    } else if (type == GL_UNSIGNED_SHORT_5_6_5) {
        if (format == GL_RGB) {
            return 2;
        }
    }

    STARFISH_UNIMPLEMENTED("format: 0x%04X, type: 0x%04X", format, type);
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

} // namespace Starfish

#endif
