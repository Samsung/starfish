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

#ifndef __StarfishXGLPlatform__
#define __StarfishXGLPlatform__

#include "StarfishPlatform.h"
#include <cstring>
#include <memory>

#if defined(PORT_WEBVIEW_BRIDGE_EFL)
#define GL_BRIDGE_EVASGL
#else
#define GL_BRIDGE_EGL
#endif

#ifndef EGLDisplay
typedef void* EGLDisplay;
#endif

#ifndef EGLSurface
typedef void* EGLSurface;
#endif

#ifndef EGLConfig
typedef void* EGLConfig;
#endif

#ifndef EGLContext
typedef void* EGLContext;
#endif

#ifndef Evas_GL
typedef struct _Evas_GL Evas_GL;
#endif

#ifndef Evas_GL_Context
typedef struct _Evas_GL_Context Evas_GL_Context;
#endif

#ifndef Evas_GL_Surface
typedef struct _Evas_GL_Surface Evas_GL_Surface;
#endif

#ifndef Evas_GL_API
typedef struct _Evas_GL_API Evas_GL_API;
#endif

#if defined(GL_BRIDGE_EVASGL)
using XGLContext = Evas_GL_Context*;
#else
using XGLContext = EGLContext;
#endif

struct XGLPlatform {
    enum class Type {
        UNDEFINED,
        EVAS,
        EGL,
    };
    Type type;
    XGLContext context;

    union {
        struct {
            Evas_GL* object;
            Evas_GL_API* api;
            Evas_GL_Surface* surface;
        } evasgl;
        struct {
            EGLDisplay display;
            EGLConfig config;
            EGLSurface surface;
        } egl;
    };

    XGLPlatform()
    {
        type = Type::UNDEFINED;
        context = nullptr;
        memset(&evasgl, 0, sizeof(evasgl));
        memset(&egl, 0, sizeof(egl));
    }

    static std::shared_ptr<XGLPlatform> instance();
    static const XGLPlatform& ref();

    void update(const XGLPlatform platform);
    bool isValid();
};

#endif
