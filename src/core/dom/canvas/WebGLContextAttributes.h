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

#ifndef __StarfishWebGLContextAttributes__
#define __StarfishWebGLContextAttributes__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "StarfishBase.h"
#include "core/dom/canvas/WebGLObject.h"
#include "platform/canvas/webgl/GLESTypes.h"
#include <string>

namespace Starfish {

class String;

enum class WebGLPowerPreference { DEFAULT, LOW_POWER, HIGH_PERFORMANCE };

struct WebGLContextAttributes {
    DEFINE_GETTER_SETTER(bool, alpha, Alpha);
    DEFINE_GETTER_SETTER(bool, depth, Depth);
    DEFINE_GETTER_SETTER(bool, stencil, Stencil);
    DEFINE_GETTER_SETTER(bool, antialias, Antialias);
    DEFINE_GETTER_SETTER(bool, premultipliedAlpha, PremultipliedAlpha);
    DEFINE_GETTER_SETTER(bool, preserveDrawingBuffer, PreserveDrawingBuffer);
    DEFINE_GETTER_SETTER(bool, failIfMajorPerformanceCaveat,
                         FailIfMajorPerformanceCaveat);
    DEFINE_GETTER_SETTER(bool, desynchronized, Desynchronized);
    String* powerPreference() const;
    void setPowerPreference(String* preference);

    bool m_alpha = true;
    bool m_depth = true;
    bool m_stencil = false;
    bool m_antialias = true;
    bool m_premultipliedAlpha = true;
    bool m_preserveDrawingBuffer = false;
    WebGLPowerPreference m_powerPreference = WebGLPowerPreference::DEFAULT;
    bool m_failIfMajorPerformanceCaveat = false;
    bool m_desynchronized = false;
};

} // namespace Starfish

#endif
#endif
