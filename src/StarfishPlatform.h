/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPlatform__
#define __StarfishPlatform__

#if defined(STARFISH_EFL_CAIRO_GL)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_EFL
#elif defined(STARFISH_UV_CAIRO_GL)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_EFL_HEADLESS)
#define PORT_GRAPHIC_BACKEND_MOCK
#define PORT_CANVAS_BACKEND_MOCK
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MOCK
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_GLIB_CAIRO_GL)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_GLIB
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_GLIB_HEADLESS)
#define PORT_GRAPHIC_BACKEND_MOCK
#define PORT_CANVAS_BACKEND_MOCK
#define PORT_EVENTLOOP_BACKEND_GLIB
#define PORT_IMAGEDECODER_BACKEND_MOCK
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_TIZEN_WEARABLE_WIDGET)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_EFL
#elif defined(STARFISH_ANDROID)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_WINDOWS_UWP)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_WINDOWS)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_WINDOWS
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_FLUTTER)
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#define PORT_WEBVIEW_BRIDGE_FLUTTER
#define PORT_BACKEND_GL_WITH_EXTERNAL_TBM
#endif

#if defined(PORT_CANVAS_BACKEND_CAIRO)
#define PORT_CANVAS_NEEDS_PREMULTIPLIED_ALPHA
#endif

#if !defined(STARFISH_WEBWORKER_HOST)
#define STARFISH_WEBWORKER_NOT_HOST
#endif

#endif
