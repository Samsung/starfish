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

#ifndef __StarfishIncludeGL__
#define __StarfishIncludeGL__

#if !defined(STARFISH_DALI) && !defined(STARFISH_EFL_HEADLESS)

#if defined(STARFISH_WINDOWS_UWP)
// Enable function definitions in the GL headers below
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <angle_windowsstore.h>
#elif defined(STARFISH_WINDOWS)
#include <GL/glew.h>
#include <GL/wglew.h>

#include <Windows.h>
#pragma comment(lib, "Opengl32.lib")
#elif defined(STARFISH_ANDROID)
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <android/hardware_buffer.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES3/gl3.h>
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#endif

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT 0x80E1
#endif

#ifndef GL_MAJOR_VERSION
#define GL_MAJOR_VERSION 0x821B
#endif

#ifndef GL_MINOR_VERSION
#define GL_MINOR_VERSION 0x821C
#endif

#ifndef GL_UNPACK_ROW_LENGTH
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#endif

#ifndef GL_UNPACK_SKIP_ROWS
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

#ifndef GL_UNPACK_SKIP_PIXELS
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#endif

#ifndef GL_DEPTH_STENCIL
#define GL_DEPTH_STENCIL 0x84F9
#endif

#ifndef GL_UNSIGNED_INT_24_8
#define GL_UNSIGNED_INT_24_8 0x84FA
#endif

#ifndef GL_TEXTURE_SWIZZLE_R
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#endif
#ifndef GL_TEXTURE_SWIZZLE_G
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#endif
#ifndef GL_TEXTURE_SWIZZLE_B
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#endif
#ifndef GL_TEXTURE_SWIZZLE_A
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#endif
#ifndef TEXTURE_SWIZZLE_RGBA
#define TEXTURE_SWIZZLE_RGBA 0x8E46
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_GREEN
#define GL_GREEN 0x1904
#endif
#ifndef GL_BLUE
#define GL_BLUE 0x1905
#endif
#ifndef GL_ALPHA
#define GL_ALPHA 0x1906
#endif

#endif
#endif
