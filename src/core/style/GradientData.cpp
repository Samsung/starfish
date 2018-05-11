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
#include "core/style/Style.h"
#include "core/style/CalcData.h"
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
    if (colorStops.front()->offset().percent() < 0 ||
        colorStops.back()->offset().percent() > 1)
        return true;

    return false;
}

static bool normalizeAndAddStops(GCVector<ColorStop*>& colorStops)
{
    const float firstOffset = colorStops.front()->offset().percent();
    const float lastOffset = colorStops.back()->offset().percent();
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
            (colorStops[i]->offset().percent() - firstOffset) / span;
        colorStops[i]->setOffset(
            Length(Length::Type::Percent, normalizedOffset));
    }

    return true;
}

static float positionFromSideValue(const Unit::Rect& rect, FrameBox* owner,
                                   const SideValue side, Length offset,
                                   bool isHorizontal)
{
    float origin = isHorizontal ? rect.x() : rect.y();
    int sign = 1;
    float edgeDistance = isHorizontal ? rect.width() : rect.height();
    float x = rect.x();
    float y = rect.y();
    // In this case the center of the gradient is given relative to an edge in
    // the form of: [ top | bottom | right | left ] [ <percentage> | <length> ].
    if (offset.isAuto() && side != SideValue::NoneSideValue) {
        switch (side) {
        case SideValue::TopSideValue:
            STARFISH_ASSERT(!isHorizontal);
            return y;
        case SideValue::LeftSideValue:
            STARFISH_ASSERT(isHorizontal);
            return x;
        case SideValue::BottomSideValue:
            STARFISH_ASSERT(!isHorizontal);
            return y + rect.height();
        case SideValue::RightSideValue:
            STARFISH_ASSERT(isHorizontal);
            return x + rect.width();
        case SideValue::CenterSideValue: {
            if (isHorizontal) {
                return x + sign * .5f * edgeDistance;
            } else {
                return y + sign * .5f * edgeDistance;
            }
        }
        default:
            STARFISH_ASSERT_NOT_REACHED();
            break;
        }
    } else if (!offset.isAuto() && side != SideValue::NoneSideValue) {
        if (side == SideValue::RightSideValue ||
            side == SideValue::CenterSideValue) {
            // For right/bottom, the offset is relative to the far edge.
            origin = x + edgeDistance;
            sign = -1;
        }
    }

    if (offset.isPercent()) {
        return origin + sign * offset.percent() * edgeDistance;
    } else if (offset.isCalc() && owner->node()) {
        return origin +
               sign *
                   offset.calcData()->specifiedValue(edgeDistance,
                                                     owner->node());
    }

    return origin + sign * offset.specifiedValue(edgeDistance, owner);
}

void GradientData::setHorizontalSide(SideValue side)
{
    STARFISH_ASSERT(side == SideValue::CenterSideValue ||
                    side == SideValue::LeftSideValue ||
                    side == SideValue::RightSideValue);
    m_horizentalSide = side;
}

void GradientData::setVerticalSide(SideValue side)
{
    STARFISH_ASSERT(side == SideValue::CenterSideValue ||
                    side == SideValue::TopSideValue ||
                    side == SideValue::BottomSideValue);
    m_verticalSide = side;
}

LinearGradientData* GradientData::asLinearGradientData()
{
    STARFISH_ASSERT(m_type == GradientType::LinearGradient);
    return (LinearGradientData*)this;
}

RadialGradientData* GradientData::asRadialGradientData()
{
    STARFISH_ASSERT(m_type == GradientType::RadialGradient);
    return (RadialGradientData*)this;
}

void GradientData::checkComputed(Length curFontSize, Length rootFontSize,
                                 Font* font, LayoutSize windowSize,
                                 ComputedStyle* cs)
{
    for (auto item : m_colorStopList) {
        auto v = item->offset();
        v.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                windowSize.width(), windowSize.height(), cs);
        item->setOffset(v);
    }
}

