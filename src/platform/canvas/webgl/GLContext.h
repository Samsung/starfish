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

namespace Starfish {

class Renderer;

class GLContext {
public:
    GLContext();
    GLContext(Renderer* renderer);

    bool createSharedContext();
    bool destory();
    bool setCurrent();
    void resetCurrent();
    void reset();
    bool isValid();

private:
    uintptr_t m_context{ UINTPTR_MAX };
    Renderer* m_renderer{ nullptr };
};

class GLContextScope final {
public:
    explicit GLContextScope(GLContext context);
    ~GLContextScope();

    GLContextScope(const GLContextScope& other) = delete;
    GLContextScope& operator=(const GLContextScope& other) = delete;
    GLContextScope(GLContextScope&& other) = delete;

    bool hasError()
    {
        return m_result == false;
    }

    static GLContext getCurrentGLContext();

private:
    static thread_local GLContext currentContext;
    bool m_result{ false };
};

class GLRevertableContextScope final {
public:
    explicit GLRevertableContextScope(GLContext context, Renderer* renderer);
    ~GLRevertableContextScope();

    GLRevertableContextScope(const GLRevertableContextScope& other) = delete;
    GLRevertableContextScope& operator=(const GLRevertableContextScope& other) =
        delete;
    GLRevertableContextScope(GLRevertableContextScope&& other) = delete;

private:
    Renderer* m_renderer = nullptr;
};

} // namespace Starfish

#endif // __StarfishGLContext__

#endif
