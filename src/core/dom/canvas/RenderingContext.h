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

#ifndef __StarfishRenderingContext__
#define __StarfishRenderingContext__

#ifdef STARFISH_ENABLE_CANVAS

#include "binding/ScriptWrappable.h"

namespace Starfish {

class CanvasSurface;
class HTMLCanvasElement;

class RenderingContext : public ScriptWrappable {
public:
    RenderingContext(HTMLCanvasElement* canvasElement)
        : ScriptWrappable(this)
        , m_ownerHTMLCanvasElement(canvasElement)
        , m_surface(nullptr)
        , m_originCleanFlag(false)
    {
    }

    HTMLCanvasElement* canvas()
    {
        return m_ownerHTMLCanvasElement;
    }

    CanvasSurface* surface()
    {
        return m_surface;
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
    virtual void flush() = 0;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc,
                   GC_WORD_OFFSET(RenderingContext, m_ownerHTMLCanvasElement));
        GC_set_bit(desc, GC_WORD_OFFSET(RenderingContext, m_surface));
    }

    HTMLCanvasElement* m_ownerHTMLCanvasElement;
    CanvasSurface* m_surface;
    bool m_originCleanFlag;
};
}

#endif
#endif