LinearGradientData::LinearGradientData(float angleDeg)
    : GradientData(GradientType::LinearGradient)
    , m_angleDeg(angleDeg)
{
}

void GradientData::makeSpecifiedColorStops(GCVector<ColorStop*>& out, float& x1,
                                           float& y1, float& r1, float& x2,
                                           float& y2, float& r2,
                                           FrameBox* owner)
{
    float gradientLength = 0.0f;
    if (m_type == GradientType::LinearGradient) {
        gradientLength = hypotf(x2 - x1, y2 - y1);
    } else {
        gradientLength = r2;
    }

    size_t size = m_colorStopList.size();
    out.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        auto item = m_colorStopList[i];

        ColorStop* cs = new ColorStop();

        cs->setColor(item->color());

        const auto& offset = item->offset();

        if (offset.isAuto()) {
            // If the first color-stop does not have a position, set its
            // position to 0%. If the last color-stop does not have a position,
            // set its position to 100%.
            if (i == 0) {
                cs->setOffset(Length(Length::Type::Percent, 0.0f));
                cs->setSpecified(true);
            } else if (i == size - 1) {
                cs->setOffset(Length(Length::Type::Percent, 1.0f));
                cs->setSpecified(true);
            }
        } else if (offset.isPercent()) {
            cs->setOffset(offset);
            cs->setSpecified(true);
        } else {
            float length = offset.specifiedValue(gradientLength, owner);
            length = (gradientLength > 0) ? length / gradientLength : 0;
            cs->setOffset(Length(Length::Type::Percent, length));
            cs->setSpecified(true);
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
            if (cs->offset().percent() <
                out[prevSpecifiedIndex]->offset().percent()) {
                cs->setOffset(
                    Length(Length::Type::Percent,
                           out[prevSpecifiedIndex]->offset().percent()));
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
                    float lastSpecifiedOffset =
                        out[unspecifiedRunStart - 1]->offset().percent();
                    float next_specified_offset =
                        out[unspecifiedRunEnd]->offset().percent();
                    float delta =
                        (next_specified_offset - lastSpecifiedOffset) /
                        (unspecifiedRunEnd - unspecifiedRunStart + 1);

                    for (size_t j = unspecifiedRunStart; j < unspecifiedRunEnd;
                         ++j)
                        out[j]->setOffset(
                            Length(Length::Type::Percent,
                                   lastSpecifiedOffset +
                                       (j - unspecifiedRunStart + 1) * delta));
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

    if (m_type == GradientType::LinearGradient) {
        float firstOffset = out.front()->offset().percent();
        float lastOffset = out.back()->offset().percent();
        if (normalizeAndAddStops(out)) {
            float dx = x2 - x1;
            float dy = y2 - y1;
            x2 = x1 + dx * lastOffset;
            y2 = y1 + dy * lastOffset;
            x1 = x1 + dx * firstOffset;
            y1 = y1 + dy * firstOffset;
        }
    } else {
        float firstOffset = out.front()->offset().percent();
        float lastOffset = out.back()->offset().percent();
        if (normalizeAndAddStops(out)) {
            // Radial offsets are relative to the [0 , endRadius] segment.
            float adjustedr1 = r2 * firstOffset;
            float adjustedr2 = r2 * lastOffset;
            // Unlike linear gradients (where we can adjust the points
            // arbitrarily), we cannot let our radii turn negative here.
            if (adjustedr2 < 0) {
                // For the non-repeat case, this can never happen:
                // clampNegativeOffsets() ensures we don't have to deal with
                // negative offsets at this point.

                // When in repeat mode, we deal with it by repositioning both
                // radii in the positive domain - shifting them by a multiple of
                // the radius span (which is the period of our repeating
                // gradient -> hence no visible side effects).
                const float radiusSpan = adjustedr2 - adjustedr1;
                const float shiftToPositive =
                    radiusSpan * ceilf(-adjustedr1 / radiusSpan);
                adjustedr1 += shiftToPositive;
                adjustedr2 += shiftToPositive;
            }
            r1 = adjustedr1;
            r2 = adjustedr2;
        }
    }
}

bool LinearGradientData::computeEndPointsFromAngle(const Unit::Rect& rect,
                                                   const float angleDeg,
                                                   float& x1, float& y1,
                                                   float& x2, float& y2)
{
    int x = rect.x();
    int y = rect.y();
    int maxX = rect.maxX();
    int maxY = rect.maxY();

    float angle = fmodf(angleDeg, 360);

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
}

bool LinearGradientData::computeEndPoints(const Unit::Rect& rect, float& x1,
                                          float& y1, float& x2, float& y2)
{
    x1 = y1 = x2 = y2 = 0.0f;
    if (m_horizentalSide == SideValue::NoneSideValue &&
        m_verticalSide == SideValue::NoneSideValue) {
        return computeEndPointsFromAngle(rect, m_angleDeg, x1, y1, x2, y2);
    } else {
        if (m_horizentalSide != SideValue::NoneSideValue &&
            m_verticalSide != SideValue::NoneSideValue) {
            float rise = rect.width();
            float run = rect.height();
            if (m_horizentalSide == SideValue::LeftSideValue) {
                run *= -1;
            }
            if (m_verticalSide == SideValue::BottomSideValue) {
                rise *= -1;
            }
            float angleDeg = 90 - convertFromRadToDeg(atan2(rise, run));

            return computeEndPointsFromAngle(rect, angleDeg, x1, y1, x2, y2);

        } else if (m_horizentalSide != SideValue::NoneSideValue ||
                   m_verticalSide != SideValue::NoneSideValue) {
            float angleDeg = 0;
            if (m_horizentalSide == SideValue::RightSideValue) {
                angleDeg = 90;
            } else if (m_verticalSide == SideValue::BottomSideValue) {
                angleDeg = 180;
            } else if (m_horizentalSide == SideValue::LeftSideValue) {
                angleDeg = 270;
            }
            return computeEndPointsFromAngle(rect, angleDeg, x1, y1, x2, y2);
        }
        return false;
    }
}

void GradientData::convertColorStopsToCSSColorStops(
    GCVector<CSSColorStop*>& out)
{
    for (auto item : m_colorStopList) {
        CSSColorStop* cs = new CSSColorStop();

        CSSStyleValuePair color;
        color.setColorValue(item->color());
        cs->setColor(color);

        if (item->offset().isAuto()) {
            CSSStyleValuePair offset;
            cs->setOffset(offset);
        } else {
            CSSStyleValuePair offset =
                (CSSStyleDeclaration::lengthToCSSStyleValue(item->offset()));
            cs->setOffset(offset);
        }

        out.push_back(cs);
    }
}

GradientDrawingInfo* LinearGradientData::makeGradientDrawingInfo(
    const Unit::Rect& rect, FrameBox* box)
{
    STARFISH_ASSERT(box);

    GradientDrawingInfo* ret = new GradientDrawingInfo();
    computeEndPoints(rect, ret->x1, ret->y1, ret->x2, ret->y2);
    makeSpecifiedColorStops(ret->colorStops, ret->x1, ret->y1, ret->r1, ret->x2,
                            ret->y2, ret->r2, box);
    return ret;
}

CSSGradientValue* LinearGradientData::convertToCSSGradientValue()
{
    CSSLinearGradientValue* gradient = new CSSLinearGradientValue();

    if (m_horizentalSide != SideValue::NoneSideValue ||
        m_verticalSide != SideValue::NoneSideValue) {
        if (m_horizentalSide != SideValue::NoneSideValue) {
            CSSStyleValuePair leftOrRight;
            leftOrRight.setValueKind(
                CSSStyleValuePair::ValueKind::SideValueKind);
            leftOrRight.setValue(m_horizentalSide);
            gradient->setLeftOrRight(leftOrRight);
        }
        if (m_verticalSide != SideValue::NoneSideValue) {
            CSSStyleValuePair topOrBottom;
            topOrBottom.setValueKind(
                CSSStyleValuePair::ValueKind::SideValueKind);
            topOrBottom.setValue(m_verticalSide);
            gradient->setTopOrBottom(topOrBottom);
        }
    } else {
        gradient->setAngle(CSSAngle(m_angleDeg));
    }

    convertColorStopsToCSSColorStops(gradient->cssColorStopList());

    return gradient;
}

void LinearGradientData::checkComputed(Length curFontSize, Length rootFontSize,
                                       Font* font, LayoutSize windowSize,
                                       ComputedStyle* cs)
{
    GradientData::checkComputed(curFontSize, rootFontSize, font, windowSize,
                                cs);
}

RadialGradientData::RadialGradientData()
    : GradientData(GradientType::RadialGradient)
    , m_shape(RadialGradientShape::None)
    , m_gradientSizeKeyword(RadialGradientSizeKeyword::None)
{
}

GradientDrawingInfo* RadialGradientData::makeGradientDrawingInfo(
    const Unit::Rect& rect, FrameBox* box)
{
    STARFISH_ASSERT(box);

    GradientDrawingInfo* ret = new GradientDrawingInfo();
    computeEndPoints(rect, box, ret->x1, ret->y1, ret->r1, ret->x2, ret->y2,
                     ret->r2, ret->firstRadius, ret->secondRadius);
    makeSpecifiedColorStops(ret->colorStops, ret->x1, ret->y1, ret->r1, ret->x2,
                            ret->y2, ret->r2, box);
    return ret;
}

CSSGradientValue* RadialGradientData::convertToCSSGradientValue()
{
    CSSRadialGradientValue* gradient = new CSSRadialGradientValue();

    // Position of gradient center
    // Note : Current background-position implementations can not be processed
    //        if they are 3 to 4 in length.
    CSSStyleValuePair xlist;
    xlist.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    xlist.setValueList(new ValueList(ValueList::Separator::SpaceSeparator));
    if (m_horizentalSide != SideValue::NoneSideValue) {
        CSSStyleValuePair x;
        x.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
        x.setValue(m_horizentalSide);
        xlist.multiValue()->push_back(x);
    } else if (!m_horizentalSideOffset.isAuto()) {
        CSSStyleValuePair x =
            CSSStyleDeclaration::lengthToCSSStyleValue(m_horizentalSideOffset);
        xlist.multiValue()->push_back(x);
    }
    gradient->setPositionX(xlist);

    CSSStyleValuePair ylist;
    ylist.setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    ylist.setValueList(new ValueList(ValueList::Separator::SpaceSeparator));
    if (m_verticalSide != SideValue::NoneSideValue) {
        CSSStyleValuePair y;
        y.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
        y.setValue(m_verticalSide);
        ylist.multiValue()->push_back(y);
    } else if (!m_verticalSideOffset.isAuto()) {
        CSSStyleValuePair y =
            CSSStyleDeclaration::lengthToCSSStyleValue(m_verticalSideOffset);
        ylist.multiValue()->push_back(y);
    }
    gradient->setPositionY(ylist);

    // Shape
    gradient->setShape(m_shape);

    CSSRadialGradientSize size;
    // Size of the gradient's ending shape
    if (m_gradientSizeKeyword != RadialGradientSizeKeyword::None) {
        size.setKeyword(m_gradientSizeKeyword);
    } else {
        if (!m_firstRadius.isAuto()) {
            size.setFirstRadius(
                CSSStyleDeclaration::lengthToCSSStyleValue(m_firstRadius));
            if (!m_secondRadius.isAuto()) {
                size.setSecondRadius(
                    CSSStyleDeclaration::lengthToCSSStyleValue(m_secondRadius));
            }
        }
    }
    gradient->setSize(size);

    convertColorStopsToCSSColorStops(gradient->cssColorStopList());
    return gradient;
}

void RadialGradientData::checkComputed(Length curFontSize, Length rootFontSize,
                                       Font* font, LayoutSize windowSize,
                                       ComputedStyle* cs)
{
    GradientData::checkComputed(curFontSize, rootFontSize, font, windowSize,
                                cs);

    m_horizentalSideOffset.changeToFixedIfNeeded(curFontSize, rootFontSize,
                                                 font, windowSize.width(),
                                                 windowSize.height(), cs);
    m_verticalSideOffset.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                               windowSize.width(),
                                               windowSize.height(), cs);
    m_firstRadius.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                        windowSize.width(), windowSize.height(),
                                        cs);
    m_secondRadius.changeToFixedIfNeeded(curFontSize, rootFontSize, font,
                                         windowSize.width(),
                                         windowSize.height(), cs);
}

