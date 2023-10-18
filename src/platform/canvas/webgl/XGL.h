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

#ifndef __StarfishXGL__
#define __StarfishXGL__

#include "StarfishPlatform.h"

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
#include <Evas_GL.h>
#elif defined(STARFISH_WINDOWS)
#include <GL/glew.h>
#else
// NOTE: Due to a GC(_XGC) conflict predefined in Xlib.h, X11 is forcibly
// disabled. Native window-related EGL APIs may not work properly. However, we
// would rarely use them. Use NO_ESCARGOT_GC_CONFLICT_GUARD if needed.
#ifndef NO_ESCARGOT_GC_CONFLICT_GUARD
#define EGL_NO_X11
#endif

#include <EGL/egl.h>

#ifdef EGL_NO_X11
#undef EGL_NO_X11
#endif

#endif

#endif // __StarfishXGL__
