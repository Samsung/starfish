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

#ifndef __StarfishShellConfig__
#define __StarfishShellConfig__

// Define the SHELL macros using define given from CMAKE.
#if defined(STARFISH_EFL_CAIRO) || defined(STARFISH_EFL_CAIRO_GL)
#define SHELL_ENABLE_ELEMENTARY
#endif

#if defined(STARFISH_EFL_CAIRO_GL)
#define SHELL_ENABLE_ELEMENTARY_GL
#endif

#if defined(STARFISH_EFL_HEADLESS)
#define SHELL_ENABLE_ECORE
#define SHELL_ENABLE_HEADLESS
#endif

#if defined(STARFISH_GLFW_CAIRO_GL) || defined(STARFISH_X11_CAIRO_GL) || \
    defined(STARFISH_DALI) || defined(STARFISH_ANDROID) ||               \
    defined(STARFISH_WINDOWS_UWP) || defined(STARFISH_FLUTTER)
#define SHELL_ENABLE_UV
#endif

#if defined(STARFISH_GLFW_CAIRO_GL) || defined(STARFISH_X11_CAIRO_GL)
#define SHELL_ENABLE_WINDOWLESS
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || \
    defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define SHELL_X86_64
#endif

#if defined(STARFISH_ENABLE_TEST) && defined(SHELL_X86_64)
#define SHELL_ENABLE_BACKTRACE
#endif

#if defined(STARFISH_ENABLE_TEST)
#define SHELL_ENABLE_TEST
#endif

#if defined(STARFISH_ANDROID)
#define SHELL_ANDROID
#endif

#if defined(STARFISH_WINDOWS)
#define SHELL_WINDOWS
#endif

#if defined(STARFISH_TIZEN)
#define SHELL_TIZEN
#endif

#if defined(STARFISH_ENABLE_TRANSPARENT_WINDOW)
#define SHELL_ENABLE_TRANSPARENT_WINDOW
#endif

#endif
