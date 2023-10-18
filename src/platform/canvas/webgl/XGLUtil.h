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
#include "platform/canvas/webgl/XGLPlatform.h"

#if defined(PORT_WEBVIEW_BRIDGE_X11) || defined(STARFISH_ENABLE_WEBGL)

namespace XGLUtil {

bool createXGLContext(XGLContext &context, const XGLPlatform &platform,
                      const XGLContext shareContext);

bool destroyXGLContext(const XGLPlatform &platform, const XGLContext context);

bool makeCurrentXGLContext(const XGLPlatform &platform,
                           const XGLContext context);

XGLContext getCurrentXGLContext();

bool resetCurrentXGLContext(const XGLPlatform &platform);

bool swapXGLBuffer(const XGLPlatform &platform);

void printXGLInfo(const XGLPlatform &platform);

} // namespace XGLUtil

#endif // __StarfishUtilEGL__
#endif
