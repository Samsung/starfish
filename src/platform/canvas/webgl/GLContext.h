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

#if defined(STARFISH_ENABLE_WEBGL)

#ifndef __StarfishGLContext__
#define __StarfishGLContext__

#include "platform/canvas/webgl/GLES.h" // For GLuint
#include "platform/canvas/webgl/XGLUtil.h"
#include "platform/canvas/webgl/XGLPlatform.h"

namespace Starfish {

class GLContext {
public:
    GLContext();
    GLContext(XGLContext context);

    bool create(bool shareContext);
    bool destory();
    bool setCurrent();
    void resetCurrent();
    void reset();
    bool isValid();

private:
    XGLContext m_context{ nullptr };
};

class GLContextScope final {
public:
    explicit GLContextScope(GLContext context);
    ~GLContextScope();

    GLContextScope(const GLContextScope& other) = delete;
    GLContextScope& operator=(const GLContextScope& other) = delete;
    GLContextScope(GLContextScope&& other) = delete;

    static GLContext getCurrentXGLContext();

private:
    static thread_local GLContext currentContext;
};

class GLRevertableContextScope final {
public:
    explicit GLRevertableContextScope(GLContext context);
    ~GLRevertableContextScope();

    GLRevertableContextScope(const GLRevertableContextScope& other) = delete;
    GLRevertableContextScope& operator=(const GLRevertableContextScope& other) =
        delete;
    GLRevertableContextScope(GLRevertableContextScope&& other) = delete;

private:
    GLContext m_previousContext;
};

class WebGLContextScope final {
public:
    explicit WebGLContextScope(GLContext context, GLuint fbo);
    ~WebGLContextScope();

    WebGLContextScope(const WebGLContextScope& other) = delete;
    WebGLContextScope& operator=(const WebGLContextScope& other) = delete;
    WebGLContextScope(WebGLContextScope&& other) = delete;
};

} // namespace Starfish

#endif // __StarfishGLContext__

#endif
