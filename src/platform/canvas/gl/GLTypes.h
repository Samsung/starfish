/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishGLTypes__
#define __StarfishGLTypes__

#include <stdint.h>
#include <limits>

typedef void GLvoid;
typedef char GLchar;
typedef uint32_t GLenum;
typedef uint8_t GLboolean;
typedef uint32_t GLbitfield;
typedef int8_t GLbyte;
typedef int16_t GLshort;
typedef int32_t GLint;
typedef int32_t GLsizei;
typedef uint8_t GLubyte;
typedef uint16_t GLushort;
typedef uint32_t GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef int32_t GLfixed;
typedef struct __GLsync *GLsync;
#if !defined(_EVAS_GL_H) || !defined(STARFISH_TIZEN)
typedef int64_t GLint64;
typedef uint64_t GLuint64;
#endif

// Must match Khronos khrplatform.h's khronos_intptr_t/khronos_ssize_t
// exactly (basic type, not just size): real GLES2/gl2.h -- pulled in
// alongside this header on STARFISH_WINDOWS_ANGLE builds -- typedefs
// GLintptr/GLsizeiptr from those, and MSVC errors (C2371) on a redeclaration
// that merely matches in size, e.g. ptrdiff_t on 32-bit Windows is `int`,
// not the `long int` khronos_ssize_t uses there.
#if defined(STARFISH_WINDOWS) && defined(_WIN64)
typedef long long int GLintptr;
typedef long long int GLsizeiptr;
#else
typedef long int GLintptr;
typedef long int GLsizeiptr;
#endif

// WebGL's GLintptr and GLsizeiptr are Web IDL `long long` on every platform,
// unlike the pointer-sized GL types above (32-bit on armv7). Values from
// script are held in these and range-checked before being narrowed to the
// GL types, so a large or negative value is never silently truncated.
typedef int64_t WebGLintptr;
typedef int64_t WebGLsizeiptr;

// A value outside the GL type's range cannot address data in any buffer the
// GL could allocate on this platform, so clamping it keeps the GL's own
// validation result (INVALID_VALUE for a range past the buffer or a
// negative value, OUT_OF_MEMORY for an unallocatable size) where truncation
// could turn it into a valid in-range value.
inline GLintptr clampToGLintptr(WebGLintptr value)
{
    if (value > std::numeric_limits<GLintptr>::max()) {
        return std::numeric_limits<GLintptr>::max();
    }
    if (value < std::numeric_limits<GLintptr>::min()) {
        return std::numeric_limits<GLintptr>::min();
    }
    return static_cast<GLintptr>(value);
}

inline GLsizeiptr clampToGLsizeiptr(WebGLsizeiptr value)
{
    return static_cast<GLsizeiptr>(clampToGLintptr(value));
}

#ifndef GL_NONE
#define GL_NONE 0
#endif

#endif
