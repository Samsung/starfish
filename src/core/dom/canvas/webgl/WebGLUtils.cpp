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
#include "core/modules/worker/util/Trace.h"
#include "core/util/String.h"
#include <unordered_map>

namespace Starfish {

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

bool Pixel::isTwoBytesPerPixel(GLenum type)
{
    return (type == GL_UNSIGNED_SHORT_5_5_5_1 ||
            type == GL_UNSIGNED_SHORT_4_4_4_4 ||
            type == GL_UNSIGNED_SHORT_5_6_5);
}

GLushort Pixel::makePixel5551(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (((r >> 3) & 0x1F) << 11) | (((g >> 3) & 0x1F) << 6) |
           (((b >> 3) & 0x1F) << 1) | (a & 0x01);
}

GLushort Pixel::makePixel4444(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    r = (r > 15) ? (r >> 4) : r;
    g = (g > 15) ? (g >> 4) : g;
    b = (b > 15) ? (b >> 4) : b;
    a = (a > 15) ? (a >> 4) : a;

    return ((r & 0xF) << 12) | ((g & 0xF) << 8) | ((b & 0xF) << 4) | (a & 0xF);
}

std::string glValueString(uint32_t value)
{
#if defined(ENABLE_TRACE)
#define V(n) \
    case n:  \
        return StringUtils::formatString("0x%04X ", value) + #n;

    switch (value) {
        V(GL_DEPTH_BUFFER_BIT)
        V(GL_STENCIL_BUFFER_BIT)
        V(GL_COLOR_BUFFER_BIT)
        V(GL_SRC_COLOR)
        V(GL_ONE_MINUS_SRC_COLOR)
        V(GL_SRC_ALPHA)
        V(GL_ONE_MINUS_SRC_ALPHA)
        V(GL_DST_ALPHA)
        V(GL_ONE_MINUS_DST_ALPHA)
        V(GL_DST_COLOR)
        V(GL_ONE_MINUS_DST_COLOR)
        V(GL_SRC_ALPHA_SATURATE)
        V(GL_FUNC_ADD)
        V(GL_BLEND_EQUATION_ALPHA)
        V(GL_FUNC_SUBTRACT)
        V(GL_FUNC_REVERSE_SUBTRACT)
        V(GL_BLEND_DST_RGB)
        V(GL_BLEND_SRC_RGB)
        V(GL_BLEND_DST_ALPHA)
        V(GL_BLEND_SRC_ALPHA)
        V(GL_CONSTANT_COLOR)
        V(GL_ONE_MINUS_CONSTANT_COLOR)
        V(GL_CONSTANT_ALPHA)
        V(GL_ONE_MINUS_CONSTANT_ALPHA)
        V(GL_BLEND_COLOR)
        V(GL_ARRAY_BUFFER)
        V(GL_ELEMENT_ARRAY_BUFFER)
        V(GL_ARRAY_BUFFER_BINDING)
        V(GL_ELEMENT_ARRAY_BUFFER_BINDING)
        V(GL_STREAM_DRAW)
        V(GL_STATIC_DRAW)
        V(GL_DYNAMIC_DRAW)
        V(GL_BUFFER_SIZE)
        V(GL_BUFFER_USAGE)
        V(GL_CURRENT_VERTEX_ATTRIB)
        V(GL_FRONT)
        V(GL_BACK)
        V(GL_FRONT_AND_BACK)
        V(GL_TEXTURE_2D)
        V(GL_CULL_FACE)
        V(GL_BLEND)
        V(GL_DITHER)
        V(GL_STENCIL_TEST)
        V(GL_DEPTH_TEST)
        V(GL_SCISSOR_TEST)
        V(GL_POLYGON_OFFSET_FILL)
        V(GL_SAMPLE_ALPHA_TO_COVERAGE)
        V(GL_SAMPLE_COVERAGE)
        V(GL_INVALID_ENUM)
        V(GL_INVALID_VALUE)
        V(GL_INVALID_OPERATION)
        V(GL_OUT_OF_MEMORY)
        V(GL_CW)
        V(GL_CCW)
        V(GL_LINE_WIDTH)
        V(GL_ALIASED_POINT_SIZE_RANGE)
        V(GL_ALIASED_LINE_WIDTH_RANGE)
        V(GL_CULL_FACE_MODE)
        V(GL_FRONT_FACE)
        V(GL_DEPTH_RANGE)
        V(GL_DEPTH_WRITEMASK)
        V(GL_DEPTH_CLEAR_VALUE)
        V(GL_DEPTH_FUNC)
        V(GL_STENCIL_CLEAR_VALUE)
        V(GL_STENCIL_FUNC)
        V(GL_STENCIL_FAIL)
        V(GL_STENCIL_PASS_DEPTH_FAIL)
        V(GL_STENCIL_PASS_DEPTH_PASS)
        V(GL_STENCIL_REF)
        V(GL_STENCIL_VALUE_MASK)
        V(GL_STENCIL_WRITEMASK)
        V(GL_STENCIL_BACK_FUNC)
        V(GL_STENCIL_BACK_FAIL)
        V(GL_STENCIL_BACK_PASS_DEPTH_FAIL)
        V(GL_STENCIL_BACK_PASS_DEPTH_PASS)
        V(GL_STENCIL_BACK_REF)
        V(GL_STENCIL_BACK_VALUE_MASK)
        V(GL_STENCIL_BACK_WRITEMASK)
        V(GL_VIEWPORT)
        V(GL_SCISSOR_BOX)
        V(GL_COLOR_CLEAR_VALUE)
        V(GL_COLOR_WRITEMASK)
        V(GL_UNPACK_ALIGNMENT)
        V(GL_PACK_ALIGNMENT)
        V(GL_MAX_TEXTURE_SIZE)
        V(GL_MAX_VIEWPORT_DIMS)
        V(GL_SUBPIXEL_BITS)
        V(GL_RED_BITS)
        V(GL_GREEN_BITS)
        V(GL_BLUE_BITS)
        V(GL_ALPHA_BITS)
        V(GL_DEPTH_BITS)
        V(GL_STENCIL_BITS)
        V(GL_POLYGON_OFFSET_UNITS)
        V(GL_POLYGON_OFFSET_FACTOR)
        V(GL_TEXTURE_BINDING_2D)
        V(GL_SAMPLE_BUFFERS)
        V(GL_SAMPLES)
        V(GL_SAMPLE_COVERAGE_VALUE)
        V(GL_SAMPLE_COVERAGE_INVERT)
        V(GL_NUM_COMPRESSED_TEXTURE_FORMATS)
        V(GL_COMPRESSED_TEXTURE_FORMATS)
        V(GL_DONT_CARE)
        V(GL_FASTEST)
        V(GL_NICEST)
        V(GL_GENERATE_MIPMAP_HINT)
        V(GL_BYTE)
        V(GL_UNSIGNED_BYTE)
        V(GL_SHORT)
        V(GL_UNSIGNED_SHORT)
        V(GL_INT)
        V(GL_UNSIGNED_INT)
        V(GL_FLOAT)
        V(GL_FIXED)
        V(GL_DEPTH_COMPONENT)
        V(GL_ALPHA)
        V(GL_RGB)
        V(GL_RGBA)
        V(GL_LUMINANCE)
        V(GL_LUMINANCE_ALPHA)
        V(GL_UNSIGNED_SHORT_4_4_4_4)
        V(GL_UNSIGNED_SHORT_5_5_5_1)
        V(GL_UNSIGNED_SHORT_5_6_5)
        V(GL_FRAGMENT_SHADER)
        V(GL_VERTEX_SHADER)
        V(GL_MAX_VERTEX_ATTRIBS)
        V(GL_MAX_VERTEX_UNIFORM_VECTORS)
        V(GL_MAX_VARYING_VECTORS)
        V(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)
        V(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS)
        V(GL_MAX_TEXTURE_IMAGE_UNITS)
        V(GL_MAX_FRAGMENT_UNIFORM_VECTORS)
        V(GL_SHADER_TYPE)
        V(GL_DELETE_STATUS)
        V(GL_LINK_STATUS)
        V(GL_VALIDATE_STATUS)
        V(GL_ATTACHED_SHADERS)
        V(GL_ACTIVE_UNIFORMS)
        V(GL_ACTIVE_UNIFORM_MAX_LENGTH)
        V(GL_ACTIVE_ATTRIBUTES)
        V(GL_ACTIVE_ATTRIBUTE_MAX_LENGTH)
        V(GL_SHADING_LANGUAGE_VERSION)
        V(GL_CURRENT_PROGRAM)
        V(GL_NEVER)
        V(GL_LESS)
        V(GL_EQUAL)
        V(GL_LEQUAL)
        V(GL_GREATER)
        V(GL_NOTEQUAL)
        V(GL_GEQUAL)
        V(GL_ALWAYS)
        V(GL_KEEP)
        V(GL_REPLACE)
        V(GL_INCR)
        V(GL_DECR)
        V(GL_INVERT)
        V(GL_INCR_WRAP)
        V(GL_DECR_WRAP)
        V(GL_VENDOR)
        V(GL_RENDERER)
        V(GL_VERSION)
        V(GL_EXTENSIONS)
        V(GL_NEAREST)
        V(GL_LINEAR)
        V(GL_NEAREST_MIPMAP_NEAREST)
        V(GL_LINEAR_MIPMAP_NEAREST)
        V(GL_NEAREST_MIPMAP_LINEAR)
        V(GL_LINEAR_MIPMAP_LINEAR)
        V(GL_TEXTURE_MAG_FILTER)
        V(GL_TEXTURE_MIN_FILTER)
        V(GL_TEXTURE_WRAP_S)
        V(GL_TEXTURE_WRAP_T)
        V(GL_TEXTURE)
        V(GL_TEXTURE_CUBE_MAP)
        V(GL_TEXTURE_BINDING_CUBE_MAP)
        V(GL_TEXTURE_CUBE_MAP_POSITIVE_X)
        V(GL_TEXTURE_CUBE_MAP_NEGATIVE_X)
        V(GL_TEXTURE_CUBE_MAP_POSITIVE_Y)
        V(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y)
        V(GL_TEXTURE_CUBE_MAP_POSITIVE_Z)
        V(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
        V(GL_MAX_CUBE_MAP_TEXTURE_SIZE)
        V(GL_TEXTURE0)
        V(GL_TEXTURE1)
        V(GL_TEXTURE2)
        V(GL_TEXTURE3)
        V(GL_TEXTURE4)
        V(GL_TEXTURE5)
        V(GL_TEXTURE6)
        V(GL_TEXTURE7)
        V(GL_TEXTURE8)
        V(GL_TEXTURE9)
        V(GL_TEXTURE10)
        V(GL_TEXTURE11)
        V(GL_TEXTURE12)
        V(GL_TEXTURE13)
        V(GL_TEXTURE14)
        V(GL_TEXTURE15)
        V(GL_TEXTURE16)
        V(GL_TEXTURE17)
        V(GL_TEXTURE18)
        V(GL_TEXTURE19)
        V(GL_TEXTURE20)
        V(GL_TEXTURE21)
        V(GL_TEXTURE22)
        V(GL_TEXTURE23)
        V(GL_TEXTURE24)
        V(GL_TEXTURE25)
        V(GL_TEXTURE26)
        V(GL_TEXTURE27)
        V(GL_TEXTURE28)
        V(GL_TEXTURE29)
        V(GL_TEXTURE30)
        V(GL_TEXTURE31)
        V(GL_ACTIVE_TEXTURE)
        V(GL_REPEAT)
        V(GL_CLAMP_TO_EDGE)
        V(GL_MIRRORED_REPEAT)
        V(GL_FLOAT_VEC2)
        V(GL_FLOAT_VEC3)
        V(GL_FLOAT_VEC4)
        V(GL_INT_VEC2)
        V(GL_INT_VEC3)
        V(GL_INT_VEC4)
        V(GL_BOOL)
        V(GL_BOOL_VEC2)
        V(GL_BOOL_VEC3)
        V(GL_BOOL_VEC4)
        V(GL_FLOAT_MAT2)
        V(GL_FLOAT_MAT3)
        V(GL_FLOAT_MAT4)
        V(GL_SAMPLER_2D)
        V(GL_SAMPLER_CUBE)
        V(GL_VERTEX_ATTRIB_ARRAY_ENABLED)
        V(GL_VERTEX_ATTRIB_ARRAY_SIZE)
        V(GL_VERTEX_ATTRIB_ARRAY_STRIDE)
        V(GL_VERTEX_ATTRIB_ARRAY_TYPE)
        V(GL_VERTEX_ATTRIB_ARRAY_NORMALIZED)
        V(GL_VERTEX_ATTRIB_ARRAY_POINTER)
        V(GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING)
        V(GL_IMPLEMENTATION_COLOR_READ_TYPE)
        V(GL_IMPLEMENTATION_COLOR_READ_FORMAT)
        V(GL_COMPILE_STATUS)
        V(GL_INFO_LOG_LENGTH)
        V(GL_SHADER_SOURCE_LENGTH)
        V(GL_SHADER_COMPILER)
        V(GL_SHADER_BINARY_FORMATS)
        V(GL_NUM_SHADER_BINARY_FORMATS)
        V(GL_LOW_FLOAT)
        V(GL_MEDIUM_FLOAT)
        V(GL_HIGH_FLOAT)
        V(GL_LOW_INT)
        V(GL_MEDIUM_INT)
        V(GL_HIGH_INT)
        V(GL_FRAMEBUFFER)
        V(GL_RENDERBUFFER)
        V(GL_RGBA4)
        V(GL_RGB5_A1)
        V(GL_RGB565)
        V(GL_DEPTH_COMPONENT16)
        V(GL_STENCIL_INDEX8)
        V(GL_RENDERBUFFER_WIDTH)
        V(GL_RENDERBUFFER_HEIGHT)
        V(GL_RENDERBUFFER_INTERNAL_FORMAT)
        V(GL_RENDERBUFFER_RED_SIZE)
        V(GL_RENDERBUFFER_GREEN_SIZE)
        V(GL_RENDERBUFFER_BLUE_SIZE)
        V(GL_RENDERBUFFER_ALPHA_SIZE)
        V(GL_RENDERBUFFER_DEPTH_SIZE)
        V(GL_RENDERBUFFER_STENCIL_SIZE)
        V(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE)
        V(GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME)
        V(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL)
        V(GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE)
        V(GL_COLOR_ATTACHMENT0)
        V(GL_DEPTH_ATTACHMENT)
        V(GL_STENCIL_ATTACHMENT)
        V(GL_FRAMEBUFFER_COMPLETE)
        V(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
        V(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
        V(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
        V(GL_FRAMEBUFFER_UNSUPPORTED)
        V(GL_FRAMEBUFFER_BINDING)
        V(GL_RENDERBUFFER_BINDING)
        V(GL_MAX_RENDERBUFFER_SIZE)
        V(GL_INVALID_FRAMEBUFFER_OPERATION)
        // gl2ext
        V(GL_BGRA_EXT)
    default:
        break;
    };
#undef V
#endif
    return StringUtils::formatString("0x%04X", value);
}

} // namespace Starfish

#endif