static float resolveRadius(FrameBox* owner, const Length& radius,
                           const float& widthOrHeight)
{
    float ret;
    if (radius.isPercent()) {
        ret = widthOrHeight * radius.percent();
    } else {
        ret = radius.specifiedValue(0, owner);
    }
    return clampTo<float>(std::max(ret, 0.0f));
}

void RadialGradientData::radiusToSide(const float x2, const float y2,
                                      const Unit::Rect& rect,
                                      bool (*compare)(float, float), float& r1,
                                      float& r2)
{
    float x = x2 - rect.x();
    float y = y2 - rect.y();
    float dx1 = clampTo<float>(fabs(x));
    float dy1 = clampTo<float>(fabs(y));
    float dx2 = clampTo<float>(fabs(x - rect.width()));
    float dy2 = clampTo<float>(fabs(y - rect.height()));

    float dx = compare(dx1, dx2) ? dx1 : dx2;
    float dy = compare(dy1, dy2) ? dy1 : dy2;

    if (m_shape == RadialGradientShape::Circle) {
        compare(dx, dy) ? r1 = dx, r2 = dx : r1 = dy, r2 = dy;
    } else {
        r1 = dx, r2 = dy;
    }
}

// Compute the radius of an ellipse with center at 0,0 which passes through p,
// and has width/height given by aspectRatio.
inline static void ellipseRadius(const float& x, const float& y,
                                 const float& aspectRatio, float& dx, float& dy)
{
    if (aspectRatio == 0 || std::isinf(aspectRatio)) {
        dx = dy = 0;
    }
    // x^2/a^2 + y^2/b^2 = 1
    // a/b = aspectRatio, b = a/aspectRatio
    // a = sqrt(x^2 + y^2/(1/r^2))
    float a = sqrtf(x * x + y * y * aspectRatio * aspectRatio);
    dx = clampTo<float>(a);
    dy = clampTo<float>(a / aspectRatio);
}

