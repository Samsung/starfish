/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishCanvasRenderingContext2D__
#define __StarfishCanvasRenderingContext2D__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/canvas/CanvasRenderingContext2DMixIn.h"

namespace Starfish {

class CanvasRenderingContext2D : public CanvasRenderingContext2DMixIn {
public:
    CanvasRenderingContext2D(HTMLCanvasElement* canvasElement);
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCanvasRenderingContext2D() const override;

    HTMLCanvasElement* canvas()
    {
        return m_ownerHTMLCanvasElement;
    }

    void* operator new(size_t size)
    {
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(CanvasRenderingContext2D)] = { 0 };
            CanvasRenderingContext2D::fillGCDescriptor(desc);
            descr =
                GC_make_descriptor(desc, GC_WORD_LEN(CanvasRenderingContext2D));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        CanvasRenderingContext2DMixIn::fillGCDescriptor(desc);
    }

private:
};
}

#endif
#endif
