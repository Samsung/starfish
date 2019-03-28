/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarfishCanvasRenderingContext__
#define __StarfishCanvasRenderingContext__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"

namespace Starfish {
class CanvasSurface;
class CanvasRenderingContext : public ScriptWrappable {
public:
    CanvasRenderingContext(ExecutionContext* ownerExecutionContext)
        : ScriptWrappable(this, ownerExecutionContext)
        , m_originCleanFlag(false)
    {
    }

    virtual ~CanvasRenderingContext()
    {
    }

    bool originCleanFlag()
    {
        return m_originCleanFlag;
    }

    void setOriginCleanFlag(bool value)
    {
        m_originCleanFlag = value;
    }

    virtual ScriptBindingInstance* scriptBindingInstance();
    virtual void initialize() = 0;
    virtual CanvasSurface* surface() = 0;
    virtual void flush() = 0;
    virtual void onResize() = 0;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
    }

    bool m_originCleanFlag;
};
}

#endif
#endif