// Compute the radius to the closest/farthest corner (depending on the compare
// functor).
void RadialGradientData::radiusToCorner(const float x2, const float y2,
                                        const Unit::Rect& rect,
                                        bool (*compare)(float, float),
                                        float& r1, float& r2)
{
    struct point {
        float x;
        float y;
    } coners[4];
    coners[0] = { rect.x(), rect.y() };
    coners[1] = { rect.x() + rect.width(), rect.y() };
    coners[2] = { rect.x() + rect.width(), rect.y() + rect.height() };
    coners[3] = { rect.x(), rect.y() + rect.height() };

    unsigned cornerIndex = 0;
    float distance =
        hypotf(x2 - coners[cornerIndex].x, y2 - coners[cornerIndex].y);
    for (unsigned i = 1; i < 4; ++i) {
        float newDistance = hypotf(x2 - coners[i].x, y2 - coners[i].y);
        if (compare(newDistance, distance)) {
            cornerIndex = i;
            distance = newDistance;
        }
    }

    if (m_shape == RadialGradientShape::Circle) {
        r1 = r2 = distance;
    } else {
        float tdx = 0, tdy = 0;
        radiusToSide(x2, y2, rect, compare, tdx, tdy);
        ellipseRadius(coners[cornerIndex].x - x2, coners[cornerIndex].y - y2,
                      tdx / tdy, r1, r2);
    }
}

