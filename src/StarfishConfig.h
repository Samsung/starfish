/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarfishConfig__
#define __StarfishConfig__

#if defined(STARFISH_EFL_CAIRO)
#define PORT_WINDOW_BACKEND_GL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_EFL
#elif defined(STARFISH_ECORE_WAYLAND2_CAIRO_GL)
#define PORT_WINDOW_BACKEND_GL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_ECORE_WAYLAND2
#elif defined(STARFISH_GLFW_CAIRO_GL)
#define PORT_WINDOW_BACKEND_GL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_GLFW
#define PORT_NEEDS_THREADED_PUBLIC_API
#elif defined(STARFISH_EFL_CAIRO_HEADLESS)
#define PORT_WINDOW_BACKEND_EFL_HEADLESS
#define PORT_GRAPHIC_BACKEND_MOCK
#define PORT_CANVAS_BACKEND_MOCK
#define PORT_COMPOSITOR_BACKEND_MOCK
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MOCK
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_EFL_SKIA)
#define PORT_WINDOW_BACKEND_GL
#define PORT_CANVAS_BACKEND_SKIA
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_PIXEL_ORDER_RGBA
#define PORT_WEBVIEW_BRIDGE_EFL
#elif defined(STARFISH_DALI)
#define PORT_WINDOW_BACKEND_GB
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_DALI
#define PORT_NEEDS_THREADED_PUBLIC_API
#elif defined(STARFISH_TIZEN_WEARABLE_WIDGET)
#define PORT_WINDOW_BACKEND_GB
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_EFL
#elif defined(STARFISH_ANDROID)
#define PORT_WINDOW_BACKEND_GB
#define PORT_CANVAS_BACKEND_SKIA
#define PORT_COMPOSITOR_BACKEND_SKIA
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_RGBA
#define PORT_NEEDS_THREADED_PUBLIC_API
#elif defined(STARFISH_WINDOWS)
#define PORT_WINDOW_BACKEND_GL
#define PORT_GRAPHIC_BACKEND_GL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_WINDOWS
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_PIXEL_ORDER_BGRA
#endif

#include "StarfishBase.h"
#include "StarfishInfo.h"

#include <SkMatrix.h>
#include <curl/curl.h>

#include "core/layout/LayoutUtil.h"
#include "core/util/String.h"
#include "core/util/AtomicString.h"
#include "core/util/QualifiedName.h"
#include "core/util/Messages.h"
#include "core/style/Length.h"
#include "core/style/Unit.h"
#include "core/style/UnitHelper.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/profiling/Profiling.h"
#include "platform/loader/ResourceURL.h"

#endif
