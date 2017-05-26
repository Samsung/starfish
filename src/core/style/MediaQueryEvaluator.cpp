/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "core/style/MediaQueryEvaluator.h"
#include "core/layout/LayoutUtil.h"

namespace StarFish {

enum MediaFeaturePrefix { NoPrefix, MinPrefix, MaxPrefix };

bool MediaQueryEvaluator::mediaTypeMatch(String* mediaTypeToMatch) const
{
    // If the m_mediaType is an empty string, it means that we support all media
    // types.
    return m_mediaType->equals(String::emptyString) ||
           mediaTypeToMatch->equals(String::emptyString) ||
           mediaTypeToMatch->equalsWithoutCase("all") ||
           mediaTypeToMatch->equals(m_mediaType);
}

static bool applyRestrictor(MediaQuery::RestrictorType type, bool value)
{
    return type == MediaQuery::Not ? !value : value;
}

bool MediaQueryEvaluator::eval(MediaQuerySet* mediaQueries) const
{
    auto queries = mediaQueries->queryVector();
    if (!queries.size()) {
        // Empty query list evaluates to true.
        return true;
    }

    // Iterate over queries, stop if any of them eval to true.
    auto iter = queries.begin();
    while (iter != queries.end()) {
        if (eval(*iter)) {
            return true;
        }
        iter++;
    }

    return false;
}

bool MediaQueryEvaluator::eval(MediaQuery* query) const
{
    if (!mediaTypeMatch(query->mediaType())) {
        return applyRestrictor(query->restrictor(), false);
    }

    auto expressions = query->expressions();
    size_t i = 0;
    for (; i < expressions.size(); ++i) {
        if (!eval(expressions[i])) {
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

bool MediaQueryEvaluator::eval(MediaQueryExp* exp) const
{
    // TODO: Consider remaining features.
    String* feature = exp->mediaFeature();
    MediaQueryExpValue value = exp->expValue();
    if (feature->equals("width")) {
        return widthMediaFeatureEval(value, m_mediaValues, NoPrefix);
    } else if (feature->equals("min-width")) {
        return widthMediaFeatureEval(value, m_mediaValues, MinPrefix);
    } else if (feature->equals("max-width")) {
        return widthMediaFeatureEval(value, m_mediaValues, MaxPrefix);
    } else if (feature->equals("height")) {
        return heightMediaFeatureEval(value, m_mediaValues, NoPrefix);
    } else if (feature->equals("min-height")) {
        return heightMediaFeatureEval(value, m_mediaValues, MinPrefix);
    } else if (feature->equals("max-height")) {
        return heightMediaFeatureEval(value, m_mediaValues, MaxPrefix);
    } else {
        STARFISH_LOG_INFO("unsupported media feature: %s\n",
                          exp->mediaFeature()->utf8Data());
        return false;
    }
}

} /* namespace StarFish */