bool RadialGradientData::computeEndPoints(const Unit::Rect& rect,
                                          FrameBox* owner, float& x1, float& y1,
                                          float& r1, float& x2, float& y2,
                                          float& r2, float& firstRadius,
                                          float& secondRadius)
{
    int x = rect.x();
    int y = rect.y();
    r1 = 0;
    if (m_verticalSide == SideValue::NoneSideValue &&
        m_verticalSideOffset.isAuto() &&
        m_horizentalSide == SideValue::NoneSideValue &&
        m_horizentalSideOffset.isAuto()) {
        x1 = x + rect.width() / 2;
        y1 = y + rect.height() / 2;
    } else {
        computeEndPointsFromSideValue(rect, owner, x1, y1);
    }
    x2 = x1;
    y2 = y1;

    firstRadius = 0;
    secondRadius = 0;
    if (!m_firstRadius.isAuto()) {
        firstRadius = resolveRadius(owner, m_firstRadius, rect.width());
        if (m_secondRadius.isAuto()) {
            secondRadius = firstRadius;
        } else {
            secondRadius = resolveRadius(owner, m_secondRadius, rect.height());
        }
    } else {
        switch (m_gradientSizeKeyword) {
        case RadialGradientSizeKeyword::ClosetSide:
            radiusToSide(x2, y2, rect, [](float a, float b) { return a < b; },
                         firstRadius, secondRadius);
            break;
        case RadialGradientSizeKeyword::FarthestSide:
            radiusToSide(x2, y2, rect, [](float a, float b) { return a > b; },
                         firstRadius, secondRadius);
            break;
        case RadialGradientSizeKeyword::ClosetCorner:
            radiusToCorner(x2, y2, rect, [](float a, float b) { return a < b; },
                           firstRadius, secondRadius);
            break;
        case RadialGradientSizeKeyword::FarthestCorner:
        default:
            radiusToCorner(x2, y2, rect, [](float a, float b) { return a > b; },
                           firstRadius, secondRadius);
            break;
        }
    }

    (!firstRadius || !secondRadius) ? r2 = 0 : r2 = firstRadius;
    return true;
}

