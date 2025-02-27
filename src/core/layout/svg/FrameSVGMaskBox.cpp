/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameSVGMaskBox.h"
#include "FrameSVGSVGBox.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/svg/SVGElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/page/WebView.h"

#include "core/modules/canvas/image/BufferedNativeImageData.h"

namespace Starfish {

static void makeLuminanceMask(uint8_t* ptr, size_t w, size_t s, size_t h)
{
    for (size_t y = 0; y < h; y++) {
        uint8_t* p = ptr;
        for (size_t x = 0; x < w; x++) {
            uint8_t a = p[3];
#ifdef PORT_PIXEL_ORDER_RGBA
            uint32_t r = p[0];
            uint32_t g = p[1];
            uint32_t b = p[2];
#else
            uint32_t r = p[2];
            uint32_t g = p[1];
            uint32_t b = p[0];
#endif
            *reinterpret_cast<uint32_t*>(p) =
                ((r * 109 + g * 366 + b * 37 + 256) << 15) & 0xff000000;
            p += 4;
        }
        ptr += s;
    }
}

void* FrameSVGMaskBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameSVGMaskBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameSVGMaskBox)] = { 0 };
        FrameSVGMaskBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGMaskBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FrameSVGMaskBox::paintContent(PaintingContext& ctx)
{
    // ‘mask’ elements are never rendered directly
}

void FrameSVGMaskBox::paintSVG(PaintingContext& ctx)
{
    // ‘mask’ elements are never rendered directly
}

void FrameSVGMaskBox::applyMask(PaintingContext& ctx, FrameSVGBox* targetBox)
{
    FrameSVGSVGBox* viewportBox = outmostSVGViewportBox();
    auto ctm = ctx.m_canvas->currentTransformMatrix();
    viewportBox->pushToSVGMaskPaintingStack(targetBox);
    ctx.m_canvas->setMatrix(viewportBox->svgPaintingMatrix());
    ctx.m_canvas->beginLayer(viewportBox->topmostMaskPaintingRect(), 1,
                             CanvasLayerMode::Mask);
    ctx.m_canvas->setMatrix(ctm);

    Frame* f = firstChild();
    while (f) {
        if (f->isFrameSVGBox()) {
            FrameSVGBox* childBox = f->asFrameSVGBox();
            ctx.m_canvas->save();
            childBox->paintContent(ctx);
            ctx.m_canvas->restore();
        }
        f = f->next();
    }

    Canvas::LayerPixelModifyFunction fn;
    if (style()->maskType() == MaskTypeValue::LuminanceMaskTypeValue) {
        fn = makeLuminanceMask;
    }
    ctx.m_canvas->endLayer(fn);
    ctx.m_canvas->setMatrix(ctm);

    viewportBox->popSVGMaskPaintingStack();
}
} // namespace Starfish
