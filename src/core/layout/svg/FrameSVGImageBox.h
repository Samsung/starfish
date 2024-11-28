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

#ifndef __StarfishFrameSVGImageBox__
#define __StarfishFrameSVGImageBox__

#include "core/layout/svg/FrameSVGBox.h"
#include "core/dom/svg/SVGImageElement.h"

namespace Starfish {

class FrameSVGImageBox final : public FrameSVGBox {
public:
    FrameSVGImageBox(Node* node)
        : FrameSVGBox(node)
    {
    }

    virtual const char* name() override
    {
        return "FrameSVGImageBox";
    }

    virtual void layoutSVG() override
    {
        // https://www.w3.org/TR/SVG11/struct.html#ImageElement
        // For most raster content (PNG, JPEG) the bounds of the image should be
        // used (i.e. the ‘image’ element has an implicit ‘viewBox’ of '0 0
        // raster-image-width raster-image-height').
        auto vp = viewport();
        NativeImageData* id = node()->asSVGImageElement()->imageData();
        if (id && node()->asSVGImageElement()->preserveAspectRatioAlign() ==
                      NativeImageData::None) {
            auto styleWidth = style()->width();
            auto styleHeight = style()->height();
            FrameBox* cb = layoutParent()->asFrameBox();
            if (styleWidth.isAuto() && styleHeight.isAuto()) {
                setWidth(id->width());
                setHeight(id->height());
            } else if (styleWidth.isAuto()) {
                setHeight(styleHeight.specifiedValue(vp.height(), this));
                if (id->width() && id->height()) {
                    setWidth(height() * id->width() / id->height());
                } else {
                    setWidth(0);
                }
            } else if (styleHeight.isAuto()) {
                setWidth(styleWidth.specifiedValue(vp.width(), this));
                if (id->width() && id->height()) {
                    setHeight(width() * id->height() / id->width());
                } else {
                    setHeight(0);
                }
            }
        }
    }

    virtual void paintSVG(PaintingContext& ctx) override
    {
        SVGImageElement* e = node()->asSVGImageElement();
        if (e->imageData()) {
            auto svgAlign = e->preserveAspectRatioAlign();
            if (svgAlign == NativeImageData::None) {
                ctx.m_canvas->drawImage(e->imageData(),
                                        Unit::Rect(0, 0, width(), height()));
            } else {
                NativeImageData* id = e->imageData();
                LayoutUnit containerWidth = width();
                LayoutUnit containerHeight = height();

                if (containerWidth == LayoutUnit(0) ||
                    containerHeight == LayoutUnit(0)) {
                    return;
                }

                LayoutUnit imageDstWidth;
                LayoutUnit imageDstHeight;

                if (containerWidth / containerHeight >
                    (float)id->width() / (float)id->height()) {
                    imageDstWidth = containerHeight * (float)id->width() /
                                    (float)id->height();
                    imageDstHeight = containerHeight;
                } else {
                    imageDstWidth = containerWidth;
                    imageDstHeight = containerWidth * (float)id->height() /
                                     (float)id->width();
                }

                LayoutUnit remainX = containerWidth - imageDstWidth;
                LayoutUnit remainY = containerHeight - imageDstHeight;

                LayoutUnit x, y;

                if (svgAlign == NativeImageData::xMinYMin) {
                } else if (svgAlign == NativeImageData::xMidYMin) {
                    x = remainX / 2;
                } else if (svgAlign == NativeImageData::xMaxYMin) {
                    x = remainX;
                } else if (svgAlign == NativeImageData::xMinYMid) {
                    y = remainY / 2;
                } else if (svgAlign == NativeImageData::xMidYMid) {
                    x = remainX / 2;
                    y = remainY / 2;
                } else if (svgAlign == NativeImageData::xMaxYMid) {
                    x = remainX;
                    y = remainY / 2;
                } else if (svgAlign == NativeImageData::xMinYMax) {
                    y = remainY;
                } else if (svgAlign == NativeImageData::xMidYMax) {
                    x = remainX / 2;
                    y = remainY;
                } else if (svgAlign == NativeImageData::xMaxYMax) {
                    x = remainX;
                    y = remainY;
                }

                ctx.m_canvas->drawImage(
                    id, Unit::Rect(x, y, imageDstWidth, imageDstHeight));
            }
        }
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(FrameSVGImageBox));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(FrameSVGImageBox)] = { 0 };
            FrameSVGImageBox::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameSVGImageBox));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }

    void* operator new[](size_t size) = delete;

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        FrameSVGBox::fillGCDescriptor(desc);
    }
};
} // namespace Starfish

#endif
