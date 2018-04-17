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

/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2015 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StarFishConfig.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSGradientValue.h"
#include "core/layout/FrameBox.h"
#include "core/style/GradientData.h"
namespace StarFish {

static bool requiresStopsNormalization(GCVector<ColorStop*>& colorStops)
{
    // We need at least two stops to normalize
    if (colorStops.size() < 2)
        return false;

    // Repeating gradients are implemented using a normalized stop offset range
    // with the point/radius pairs aligned on the interval endpoints.
    // if (desc.spread_method == kSpreadMethodRepeat)
    //     return true;

    // Degenerate stops
    if (colorStops.front()->offset().percentageValue() < 0 ||
        colorStops.back()->offset().percentageValue() > 1)
        return true;

    return false;
}

static bool normalizeAndAddStops(GCVector<ColorStop*>& colorStops)
{
    const float firstOffset = colorStops.front()->offset().percentageValue();
    const float lastOffset = colorStops.back()->offset().percentageValue();
    const float span = lastOffset - firstOffset;

    if (fabs(span) < std::numeric_limits<float>::epsilon()) {
        // All stops are coincident -> use a single clamped offset value.
        const float clamped_offset = std::min(std::max(firstOffset, 0.f), 1.f);

        // For repeating gradients, a coincident stop set defines a solid-color
        // image with the color of the last color-stop in the rule.
        // For non-repeating gradients, both the first color and the last color
        // can be significant (padding on both sides of the offset).

        // if (desc.spread_method != kSpreadMethodRepeat)
        //     desc.stops.emplace_back(clamped_offset, stops.front().color);
        // desc.stops.emplace_back(clamped_offset, stops.back().color);
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }

    for (size_t i = 0; i < colorStops.size(); ++i) {
        const float normalizedOffset =
            (colorStops[i]->offset().percentageValue() - firstOffset) / span;
        colorStops[i]->setOffset(normalizedOffset);
    }

    return true;
}

LinearGradientData* GradientData::asLinearGradientData()
{
    STARFISH_ASSERT(m_type == CSSGradientType::LinearGradient);
    return (LinearGradientData*)this;
}

LinearGradientData::LinearGradientData(float angleDeg)
    : GradientData(CSSGradientType::LinearGradient)
    , m_angleDeg(angleDeg)
    , m_sc(0)
{
}

void GradientData::makeSpecifiedColorStops(GCVector<ColorStop*>& out, float& x1,
                                           float& y1, float& x2, float& y2,
                                           FrameBox* owner)
{
    float gradientLength = 0.0f;
    if (m_type == CSSGradientType::LinearGradient) {
        gradientLength = hypotf(x2 - x1, y2 - y1);
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    size_t size = m_colorStopList.size();
    out.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        auto item = m_colorStopList[i];

        ColorStop* cs = new ColorStop();

        cs->setColor(item->color());

        const auto& offset = item->offset();

        if (offset.type().isPercentage()) {
            cs->setOffset(offset.percentageValue());
            cs->setSpecified(true);
        } else if (offset.type().isLength()) {
            float length =
                offset.lengthValue().specifiedValue(gradientLength, owner);
            length = (gradientLength > 0) ? length / gradientLength : 0;
            cs->setOffset(length);
            cs->setSpecified(true);
        } else {
            // If the first color-stop does not have a position, set its
            // position to 0%. If the last color-stop does not have a position,
            // set its position to 100%.
            if (i == 0) {
                cs->setOffset(0.0f);
                cs->setSpecified(true);
            } else if (i == size - 1) {
                cs->setOffset(1.0f);
                cs->setSpecified(true);
            }
        }

        // If a color-stop has a position that is less than the specified
        // position of any color-stop before it in the list, set its position to
        // be equal to the largest specified position of any color-stop before
        // it.
        if (cs->specified() && i > 0) {
            size_t prevSpecifiedIndex;
            for (prevSpecifiedIndex = i - 1; prevSpecifiedIndex;
                 --prevSpecifiedIndex) {
                if (out[prevSpecifiedIndex]->specified()) {
                    break;
                }
            }
            if (cs->offset().percentageValue() <
                out[prevSpecifiedIndex]->offset().percentageValue()) {
                cs->setOffset(
                    out[prevSpecifiedIndex]->offset().percentageValue());
            }
        }
        out.push_back(cs);
    }

    STARFISH_ASSERT(out.front()->specified());
    STARFISH_ASSERT(out.back()->specified());
    STARFISH_ASSERT(out.size() == m_colorStopList.size());

    // If any color-stop still does not have a position, then, for each run of
    // adjacent color-stops without positions, set their positions so that they
    // are evenly spaced between the preceding and following color-stops with
    // positions.
    if (size > 2) {
        size_t unspecifiedRunStart = 0;
        bool inUnspecifiedRun = false;

        for (size_t i = 0; i < size; ++i) {
            if (!out[i]->specified() && !inUnspecifiedRun) {
                unspecifiedRunStart = i;
                inUnspecifiedRun = true;
            } else if (out[i]->specified() && inUnspecifiedRun) {
                size_t unspecifiedRunEnd = i;

                if (unspecifiedRunStart < unspecifiedRunEnd) {
                    float lastSpecifiedOffset = out[unspecifiedRunStart - 1]
                                                    ->offset()
                                                    .percentageValue();
                    float next_specified_offset =
                        out[unspecifiedRunEnd]->offset().percentageValue();
                    float delta =
                        (next_specified_offset - lastSpecifiedOffset) /
                        (unspecifiedRunEnd - unspecifiedRunStart + 1);

                    for (size_t j = unspecifiedRunStart; j < unspecifiedRunEnd;
                         ++j)
                        out[j]->setOffset(lastSpecifiedOffset +
                                          (j - unspecifiedRunStart + 1) *
                                              delta);
                }
                inUnspecifiedRun = false;
            }
        }
    }

    // At this point we have a fully resolved set of stops. Time to perform
    // adjustments for repeat gradients and degenerate values if needed.
    if (!requiresStopsNormalization(out)) {
        return;
    }

    if (m_type == CSSGradientType::LinearGradient) {
        float firstOffset = out.front()->offset().percentageValue();
        float lastOffset = out.back()->offset().percentageValue();
        if (normalizeAndAddStops(out)) {
            float dx = x2 - x1;
            float dy = y2 - y1;
            x2 = x1 + dx * lastOffset;
            y2 = y1 + dy * lastOffset;
            x1 = x1 + dx * firstOffset;
            y1 = y1 + dy * firstOffset;
        }

    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

bool LinearGradientData::computeEndPoints(const Unit::Rect& rect, float& x1,
                                          float& y1, float& x2, float& y2)
{
    int x = rect.x();
    int y = rect.y();
    int maxX = rect.maxX();
    int maxY = rect.maxY();

    if (m_sc == 0) {
        float angle = fmodf(m_angleDeg, 360);
        if (angle < 0)
            angle += 360;

        if (!angle) {
            x1 = x;
            y1 = maxY;
            x2 = x;
            y2 = y;
            return true;
        }

        if (angle == 90) {
            x1 = x;
            y1 = y;

            x2 = maxX;
            y2 = y;
            return true;
        }

        if (angle == 180) {
            x1 = x;
            y1 = y;
            x2 = x;
            y2 = maxY;
            return true;
        }

        if (angle == 270) {
            x1 = maxX;
            y1 = y;
            x2 = x;
            y2 = y;
            return true;
        }

        float slope = tan(convertFromDegToRad(90 - angle));

        float perpendicularSlope = -1 / slope;

        float halfHeight = rect.height() / 2;
        float halfWidth = rect.width() / 2;

        float cx, cy;

        if (angle < 90) {
            cx = halfWidth;
            cy = halfHeight;
        } else if (angle < 180) {
            cx = halfWidth;
            cy = -halfHeight;
        } else if (angle < 270) {
            cx = -halfWidth;
            cy = -halfHeight;
        } else {
            cx = -halfWidth;
            cy = halfHeight;
        }

        // Compute c (of y = mx + c) using the corner point.
        float c = cy - perpendicularSlope * cx;
        float ex = c / (slope - perpendicularSlope);
        float ey = perpendicularSlope * ex + c;

        x2 = x + halfWidth + ex;
        y2 = y + halfHeight - ey;

        x1 = x + halfWidth - ex;
        y1 = y + halfHeight + ey;
        return true;
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }
}

CSSGradientValue* LinearGradientData::convertToCSSGradientValue()
{
    CSSLinearGradientValue* gradient = new CSSLinearGradientValue();

    if (m_sc == 0) {
        gradient->setAngle(CSSAngle(m_angleDeg));
    } else {
        gradient->setSideOrConter(m_sc);
    }

    auto& cssColorStopList = gradient->cssColorStopList();

    for (auto item : m_colorStopList) {
        CSSColorStop* cs = new CSSColorStop();

        CSSStyleValuePair color;
        color.setColorValue(item->color());
        cs->setColor(color);

        if (item->offset().type().isPercentage()) {
            CSSStyleValuePair offset;
            offset.setPercentageValue(item->offset().percentageValue());
            cs->setOffset(offset);
        } else if (item->offset().type().isLength()) {
            CSSStyleValuePair offset =
                (CSSStyleDeclaration::lengthToCSSStyleValue(
                    item->offset().lengthValue()));
            cs->setOffset(offset);
        } else if (item->offset().type().isNone()) {
            CSSStyleValuePair offset;
            cs->setOffset(offset);
        }

        cssColorStopList.push_back(cs);
    }

    return gradient;
}

void LinearGradientData::checkComputed(Length curFontSize, Length rootFontSize,
                                       Font* font, LayoutSize windowSize,
                                       ComputedStyle* cs)
{
    for (auto item : m_colorStopList) {
        auto offset = item->offset();
        if (offset.type().isLength()) {
            auto v = offset.lengthValue();
            v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                    windowSize.width(), windowSize.height(),
                                    cs);
            item->setOffset(v);
        }
    }
}
}