void RadialGradientData::computeEndPointsFromSideValue(const Unit::Rect& rect,
                                                       FrameBox* owner,
                                                       float& x, float& y)
{
    x = positionFromSideValue(rect, owner, m_horizentalSide,
                              m_horizentalSideOffset, true);
    y = positionFromSideValue(rect, owner, m_verticalSide, m_verticalSideOffset,
                              false);
}

bool GradientData::equals(GradientData* other) const
{
    if ((m_type != other->m_type) ||
        (m_horizentalSide != other->m_horizentalSide) ||
        (m_verticalSide != other->m_verticalSide)) {
        return false;
    }

    if (m_colorStopList.size() != other->m_colorStopList.size()) {
        return false;
    }

    for (size_t i = 0; i < m_colorStopList.size(); ++i) {
        if (!(m_colorStopList[i]->equals(other->m_colorStopList[i]))) {
            return false;
        }
    }

    return true;
}

bool LinearGradientData::equals(GradientData* other) const
{
    if (!GradientData::equals(other)) {
        return false;
    }

    LinearGradientData* r = other->asLinearGradientData();
    if ((m_angleDeg != r->m_angleDeg)) {
        return false;
    }
    return true;
}

bool RadialGradientData::equals(GradientData* other) const
{
    if (!GradientData::equals(other)) {
        return false;
    }
    RadialGradientData* r = other->asRadialGradientData();
    if ((m_shape != r->m_shape) ||
        (m_horizentalSideOffset != r->m_horizentalSideOffset) ||
        (m_verticalSideOffset != r->m_verticalSideOffset) ||
        (m_firstRadius != r->m_firstRadius) ||
        (m_secondRadius != r->m_secondRadius) ||
        (m_gradientSizeKeyword != r->m_gradientSizeKeyword)) {
        return false;
    }
    return true;
}
}
