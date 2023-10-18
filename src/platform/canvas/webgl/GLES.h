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

#ifndef __StarfishGLES__
#define __StarfishGLES__

#include "StarfishPlatform.h"

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
#include "platform/canvas/webgl/GLES_EvasGL_GLES3_Helpers.h"

EVAS_GL_GLOBAL_GLES3_DECLARE()

#elif defined(PORT_WEBVIEW_BRIDGE_GLFW) || defined(STARFISH_X11_CAIRO_GL)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#elif defined(STARFISH_WINDOWS)
#include <GL/glew.h>
#else
// Assume GLES3/gl3.h is available as default.
#include <GLES3/gl3.h>
#endif

#endif
