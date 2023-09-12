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

#ifndef __StarfishEGLUtil__
#define __StarfishEGLUtil__

#include "StarfishPlatform.h"

#if defined(PORT_WEBVIEW_BRIDGE_X11) || defined(STARFISH_ENABLE_WEBGL)

#ifndef EGLDisplay
typedef void *EGLDisplay;
#endif

#ifndef EGLSurface
typedef void *EGLSurface;
#endif

#ifndef EGLConfig
typedef void *EGLConfig;
#endif

#ifndef EGLContext
typedef void *EGLContext;
#endif

struct EGLPlatform {
    EGLDisplay display{ nullptr };
    EGLSurface surface{ nullptr };
    EGLContext context{ nullptr };
    EGLConfig config{ nullptr };
};

namespace EGLUtil {

bool createEGLContext(EGLContext &context, const EGLDisplay eglDisplay,
                      const EGLConfig eglConfig, const EGLContext shareContext);

bool destroyEGLContext(const EGLDisplay display, const EGLContext context);

bool makeCurrentEGLContext(const EGLDisplay display,
                           const EGLSurface drawSurface,
                           const EGLSurface readSurface,
                           const EGLContext context);

bool makeCurrentEGLContext(const EGLPlatform &platform);

void resetCurrentEGLContext(const EGLDisplay display);

bool resetCurrentEGLContext(const EGLPlatform &platform);

bool swapGLBuffer(const EGLPlatform &platform);

void printEGLInfo(const EGLDisplay eglDisplay, const EGLConfig eglConfig,
                  const EGLContext eglContext);

} // namespace EGLUtil

#endif // __StarfishUtilEGL__
#endif
