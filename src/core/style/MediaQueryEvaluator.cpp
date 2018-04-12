/*
 * CSS Media Query Evaluator
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
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
/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "core/layout/LayoutUtil.h"
#include "core/style/CSSParser.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryResult.h"
#include "core/style/MediaValues.h"
#include "core/style/UnitHelper.h"

namespace StarFish {

enum MediaFeaturePrefix { NoPrefix, MinPrefix, MaxPrefix };

bool MediaQueryEvaluator::mediaTypeMatch(String* mediaTypeToMatch) const
{
    // If the m_mediaType is an empty string, it means that we support all media
    // types.
    return m_mediaType->equals(String::emptyString) ||
           mediaTypeToMatch->equals(String::emptyString) ||
           mediaTypeToMatch->equalsIgnoreCase("all") ||
           mediaTypeToMatch->equals(m_mediaType);
}

static bool applyRestrictor(MediaQuery::RestrictorType type, bool value)
{
    return type == MediaQuery::Not ? !value : value;
}

bool MediaQueryEvaluator::eval(
    MediaQuerySet* mediaQueries, MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult) const
{
    auto queries = mediaQueries->queryVector();
    if (!queries.size()) {
        // Empty query list evaluates to true.
        return true;
    }

    // Iterate over queries, stop if any of them eval to true.
    auto iter = queries.begin();
    while (iter != queries.end()) {
        if (eval(*iter, viewportDependentResult, deviceDependentResult)) {
            return true;
        }
        iter++;
    }

    return false;
}

bool MediaQueryEvaluator::eval(
    MediaQuery* query, MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult) const
{
    if (!mediaTypeMatch(query->mediaType())) {
        return applyRestrictor(query->restrictor(), false);
    }

    auto expressions = query->expressions();
    size_t i = 0;
    for (; i < expressions.size(); ++i) {
        bool result = eval(expressions[i]);
        if (viewportDependentResult && expressions[i]->isViewportDependent()) {
            viewportDependentResult->push_back(
                new MediaQueryResult(expressions[i], result));
        }
        if (deviceDependentResult && expressions[i]->isDeviceDependent()) {
            deviceDependentResult->push_back(
                new MediaQueryResult(expressions[i], result));
        }
        if (!result) {
            break;
        }
    }

    // Assume true if we are at the end of the list, otherwise assume false.
    return applyRestrictor(query->restrictor(), expressions.size() == i);
}

static bool isLength(UnitType type)
{
    return type >= UnitType::Ems && type <= UnitType::UserUnits;
}

static bool isResolution(UnitType type)
{
    return type >= UnitType::DotsPerPixel &&
           type <= UnitType::DotsPerCentimeter;
}

double conversionToCanonicalUnitsScaleFactor(UnitType unitType)
{
    double factor = 1.0;
    switch (unitType) {
    // These are "canonical" units in their respective categories.
    case UnitType::Pixels:
    case UnitType::UserUnits:
    case UnitType::Degrees:
    case UnitType::Milliseconds:
    case UnitType::Hertz:
        break;
    case UnitType::Centimeters:
        factor = unitPxPerCm;
        break;
    case UnitType::DotsPerCentimeter:
        factor = 1 / unitPxPerCm;
        break;
    case UnitType::Millimeters:
        factor = unitPxPerMm;
        break;
    case UnitType::Inches:
        factor = unitPxPerIn;
        break;
    case UnitType::DotsPerInch:
        factor = 1 / unitPxPerIn;
        break;
    case UnitType::Points:
        factor = unitPxPerPt;
        break;
    case UnitType::Picas:
        factor = unitPxPerPc;
        break;
    case UnitType::Radians:
        factor = 180 / M_PI;
        break;
    case UnitType::Gradians:
        factor = 0.9;
        break;
    case UnitType::Turns:
        factor = 360;
        break;
    case UnitType::Seconds:
    case UnitType::Kilohertz:
        factor = 1000;
        break;
    default:
        break;
    }

    return factor;
}

static bool computeLength(MediaQueryExpValue& value, MediaValues* mediaValues,
                          double& result)
{
    if (!value.isValue) {
        return false;
    }

    if (value.unit == UnitType::Number) {
        result = clampToInteger(value.value);
        return !result;
    } else if (isLength(value.unit)) {
        return mediaValues->computeLength(value.value, value.unit, result);
    }

    return false;
}

template <typename T>
bool compareValue(T a, T b, MediaFeaturePrefix op)
{
    switch (op) {
    case MinPrefix:
        return a >= b;
    case MaxPrefix:
        return a <= b;
    case NoPrefix:
        return a == b;
    }
    return false;
}

bool compareDoubleValue(double a, double b, MediaFeaturePrefix op)
{
    const double precision = std::numeric_limits<double>::epsilon();
    switch (op) {
    case NoPrefix:
        return std::abs(a - b) <= precision;
    case MinPrefix:
        return a >= (b - precision);
    case MaxPrefix:
        return a <= (b + precision);
    }
    return false;
}

static bool computeLengthAndCompare(MediaQueryExpValue& value,
                                    MediaValues* mediaValues,
                                    MediaFeaturePrefix op,
                                    double compareToValue)
{
    double length;
    return computeLength(value, mediaValues, length) &&
           compareDoubleValue(compareToValue, length, op);
}

static bool widthMediaFeatureEval(MediaQueryExpValue& value,
                                  MediaValues* mediaValues,
                                  MediaFeaturePrefix op)
{
    int32_t width = mediaValues->viewportWidth();
    if (value.isValid()) {
        return computeLengthAndCompare(value, mediaValues, op, width);
    }
    return width;
}

static bool heightMediaFeatureEval(MediaQueryExpValue& value,
                                   MediaValues* mediaValues,
                                   MediaFeaturePrefix op)
{
    int32_t height = mediaValues->viewportHeight();
    if (value.isValid()) {
        return computeLengthAndCompare(value, mediaValues, op, height);
    }
    return height;
}

static bool deviceWidthMediaFeatureEval(MediaQueryExpValue& value,
                                        MediaValues* mediaValues,
                                        MediaFeaturePrefix op)
{
    int32_t width = mediaValues->screenWidth();
    if (value.isValid()) {
        return computeLengthAndCompare(value, mediaValues, op, width);
    }
    return width;
}

static bool deviceHeightMediaFeatureEval(MediaQueryExpValue& value,
                                         MediaValues* mediaValues,
                                         MediaFeaturePrefix op)
{
    int32_t height = mediaValues->screenHeight();
    if (value.isValid()) {
        return computeLengthAndCompare(value, mediaValues, op, height);
    }
    return height;
}

static bool orientationMediaFeatureEval(MediaQueryExpValue& value,
                                        MediaValues* mediaValues,
                                        MediaFeaturePrefix op)
{
    int32_t width = mediaValues->viewportWidth();
    int32_t height = mediaValues->viewportHeight();

    // The ‘orientation’ media feature is ‘portrait’ when the value of the
    // ‘height’ media feature is greater than or equal to the value of the
    // ‘width’ media feature. Otherwise ‘orientation’ is ‘landscape’.
    if (value.isID) {
        if (width > height) {
            return value.id->equalsIgnoreCase("landscape");
        } else {
            return value.id->equalsIgnoreCase("portrait");
        }
    }

    // Expression for the orientation evaluates to true if width and height >=
    // 0.
    return width >= 0 && height >= 0;
}

static bool compareAspectRatioValue(MediaQueryExpValue& value, int32_t width,
                                    int32_t height, MediaFeaturePrefix op)
{
    if (value.isRatio) {
        return compareValue(width * static_cast<int32_t>(value.denominator),
                            height * static_cast<int32_t>(value.numerator), op);
    }
    return false;
}

static bool aspectRatioMediaFeatureEval(MediaQueryExpValue& value,
                                        MediaValues* mediaValues,
                                        MediaFeaturePrefix op)
{
    if (value.isValid()) {
        return compareAspectRatioValue(value, mediaValues->viewportWidth(),
                                       mediaValues->viewportHeight(), op);
    }
    // Assume if we have a device, its aspect ratio is non-zero.
    return true;
}

static bool deviceAspectRatioMediaFeatureEval(MediaQueryExpValue& value,
                                              MediaValues* mediaValues,
                                              MediaFeaturePrefix op)
{
    if (value.isValid()) {
        return compareAspectRatioValue(value, mediaValues->screenWidth(),
                                       mediaValues->screenHeight(), op);
    }
    // Assume if we have a device, its device aspect ratio is non-zero.
    return true;
}

static bool numberValue(const MediaQueryExpValue& value, float& result)
{
    if (value.isValue && value.unit == UnitType::Number) {
        result = value.value;
        return true;
    }
    return false;
}

static bool colorMediaFeatureEval(MediaQueryExpValue& value,
                                  MediaValues* mediaValues,
                                  MediaFeaturePrefix op)
{
    float number;
    int32_t colorBitsPerComponent = mediaValues->colorBitsPerComponent();

    if (value.isValid()) {
        return numberValue(value, number) &&
               compareValue(colorBitsPerComponent, static_cast<int32_t>(number),
                            op);
    }

    return colorBitsPerComponent != 0;
}

static bool colorIndexMediaFeatureEval(MediaQueryExpValue& value,
                                       MediaValues* mediaValues,
                                       MediaFeaturePrefix op)
{
    // Assume that we do not support indexed displays because it is unknown how
    // to retrieve the information if the display mode is indexed.
    // This matches Firefox and Chrome.
    if (!value.isValid()) {
        return false;
    }

    // If the device does not use a color lookup table, the value is zero.
    float number;
    return numberValue(value, number) &&
           compareValue(0, static_cast<int32_t>(number), op);
}

static bool monochromeMediaFeatureEval(MediaQueryExpValue& value,
                                       MediaValues* mediaValues,
                                       MediaFeaturePrefix op)
{
    if (!value.isValid()) {
        return false;
    }

    if (!mediaValues->isMonochrome()) {
        float number;
        return numberValue(value, number) &&
               compareValue(0, static_cast<int>(number), op);
    }

    return colorMediaFeatureEval(value, mediaValues, op);
}

static bool evalResolution(MediaQueryExpValue& value, MediaValues* mediaValues,
                           MediaFeaturePrefix op)
{
    float actualResolution = mediaValues->devicePixelRatio();

    if (!value.isValid()) {
        return !!actualResolution;
    }

    if (!value.isValue || !isResolution(value.unit)) {
        return false;
    }

    if (value.unit == UnitType::Number) {
        return compareValue(actualResolution, clampTo<float>(value.value), op);
    }

    double canonicalFactor = conversionToCanonicalUnitsScaleFactor(value.unit);
    double dppxFactor =
        conversionToCanonicalUnitsScaleFactor(UnitType::DotsPerPixel);
    float valueInDppx =
        clampTo<float>(value.value * (canonicalFactor / dppxFactor));
    if (value.unit == UnitType::DotsPerCentimeter) {
        // To match DPCM to DPPX values, we limit to 2 decimal points.
        // The http://dev.w3.org/csswg/css3-values/#absolute-lengths recommends
        // "that the pixel unit refer to the whole number of device pixels that
        // best approximates the reference pixel". With that in mind, allowing 2
        // decimal point precision seems appropriate.
        return compareValue(floorf(0.5 + 100 * actualResolution) / 100,
                            floorf(0.5 + 100 * valueInDppx) / 100, op);
    }

    return compareValue(actualResolution, valueInDppx, op);
}

static bool resolutionMediaFeatureEval(MediaQueryExpValue& value,
                                       MediaValues* mediaValues,
                                       MediaFeaturePrefix op)
{
    return (!value.isValid() || isResolution(value.unit)) &&
           evalResolution(value, mediaValues, op);
}

static bool scanMediaFeatureEval(MediaQueryExpValue& value,
                                 MediaValues* mediaValues,
                                 MediaFeaturePrefix op)
{
    // The ‘scan’ media feature describes the scanning process of "tv" output
    // devices. But, we do not support "tv" media types now.
    return false;
}

static bool gridMediaFeatureEval(MediaQueryExpValue& value,
                                 MediaValues* mediaValues,
                                 MediaFeaturePrefix op)
{
    // We do not support grid-based output devices (e.g., a "tty" terminal, or
    // a phone display with only one fixed font). So, the value should be 0.
    float number;
    if (value.isValid() && numberValue(value, number)) {
        return compareValue(static_cast<int32_t>(number), 0, op);
    }
    return false;
}

static bool hoverMediaFeatureEval(MediaQueryExpValue& value,
                                  MediaValues* mediaValues,
                                  MediaFeaturePrefix op)
{
    // https://drafts.csswg.org/mediaqueries-4/#hover
    // NOTE: The spec says that this feature is at-risk, and may be dropped
    // during the CR period.
    if (value.isID) {
        // TODO: Suppose that the (primary) pointing device we support can
        // hover. However, the (primary) pointing device can’t hover, or that
        // there is no pointing device.
        return value.id->equalsIgnoreCase("hover");
    }
    return true;
}

static bool pointerMediaFeatureEval(MediaQueryExpValue& value,
                                    MediaValues* mediaValues,
                                    MediaFeaturePrefix op)
{
    // https://drafts.csswg.org/mediaqueries-4/#pointer
    // NOTE: The spec says that this feature is at-risk, and may be dropped
    // during the CR period.
    if (value.isID) {
        // TODO: Suppose that the (primary) input mechanism of the device
        // includes an accurate pointing device. However, the (primary) input
        // mechanism of the device does not include a pointing device and
        // includes a pointing device of limited accuracy.
        return value.id->equalsIgnoreCase("fine");
    }
    return true;
}

static bool scriptingMediaFeatureEval(MediaQueryExpValue& value,
                                      MediaValues* mediaValues,
                                      MediaFeaturePrefix op)
{
    // https://drafts.csswg.org/mediaqueries-5/#scripting
    if (value.isID) {
        // TODO: Consider the 'initial-only' value
        return mediaValues->hasScriptEngineInstance()
                   ? value.id->equalsIgnoreCase("enabled")
                   : value.id->equalsIgnoreCase("none");
    }
    return true;
}

static bool updateMediaFeatureEval(MediaQueryExpValue& value,
                                   MediaValues* mediaValues,
                                   MediaFeaturePrefix op)
{
    // https://www.w3.org/TR/mediaqueries-4/#update
    if (value.isID) {
        // NOTE: Suppose that the output device we support is not unusually
        // constrained in speed, so regularly-updating things like CSS
        // animations can be used. However, we should consider more value if we
        // support more output devices. (e.g., none: documents printed on paper,
        // slow: E-ink screens or severely under-powered devices)
        return value.id->equalsIgnoreCase("fast");
    }
    return true;
}

bool MediaQueryEvaluator::eval(MediaQueryExp* exp) const
{
    // MediaQueryExp can be nullptr when it has invalid expressions.
    if (!exp) {
        return false;
    }

    MediaQueryExpValue value = exp->expValue();
    switch (exp->mediaFeature()) {
    case MediaFeatureNone:
        return false;
#define EVAL_MEDIA_FEATURES(name, mediaFeatureName, evalFunc, prefix) \
    case MediaFeature##name:                                          \
        return evalFunc##MediaFeatureEval(value, m_mediaValues, prefix);
        ENUM_MEDIA_FEATURES(EVAL_MEDIA_FEATURES)
#undef EVAL_MEDIA_FEATURES
    default:
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return false;
    }
}

} /* namespace StarFish */
