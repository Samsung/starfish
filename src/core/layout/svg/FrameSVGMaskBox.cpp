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
            if (a == 255) {
                uint8_t r = p[0];
                uint8_t g = p[1];
                uint8_t b = p[2];

                double luma = (r * 0.2125 + g * 0.7154 + b * 0.0721) *
                              ((double)a / 255.0);
                p[3] = luma;
            }
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

class SVGMaskPaintingDetphMarker {
public:
    SVGMaskPaintingDetphMarker(size_t& d)
        : depth(d)
    {
        depth++;
    }

    ~SVGMaskPaintingDetphMarker()
    {
        depth--;
    }

    size_t& depth;
};

void FrameSVGMaskBox::applyMask(PaintingContext& ctx, FrameSVGBox* targetBox)
{
    FrameSVGSVGBox* viewportBox = outmostSVGViewportBox();
    auto ctm = ctx.m_canvas->currentTransformMatrix();
    viewportBox->pushToSVGMaskPaintingStack(targetBox);
    LayoutRect childrenRect = (*viewportBox->svgMaskPaintingStack().begin())
        ->absoluteRect(viewportBox);

    ctx.m_canvas->setMatrix(viewportBox->svgPaintingMatrix());
    ctx.m_canvas->beginSubCanvas(childrenRect, SubCanvasMode::Mask);
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

    Canvas::SubCanvasPixelModifyFunction fn;
    if (style()->maskType() == MaskTypeValue::LuminanceMaskTypeValue) {
        fn = makeLuminanceMask;
    }
    ctx.m_canvas->endSubCanvas(fn);
    ctx.m_canvas->setMatrix(ctm);

    viewportBox->popSVGMaskPaintingStack();
}
} // namespace Starfish
