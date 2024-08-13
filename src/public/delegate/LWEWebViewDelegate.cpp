/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"

#include "LWEWebViewDelegate.h"

#if defined(_MSC_VER) || defined(STARFISH_EFL_HEADLESS) || \
    defined(STARFISH_UV_CAIRO_GL) || defined(STARFISH_ANDROID)

namespace LWEDelegate {

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    // Dummy implementation.
    // Please implement this properly when implementing a windows bridge to
    // support public API of Webview.
    return nullptr;
}

} // namespace LWEDelegate
#endif

extern "C" {

uintptr_t LWEDelegate_WebView_Create(void* win, unsigned x, unsigned y,
                                     unsigned width, unsigned height,
                                     float devicePixelRatio,
                                     const char* defaultFontName,
                                     const char* locale, const char* timezoneID)
{
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::WebView::Create(win, x, y, width, height, devicePixelRatio,
                                     defaultFontName, locale, timezoneID));
}
}
