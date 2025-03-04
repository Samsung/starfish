/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Igalia, S.L.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2015-2016 Apple, Inc. All rights reserved.
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#include <math.h>
#include "core/dom/svg/SVGFEColorMatrixElement.h"
#include "core/dom/svg/SVGAnimatedNumberList.h"
#include "core/modules/canvas/filter/Filter.h"
#include "core/modules/canvas/filter/FilterColorMatrix.h"

namespace Starfish {

inline float deg2rad(float d)
{
    float radiansPerDegreeFloat = static_cast<float>(M_PI) / 180.0f;
    return d * radiansPerDegreeFloat;
}

inline uint8_t adjustValueForPixel(float value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return value;
}

inline void applyMatrix(uint8_t* buffer, std::array<float, 20> matrix,
                        size_t stride, size_t imageWidth, size_t imageHeight)
{
    for (size_t y = 0; y < imageHeight; y++) {
        uint8_t* p = buffer;
        for (size_t x = 0; x < imageWidth; x++) {
            uint8_t b = p[0];
            uint8_t g = p[1];
            uint8_t r = p[2];
            uint8_t a = p[3];

            float r_ = matrix[0] * r + matrix[1] * g + matrix[2] * b +
                       matrix[3] * a + matrix[4] * 255;
            float g_ = matrix[5] * r + matrix[6] * g + matrix[7] * b +
                       matrix[8] * a + matrix[9] * 255;
            float b_ = matrix[10] * r + matrix[11] * g + matrix[12] * b +
                       matrix[13] * a + matrix[14] * 255;
            float a_ = matrix[15] * r + matrix[16] * g + matrix[17] * b +
                       matrix[18] * a + matrix[19] * 255;

            p[0] = adjustValueForPixel(b_);
            p[1] = adjustValueForPixel(g_);
            p[2] = adjustValueForPixel(r_);
            p[3] = adjustValueForPixel(a_);

            p += 4;
        }
        buffer += stride;
    }
}

inline void applySaturateAndHueRotate(uint8_t* buffer,
                                      std::array<float, 9> matrix,
                                      size_t stride, size_t imageWidth,
                                      size_t imageHeight)
{
    std::array<float, 20> newMatrix = { matrix[0], matrix[1], matrix[2], 0, 0,
                                        matrix[3], matrix[4], matrix[5], 0, 0,
                                        matrix[6], matrix[7], matrix[8], 0, 0,
                                        0,         0,         0,         1, 0 };
    applyMatrix(buffer, newMatrix, stride, imageWidth, imageHeight);
}

inline void applyLuminanceAlpha(uint8_t* buffer, size_t stride,
                                size_t imageWidth, size_t imageHeight)
{
    std::array<float, 20> newMatrix = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,    0.0,    0.0,    0.0, 0.0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.2125, 0.7154, 0.0721, 0.0, 0.0,
    };
    applyMatrix(buffer, newMatrix, stride, imageWidth, imageHeight);
}

inline std::array<float, 9> saturationMatrix(float value)
{
    std::array<float, 9> result = {
        0.213f + 0.787f * value, 0.715f - 0.715f * value,
        0.072f - 0.072f * value, 0.213f - 0.213f * value,
        0.715f + 0.285f * value, 0.072f - 0.072f * value,
        0.213f - 0.213f * value, 0.715f - 0.715f * value,
        0.072f + 0.928f * value
    };
    return result;
}

inline std::array<float, 9> hueRotateMatrix(float value)
{
    float cosHue = cos(deg2rad(value));
    float sinHue = sin(deg2rad(value));
    std::array<float, 9> result = { 0.213f + cosHue * 0.787f - sinHue * 0.213f,
                                    0.715f - cosHue * 0.715f - sinHue * 0.715f,
                                    0.072f - cosHue * 0.072f + sinHue * 0.928f,
                                    0.213f - cosHue * 0.213f + sinHue * 0.143f,
                                    0.715f + cosHue * 0.285f + sinHue * 0.140f,
                                    0.072f - cosHue * 0.072f - sinHue * 0.283f,
                                    0.213f - cosHue * 0.213f - sinHue * 0.787f,
                                    0.715f - cosHue * 0.715f + sinHue * 0.715f,
                                    0.072f + cosHue * 0.928f +
                                        sinHue * 0.072f };
    return result;
}

FilterColorMatrix::FilterColorMatrix(
    Filter* filter, SVGFilterPrimitiveStandardAttributes* element)
    : FilterPrimitive(filter, element)
{
    STARFISH_ASSERT(filter);
    STARFISH_ASSERT(element->isSVGFEColorMatrixElement());
}

void* FilterColorMatrix::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FilterColorMatrix));
    static bool typeInited = false;
    static GC_descr descr;
    if (typeInited == false) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(FilterColorMatrix)] = { 0 };
        STARFISH_ASSERT(obj_bitmap != nullptr);
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(FilterColorMatrix));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void FilterColorMatrix::apply(size_t x, size_t y, size_t width, size_t height,
                              Filter::FilterApplyContext& ctx)
{
    STARFISH_ASSERT(element()->isSVGFEColorMatrixElement());
    SVGFEColorMatrixElement* ele = element()->asSVGFEColorMatrixElement();
    String* sourceNameStr = ele->in1()->baseVal();
    auto inputSource = ctx.sourceGraphic();

    if (inputSource->size() == 0) {
        return;
    }

    SVGNumberList* values = ele->values()->baseVal();

    if (values && ele->type()->baseVal() ==
                      SVGFEColorMatrixElement::MatrixTypes::
                          SVG_FECOLORMATRIX_TYPE_MATRIX) {
        if (values->length() == 20) {
            STARFISH_ASSERT(values->length() == 20);
            std::array<float, 20> matrix;
            for (size_t i = 0; i < values->length(); i++) {
                matrix[i] = values->getItem(i)->value();
            }
            applyMatrix(inputSource->data(), matrix, ctx.stride, ctx.width,
                        ctx.height);
        }
    } else if (values && ele->type()->baseVal() ==
                             SVGFEColorMatrixElement::MatrixTypes::
                                 SVG_FECOLORMATRIX_TYPE_SATURATE) {
        if (values->length() == 1) {
            applySaturateAndHueRotate(
                inputSource->data(),
                saturationMatrix(values->getItem(0)->value()), ctx.stride,
                ctx.width, ctx.height);
        }
    } else if (values && ele->type()->baseVal() ==
                             SVGFEColorMatrixElement::MatrixTypes::
                                 SVG_FECOLORMATRIX_TYPE_HUEROTATE) {
        if (values->length() == 1) {
            applySaturateAndHueRotate(
                inputSource->data(),
                hueRotateMatrix(values->getItem(0)->value()), ctx.stride,
                ctx.width, ctx.height);
        }

    } else if (ele->type()->baseVal() ==
               SVGFEColorMatrixElement::MatrixTypes::
                   SVG_FECOLORMATRIX_TYPE_LUMINANCETOALPHA) {
        applyLuminanceAlpha(inputSource->data(), ctx.stride, ctx.width,
                            ctx.height);
    } else {
        return;
    }
}

} // namespace Starfish
