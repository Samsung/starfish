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

#include "StarfishPlatform.h"

#if defined(PORT_WEBVIEW_BRIDGE_EFL) || defined(STARFISH_ENABLE_WEBGL)

#include "platform/canvas/webgl/XGLUtil.h"
#include "platform/canvas/webgl/XGL.h"
#include "StarfishBase.h"

#if defined(GL_BRIDGE_EVASGL)

namespace XGLUtil {

void initXGLPlatform()
{
}

bool createXGLContext(XGLContext& context, const XGLContext shareContext)
{
    XGLPlatform platform = XGLPlatform::ref();
    XGLContext newContext = evas_gl_context_version_create(
        platform.evasgl.object, shareContext,
        Evas_GL_Context_Version::EVAS_GL_GLES_3_X);

    if (newContext == nullptr) {
        newContext = evas_gl_context_version_create(
            platform.evasgl.object, shareContext,
            Evas_GL_Context_Version::EVAS_GL_GLES_2_X);
    }

    context = newContext;

    STARFISH_ASSERT(context != nullptr);
    return true;
}

bool destroyXGLContext(const XGLContext context)
{
    XGLPlatform platform = XGLPlatform::ref();
    evas_gl_context_destroy(platform.evasgl.object, context);
    return true;
}

bool makeCurrentXGLContext(const XGLContext context)
{
    XGLPlatform platform = XGLPlatform::ref();
    Eina_Bool evas_result = evas_gl_make_current(
        platform.evasgl.object, platform.evasgl.surface, context);

    STARFISH_ASSERT(evas_result == EINA_TRUE);

    return evas_result == EINA_TRUE ? true : false;
}

bool resetCurrentXGLContext()
{
    XGLPlatform platform = XGLPlatform::ref();
    evas_gl_make_current(platform.evasgl.object, nullptr, nullptr);
    return true;
}

bool swapXGLBuffer()
{
    // Handled by Evas, nothing to do.
    return true;
}

void printXGLInfo()
{
}

} // namespace XGLUtil

#endif // defined(GL_BRIDGE_EVASGL)
#endif
