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

#ifndef __StarFishHTMLCanvasElement__
#define __StarFishHTMLCanvasElement__

#ifdef STARFISH_ENABLE_CANVAS

#include "core/dom/HTMLElement.h"
#include "core/paint/PaintCommandBuffer.h"

namespace StarFish {

#define STARFISH_CANVAS_DEFAULT_WIDTH 300
#define STARFISH_CANVAS_DEFAULT_HEIGHT 150

class RenderingContext;
class
    CanvasRenderingContext2DOrWebGLRenderingContextOrImageBitmapRenderingContext;
typedef CanvasRenderingContext2DOrWebGLRenderingContextOrImageBitmapRenderingContext
    RenderingContextBindindingUnion;

class HTMLCanvasElement : public HTMLElement {
public:
    enum CanvasContextMode {
        CanvasContextModeNone,
        CanvasContextModePlaceHolder,
        CanvasContextMode2D,
        CanvasContextModeBitmapRenderer,
        CanvasContextModeWebGL
    };

    HTMLCanvasElement(Document* document)
        : HTMLElement(document)
        , m_renderingContext(nullptr)
        , m_contextMode(CanvasContextModeNone)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLCanvasElement() const;
    virtual QualifiedName name();

    uint32_t width();
    void setWidth(uint32_t value);
    PaintCommandBuffer& commandBuffer()
    {
        return m_buffer;
    }

    uint32_t height();
    void setHeight(uint32_t value);
    Nullable<RenderingContextBindindingUnion> getContext(
        String* contextId, GCVector<ScriptValue> arguments);

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(HTMLCanvasElement));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(HTMLCanvasElement)] = { 0 };
            HTMLCanvasElement::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLCanvasElement));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        HTMLElement::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLCanvasElement, m_renderingContext));
    }

private:
    RenderingContext* m_renderingContext;
    PaintCommandBuffer m_buffer;
    CanvasContextMode m_contextMode;
};
}

#endif
#endif
