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
    LayoutRect childrenRect;
    Frame* f = firstChild();
    while (f) {
        childrenRect.unite(f->asFrameBox()->frameRect());
        f = f->next();
    }

    auto pixelSnappedRect = childrenRect.snapSizeToPixel();

    NativeImageData* nativeImageMask = BufferedNativeImageData::create(
            node()->webView()->screenInfo().devicePixelRatio,
            pixelSnappedRect.width().toUnsigned(),
            pixelSnappedRect.height().toUnsigned());

    FrameSVGSVGBox* viewportBox = outmostSVGViewportBox();
    bool applyMaskOnSVGViewport = viewportBox->svgMaskPaintingDepth() == 0;
    SVGMaskPaintingDetphMarker marker(viewportBox->svgMaskPaintingDepth());

    std::vector<Frame*> tree;
    f = targetBox->parent();
    while (f != viewportBox) {
        tree.push_back(f);
        f = f->parent();
    }

    Canvas* newCanvas = Canvas::create(node()->webView(), nativeImageMask);
    newCanvas->clearColor(Unit::Color(0, 0, 0, 0));

    auto transScale = viewportBox->computeTranlateScaleOnPaint();
    newCanvas->translate(-childrenRect.x() + transScale.second.getTranslateX(),
            -childrenRect.y() + transScale.second.getTranslateY());
    newCanvas->scale(transScale.second.getScaleX(), transScale.second.getScaleY());

    // painting mask content
    PaintingContext newCtx(newCanvas);
    Frame* child = firstChild();
    while (child) {
        if (child->isFrameSVGBox()) {
            FrameSVGBox* childBox = child->asFrameSVGBox();
            newCanvas->save();
            childBox->paintContent(newCtx);
            newCanvas->restore();
        }
        child = child->next();
    }

    delete newCanvas;

    if (style()->maskType() == MaskTypeValue::LuminanceMaskTypeValue) {
        makeLuminanceMask(nativeImageMask);
    }

    auto ctm = ctx.m_canvas->currentTransformMatrix();
    for (auto iter = tree.rbegin(); iter != tree.rend(); iter++) {
        Frame* f = *iter;
        f->asFrameSVGBox()->applyTransformTo(ctx.m_canvas, viewportBox->viewport());
    }
    auto transformedCTM = ctx.m_canvas->currentTransformMatrix();

    if (applyMaskOnSVGViewport) {
        ctx.m_canvas->setMatrix(viewportBox->svgPaintingMatrix());
    } else {
        ctx.m_canvas->setMatrix(ctm);
    }

    if (applyMaskOnSVGViewport) {
        ctx.m_canvas->translate(childrenRect.x(), childrenRect.y());
        ctx.m_canvas->maskNativeImage(
            nativeImageMask,
            Unit::Rect(transformedCTM.getTranslateX() - ctm.getTranslateX(),
                    transformedCTM.getTranslateY() - ctm.getTranslateY(),
                    childrenRect.width(), childrenRect.height()));
    } else {
        SkMatrix invertedMatrix;
        transScale.second.invert(&invertedMatrix);
        auto rt = SkRect::MakeXYWH(childrenRect.x(), childrenRect.y(),
                                   childrenRect.width(), childrenRect.height());
        invertedMatrix.mapRect(&rt);
        ctx.m_canvas->translate(rt.x(), rt.y());
        ctx.m_canvas->maskNativeImage(
            nativeImageMask,
            Unit::Rect(0, 0, rt.width(), rt.height()));
    }
    ctx.m_canvas->setMatrix(ctm);
}
} // namespace Starfish
