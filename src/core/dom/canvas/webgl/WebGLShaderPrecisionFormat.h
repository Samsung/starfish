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

#ifndef __StarfishWebGLShaderPrecisionFormat__
#define __StarfishWebGLShaderPrecisionFormat__

#if defined(STARFISH_ENABLE_CANVAS) && defined(STARFISH_ENABLE_WEBGL)

#include "binding/ScriptWrappable.h"
#include "platform/canvas/gl/GLTypes.h"

namespace Starfish {

class ScriptBindingInstance;

class WebGLShaderPrecisionFormat : public ScriptWrappable {
public:
    WebGLShaderPrecisionFormat(ScriptBindingInstance* instance, GLint rangeMin,
                               GLint rangeMax, GLint precision)
        : ScriptWrappable(this)
        , m_scriptBindingInstance(instance)
        , m_rangeMin(rangeMin)
        , m_rangeMax(rangeMax)
        , m_precision(precision)
    {
    }
    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    bool isWebGLShaderPrecisionFormat() const;
    ScriptBindingInstance* scriptBindingInstance() override
    {
        return m_scriptBindingInstance;
    }

    DEFINE_GETTER(GLint, rangeMin);
    DEFINE_GETTER(GLint, rangeMax);
    DEFINE_GETTER(GLint, precision);

private:
    ScriptBindingInstance* m_scriptBindingInstance;
    GLint m_rangeMin;
    GLint m_rangeMax;
    GLint m_precision;
};
} // namespace Starfish

#endif
#endif
