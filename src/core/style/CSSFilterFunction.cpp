
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

#include "StarFishConfig.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSTokenValue.h"
#include "core/style/Style.h"
#include "core/style/CSSFilterFunction.h"

namespace StarFish {

static const int kFilterSize = 10;
static const char* kFilters[kFilterSize] = {
    "blur",      "drop-shadow", "hue-rotate", "brightness", "contrast",
    "grayscale", "invert",      "opacity",    "saturate",   "sephia"
};

static bool parseLengthOrColor(const CSSTokenValue& token,
                               CSSStyleValuePair& result)
{
    if (CSSPropertyParser::parseLength(
            token.data(), CSSPropertyParser::AllowNegative, &result)) {
        return true;
    }
    if (CSSPropertyParser::parseNonNamedColor(token.data(), &result)) {
        return true;
    }
    return CSSPropertyParser::parseNamedColor(token.data(), &result);
}

static bool parseDropShadowFilter(const CSSTokenValue& data,
                                  CSSStyleValuePair& result)
{
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, data.data(), data.length());
    const size_t total = tokens.size();
    if (total < 2) {
        return false;
    }
    int lengthTypes = 0;
    ValueList* list = new ValueList(ValueList::SpaceSeparator);
    for (size_t i = 0; i < total; i++) {
        CSSStyleValuePair item;
        if (!parseLengthOrColor(tokens[i], item)) {
            return false;
        }
        if (item.valueKind() == CSSStyleValuePair::ValueKind::Length) {
            lengthTypes++;
        }
        list->push_back(item);
    }
    // NOTE length type items: 2~4, color type items: 0~1
    if (lengthTypes <= 4 && lengthTypes >= 2 && total - lengthTypes <= 1) {
        result.setValueList(list);
        return true;
    }
    return false;
}

static bool parseLengthFilter(const CSSTokenValue& trimmed,
                              CSSStyleValuePair& result, float defaultValue = 0)
{
    if (!trimmed.length()) {
        result.setLengthValue(CSSLength(CSSLength::PX, defaultValue));
        return true;
    }
    return CSSPropertyParser::parseLength(trimmed.data(), 0, &result);
}

static bool parseAngleFilter(const CSSTokenValue& trimmed,
                             CSSStyleValuePair& result, float defaultValue = 0)
{
    if (!trimmed.length()) {
        result.setAngleValue(CSSAngle(defaultValue));
        return true;
    }
    return CSSPropertyParser::parseAngle(
        trimmed.data(), CSSPropertyParser::AllowNegative, &result);
}

static bool parseNumberOrPercentageFilter(const CSSTokenValue& trimmed,
                                          CSSStyleValuePair& result,
                                          float defaultValue = 1.0)
{
    if (!trimmed.length()) {
        result.setNumberValue(defaultValue);
        return true;
    }
    return CSSPropertyParser::parseNumberOrPercentage(trimmed.data(), 0,
                                                      &result);
}

static bool parseArguments(FilterFunctionType type, const CSSTokenValue& from,
                           CSSStyleValuePair& result)
{
    switch (type) {
    case FilterFunctionType::BlurFilterFunctionType:
        return parseLengthFilter(from.trim(), result);
    case FilterFunctionType::DropShadowFilterFunctionType:
        return parseDropShadowFilter(from, result);
    case FilterFunctionType::HueRotateFilterFunctionType:
        return parseAngleFilter(from.trim(), result);
    case FilterFunctionType::BrightnessFilterFunctionType:
    case FilterFunctionType::ContrastFilterFunctionType:
    case FilterFunctionType::GrayScaleFilterFunctionType:
    case FilterFunctionType::InvertFilterFunctionType:
    case FilterFunctionType::OpacityFilterFunctionType:
    case FilterFunctionType::SaturateFilterFunctionType:
    case FilterFunctionType::SepiaFilterFunctionType:
        return parseNumberOrPercentageFilter(from.trim(), result);
    case FilterFunctionType::SVGUrlFilterFunctionType:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    return false;
}

const char* CSSFilterFunction::typeToString(FilterFunctionType type)
{
    int enumIndex = (int)type;
    if (enumIndex < kFilterSize) {
        return kFilters[enumIndex];
    }
    return "";
}

String* CSSFilterFunction::toString() const
{
    if (m_type == FilterFunctionType::SVGUrlFilterFunctionType) {
        return m_data.toString();
    }
    StringBuilder result;
    result.appendString(typeToString(m_type));
    result.appendString("(");
    result.appendString(m_data.toString());
    result.appendString(")");
    return result.finalize();
}

CSSFilterFunction* CSSFilterFunction::parse(const CSSTokenValue& from)
{
    CSSStyleValuePair item;
    if (CSSPropertyParser::parseUrl(from.data(), &item)) {
        return new CSSFilterFunction(
            FilterFunctionType::SVGUrlFilterFunctionType, item);
    }

    CSSTokenValue token = from;
    std::transform(token.begin(), token.end(), token.begin(), tolower);
    for (size_t i = 0; i < kFilterSize; i++) {
        Nullable<CSSTokenValue> matched =
            CSSPropertyParser::parseFunctionBlock(token.data(), kFilters[i]);
        if (!matched.hasValue()) {
            continue;
        }
        CSSStyleValuePair data;
        if (!parseArguments((FilterFunctionType)i, matched.getValue(), data)) {
            return nullptr;
        }
        return new CSSFilterFunction((FilterFunctionType)i, data);
    }
    return nullptr;
}
} // namespace StarFish
