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

#ifndef __StarFishFrameSVGImageBox__
#define __StarFishFrameSVGImageBox__

#include "core/layout/svg/FrameSVGBox.h"
#include "core/dom/svg/SVGImageElement.h"

namespace StarFish {

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

    virtual void paintSVG(PaintingContext& ctx) override
    {
        SVGImageElement* e = node()->asSVGImageElement();
        if (e->imageData()) {
            if (e->preserveAspectRatioValue() == NativeImageData::None) {
                ctx.m_canvas->drawImage(e->imageData(),
                                        Unit::Rect(0, 0, width(), height()));
            } else {
                NativeImageData* id = e->imageData();
                auto v = e->preserveAspectRatioValue();
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

                if (v == NativeImageData::xMinYMin) {
                } else if (v == NativeImageData::xMidYMin) {
                    x = remainX / 2;
                } else if (v == NativeImageData::xMaxYMin) {
                    x = remainX;
                } else if (v == NativeImageData::xMinYMid) {
                    y = remainY / 2;
                } else if (v == NativeImageData::xMidYMid) {
                    x = remainX / 2;
                    y = remainY / 2;
                } else if (v == NativeImageData::xMaxYMid) {
                    x = remainX;
                    y = remainY / 2;
                } else if (v == NativeImageData::xMinYMax) {
                    y = remainY;
                } else if (v == NativeImageData::xMidYMax) {
                    x = remainX / 2;
                    y = remainY;
                } else if (v == NativeImageData::xMaxYMax) {
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
}

#endif
