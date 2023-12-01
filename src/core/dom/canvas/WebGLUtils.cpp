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

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishConfig.h"
#include "WebGLUtils.h"
#include "platform/canvas/webgl/GLES.h"

namespace Starfish {

const char* webglErrorString(unsigned int code)
{
    switch (code) {
    case GL_NO_ERROR:
        return "NO_ERROR";
    case GL_INVALID_ENUM:
        return "INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "INVALID_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "OUT_OF_MEMORY";
    default:
        return "UNDEFINED_ERROR";
    }
    return "";
}

} // namespace Starfish

#endif
