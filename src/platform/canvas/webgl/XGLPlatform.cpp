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

#include "platform/canvas/webgl/XGLPlatform.h"

bool XGLPlatform::checkValid(const XGLPlatform& env)
{
    if (env.type == Type::UNDEFINED) {
        return false;
    }

    if (env.type == Type::EGL) {
        if (!env.egl.display || !env.egl.surface || !env.egl.config) {
            return false;
        }
    } else if (env.type == Type::EVAS) {
        if (!env.evasgl.object || !env.evasgl.api || !env.evasgl.surface) {
            return false;
        }
    } else {
        return false;
    }

    if (!env.context) {
        return false;
    }

    return true;
}
