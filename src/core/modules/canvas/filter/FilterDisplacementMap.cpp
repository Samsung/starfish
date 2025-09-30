/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "StarfishConfig.h"

#include "core/dom/svg/SVGFEDisplacementMapElement.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterDisplacementMap.h"
#include "core/layout/svg/FrameSVGBox.h"

#include <cstring>
#include <vector>

namespace Starfish {

FilterDisplacementMap::FilterDisplacementMap(
    Filter* filter, SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(
          filter, element,
          element->asSVGFEDisplacementMapElement()->in1()->baseVal(),
          element->asSVGFEDisplacementMapElement()->in2()->baseVal(),
          element->output()->baseVal())
{
}

void* FilterDisplacementMap::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterDisplacementMap));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterDisplacementMap)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr =
            GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterDisplacementMap));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool FilterDisplacementMap::canShrinkPreviousResult() const
{
    if (input()->equals("SourceGraphic")) {
        return true;
    }
    return false;
}

Filter::FilterBias FilterDisplacementMap::computeBias(ComputeBiasContext& ctx)
{
    if (input()->equals("SourceGraphic")) {
        auto e = element()->asSVGFEDisplacementMapElement();
        float scale = e->scale()->animVal();
        scale *= std::max(ctx.viewportScale.first, ctx.viewportScale.second);

        scale /= 2;
        Unit::Rect newRt = Unit::Rect(ctx.unadjustedFrameRectByFilter.x(),
                                      ctx.unadjustedFrameRectByFilter.y(),
                                      ctx.unadjustedFrameRectByFilter.width(),
                                      ctx.unadjustedFrameRectByFilter.height());
        newRt.setX(newRt.x() - scale);
        newRt.setY(newRt.y() - scale);
        newRt.setWidth(newRt.width() + scale * 2);
        newRt.setHeight(newRt.height() + scale * 2);

        ctx.currentVisibleRect = ctx.unadjustedFrameRectByFilter;
        ctx.currentVisibleRect.setX(ctx.currentVisibleRect.x() - scale);
        ctx.currentVisibleRect.setY(ctx.currentVisibleRect.y() - scale);
        ctx.currentVisibleRect.setWidth(ctx.currentVisibleRect.width() +
                                        scale * 2);
        ctx.currentVisibleRect.setHeight(ctx.currentVisibleRect.height() +
                                         scale * 2);

        return Filter::FilterBias(newRt);
    }
    return Filter::FilterBias();
}

void FilterDisplacementMap::apply(const Unit::Rect& subRegionInFloat,
                                  Filter::FilterApplyContext& ctx)
{
    auto e = element()->asSVGFEDisplacementMapElement();

    auto inputSource = filter()->fetchInputSource(ctx, this);
    auto inputSource2 = filter()->fetchInputSource2(ctx, this);
    auto outputSource = filter()->fetchOutputSource(
        ctx, this, ctx.sourceGraphic(), subRegionInFloat);

    unsigned char* srcData = (unsigned char*)inputSource->data();
    unsigned char* srcData2 = (unsigned char*)inputSource2->data();

    convertImageBufferAsUnmultipliedAlphaIfNeeds(
        inputSource2->data(), ctx.width, ctx.stride, ctx.height);

    unsigned char* dstData = (unsigned char*)outputSource->data();

    if (!srcData || !srcData2 || !dstData) {
        filter()->registerOutput(ctx, this, outputSource);
        return;
    }

    float scale = e->scale()->animVal();
    auto xChannel = e->xChannelSelector()->animVal();
    auto yChannel = e->yChannelSelector()->animVal();

    auto channelToIndex =
        [](SVGFEDisplacementMapElement::ChannelSelector v) -> int {
        switch (v) {
        case SVGFEDisplacementMapElement::ChannelSelector::SVG_CHANNEL_R:
            return STARFISH_PIXEL_R_INDEX;
        case SVGFEDisplacementMapElement::ChannelSelector::SVG_CHANNEL_G:
            return STARFISH_PIXEL_G_INDEX;
        case SVGFEDisplacementMapElement::ChannelSelector::SVG_CHANNEL_B:
            return STARFISH_PIXEL_B_INDEX;
        case SVGFEDisplacementMapElement::ChannelSelector::SVG_CHANNEL_A:
            return STARFISH_PIXEL_A_INDEX;
        default:
            // Should never reach here. Just to avoid warning.
            return STARFISH_PIXEL_A_INDEX;
        }
        STARFISH_ASSERT_NOT_REACHED();
        return 0;
    };

    int displacementChannelX =
        channelToIndex((SVGFEDisplacementMapElement::ChannelSelector)xChannel);
    int displacementChannelY =
        channelToIndex((SVGFEDisplacementMapElement::ChannelSelector)yChannel);

    int width = ctx.width;
    int height = ctx.height;
    int stride = ctx.stride;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixel_offset = (y * stride) + (x * 4);

            unsigned char displaceXVal =
                srcData2[pixel_offset + displacementChannelX];
            unsigned char displaceYVal =
                srcData2[pixel_offset + displacementChannelY];

            // P'(x,y) = P( x + scale * (XC(x,y) - 0.5), y + scale * (YC(x,y) -
            // 0.5) )
            float srcX_float = x + scale * (displaceXVal / 255.f - 0.5f);
            float srcY_float = y + scale * (displaceYVal / 255.f - 0.5f);

            int srcX = static_cast<int>(srcX_float + 0.5f);
            int srcY = static_cast<int>(srcY_float + 0.5f);

            unsigned int* dstPixel =
                (unsigned int*)(void*)(dstData + pixel_offset);

            if (srcX < 0 || srcX >= width || srcY < 0 || srcY >= height) {
                *dstPixel = 0;
            } else {
                int src_pixel_offset = (srcY * stride) + (srcX * 4);
                unsigned int* srcPixel =

                    (unsigned int*)(void*)(srcData + src_pixel_offset);
                *dstPixel = *srcPixel;
            }
        }
    }
    convertImageBufferAsPremultipliedAlphaIfNeeds(
        inputSource2->data(), ctx.width, ctx.stride, ctx.height);

    filter()->registerOutput(ctx, this, outputSource);
}

} // namespace Starfish
