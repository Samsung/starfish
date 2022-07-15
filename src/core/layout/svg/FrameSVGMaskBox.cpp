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
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/page/WebView.h"

#include "core/modules/canvas/image/BufferedNativeImageData.h"

namespace Starfish {

void FrameSVGMaskBox::makeLuminanceMask(NativeImageData* image)
{
    size_t h = image->height();
    size_t w = image->width();
    size_t s = image->stride();
    uint8_t* ptr = image->data();
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

void FrameSVGMaskBox::applyMask(PaintingContext& ctx, float x, float y)
{
    FrameBox* svgBox = this;

    while (!svgBox->isFrameSVGSVGBox()) {
        svgBox = svgBox->parent()->asFrameBox();
    }

    size_t svgElementWidth = svgBox->width().toUnsigned();
    size_t svgElementHeight = svgBox->height().toUnsigned();

    NativeImageData* nativeImageMask = BufferedNativeImageData::create(
        std::max(svgElementWidth, ctx.m_canvas->renderTargetInfo().m_width),
        std::max(svgElementHeight, ctx.m_canvas->renderTargetInfo().m_height));

    Canvas* newCanvas = Canvas::create(node()->webView(), nativeImageMask);
    newCanvas->clearColor(Unit::Color(0, 0, 0, 0));

    PaintingContext newCtx(newCanvas);
    Frame* child = firstChild();
    while (child) {
        if (child && child->isFrameSVGBox()) {
            FrameSVGBox* childBox = child->asFrameSVGBox();
            newCanvas->save();
            newCanvas->translate(childBox->x(), childBox->y());
            childBox->paintContent(newCtx);
            newCanvas->restore();
        }
        child = child->next();
    }
    delete newCanvas;

    makeLuminanceMask(nativeImageMask);
    ctx.m_canvas->maskNativeImage(
        nativeImageMask,
        Unit::Rect(x, y, nativeImageMask->width(), nativeImageMask->height()));
}
} // namespace Starfish
