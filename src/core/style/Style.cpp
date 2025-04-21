/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *               1999 Waldo Bastian (bastian@kde.org)
 *               2001 Andreas Schlapbach (schlpbch@iam.unibe.ch)
 *               2001-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

#include <SkMatrix.h>

#include "StarfishConfig.h"

#include "core/style/Style.h"

#include "Starfish.h"
#include "core/animation/AnimationTask.h"
#include "core/animation/AnimationExecutor.h"
#include "core/animation/CubicBezier.h"
#include "core/animation/Steps.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLDocument.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/ShadowRoot.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/dom/svg/SVGUseElement.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/style/AncestorSelectorFilter.h"
#include "core/style/CalcData.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSCounterFunction.h"
#include "core/style/CSSGradientValue.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSProperty.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/FontFaceSrcData.h"
#include "core/style/FilterFunctions.h"
#include "core/style/FlexBasisData.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/MediaQueryResult.h"
#include "core/style/MediaValues.h"
#include "core/style/NamedColors.h"
#include "core/style/RectData.h"
#include "core/style/StrokeLineCap.h"
#include "core/style/StrokeLineJoin.h"
#include "core/style/Style.h"
#include "core/style/StyleRule.h"
#include "core/style/ShadowData.h"
#include "core/style/WillChangeData.h"
#include "core/style/CSSVariableSyntaxTreeBuilder.h"
#include "platform/loader/ResourceLoader.h"
#include "core/style/LengthUtil.h"

namespace Starfish {

static bool compareCString(const char* keyword, const char* value)
{
    return (strlen(keyword) == strlen(value) &&
            (memcmp(value, keyword, strlen(keyword))) == 0);
}

static bool parseGridTemplateRowsAndColumns(const CSSTokenVector& tokens,
                                            GCVector<GridTrackSize*>* v,
                                            uint8_t option, bool allowRepeat,
                                            bool allowFlexible);

static FontWeightValue lighterWeight(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::OneHundredFontWeightValue:
    case FontWeightValue::TwoHundredsFontWeightValue:
    case FontWeightValue::ThreeHundredsFontWeightValue:
    case FontWeightValue::NormalFontWeightValue:
    case FontWeightValue::FiveHundredsFontWeightValue:
        return FontWeightValue::OneHundredFontWeightValue;
    case FontWeightValue::SixHundredsFontWeightValue:
    case FontWeightValue::BoldFontWeightValue:
        return FontWeightValue::NormalFontWeightValue; // 400
    case FontWeightValue::EightHundredsFontWeightValue:
    case FontWeightValue::NineHundredsFontWeightValue:
        return FontWeightValue::BoldFontWeightValue; // 700
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

static FontWeightValue bolderWeight(FontWeightValue weight)
{
    switch (weight) {
    case FontWeightValue::OneHundredFontWeightValue:
    case FontWeightValue::TwoHundredsFontWeightValue:
    case FontWeightValue::ThreeHundredsFontWeightValue:
        return FontWeightValue::NormalFontWeightValue; // 400
    case FontWeightValue::NormalFontWeightValue:
    case FontWeightValue::FiveHundredsFontWeightValue:
        return FontWeightValue::BoldFontWeightValue; // 700
    case FontWeightValue::SixHundredsFontWeightValue:
    case FontWeightValue::BoldFontWeightValue:
    case FontWeightValue::EightHundredsFontWeightValue:
    case FontWeightValue::NineHundredsFontWeightValue:
        return FontWeightValue::NineHundredsFontWeightValue; // 900
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

static const int strictFontSizeTable[8][8] = {
    { 9, 9, 9, 9, 11, 14, 18, 27 },    { 9, 9, 9, 10, 12, 15, 20, 30 },
    { 9, 9, 10, 11, 13, 17, 22, 33 },  { 9, 9, 10, 12, 14, 18, 24, 36 },
    { 9, 10, 12, 13, 16, 20, 26, 39 }, // fixed font default (13)
    { 9, 10, 12, 14, 17, 21, 28, 42 }, { 9, 10, 13, 15, 18, 23, 30, 45 },
    { 9, 10, 13, 16, 18, 24, 32, 48 } // proportional font default (16)
};

static const int fontSizeTableMax = 16;
static const int fontSizeTableMin = 9;

static Length parseAbsoluteFontSize(int col, float mediumSize)
{
    int row = 0;
    if (mediumSize >= fontSizeTableMin && mediumSize <= fontSizeTableMax) {
        row = mediumSize - fontSizeTableMin;
    }

    return Length(Length::Fixed, strictFontSizeTable[row][col]);
}

static void setComputedStyleUnitPositionX(
    ComputedStyle* style, const CSSStyleValuePair& value, uint32_t layer,
    std::function<void(ComputedStyle* style, const Length& length,
                       uint32_t layer)>
        setter)
{
    STARFISH_ASSERT(style != nullptr);
    CSSStyleValuePair::ValueKind valueKind = value.valueKind();
    if (valueKind == CSSStyleValuePair::ValueKind::Initial ||
        valueKind == CSSStyleValuePair::ValueKind::Unset) {
        setter(style, Length(Length::Percent, 0.0f), layer);
    } else if (valueKind == CSSStyleValuePair::ValueKind::SideValueKind) {
        SideValue side = value.sideValue();
        if (side == SideValue::LeftSideValue) {
            setter(style, Length(Length::Percent, 0.0f), layer);
        } else if (side == SideValue::RightSideValue) {
            setter(style, Length(Length::Percent, 1.0f), layer);
        } else if (side == SideValue::CenterSideValue) {
            setter(style, Length(Length::Percent, 0.5f), layer);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (valueKind == CSSStyleValuePair::ValueKind::ValuePairKind) {
        ValuePair* pair = value.pairValue();
        const CSSStyleValuePair& side = pair->first();
        const CSSStyleValuePair& offset = pair->second();
        CSSStyleValuePair::ValueKind offsetValueKind = offset.valueKind();
        STARFISH_ASSERT(side.valueKind() ==
                        CSSStyleValuePair::ValueKind::SideValueKind);
        STARFISH_ASSERT(
            offsetValueKind == CSSStyleValuePair::ValueKind::Length ||
            offsetValueKind == CSSStyleValuePair::ValueKind::Percentage ||
            offsetValueKind == CSSStyleValuePair::ValueKind::CalcValueKind);
        if (side.sideValue() == SideValue::LeftSideValue) {
            Optional<Length> maybeLength = LengthUtil::convertValueToLength(
                offsetValueKind, offset.value());
            if (maybeLength.hasValue()) {
                setter(style, maybeLength.getValue(), layer);
            }
        } else {
            STARFISH_ASSERT(side.sideValue() == SideValue::RightSideValue);
            CalcTerm* term1 = new CalcTerm();
            if (offsetValueKind == CSSStyleValuePair::ValueKind::Length) {
                term1->appendValue(CalcValue(offset.cssLengthValue()));
            } else if (offsetValueKind ==
                       CSSStyleValuePair::ValueKind::Percentage) {
                term1->appendValue(CalcValue(offset.percentageValue(), true));
            } else {
                term1->appendValue(CalcValue(offset.calcValue()));
            }
            term1->appendValue(true, CalcValue(-1.0f, false));

            CalcTerm* term2 = new CalcTerm();
            term2->appendValue(CalcValue(1.0f, true));

            CalcData* calcData = new CalcData();
            calcData->appendTerm(term1);
            calcData->appendTerm(term2);
            setter(style, Length(calcData), layer);
        }
    } else if (valueKind == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = value.multiValue();
        for (unsigned int i = 0; i < list->size(); i++) {
            setComputedStyleUnitPositionX(style, (*list)[i], i, setter);
        }
    } else {
        Optional<Length> len =
            LengthUtil::convertValueToLength(value.valueKind(), value.value());
        if (len.hasValue()) {
            setter(style, len.getValue(), layer);
        } else {
            setter(style, Length(Length::Percent, 0.0f), layer);
        }
    }
}

static void setComputedStyleUnitPositionY(
    ComputedStyle* style, const CSSStyleValuePair& value, uint32_t layer,
    std::function<void(ComputedStyle* style, const Length& length,
                       uint32_t layer)>
        setter)
{
    STARFISH_ASSERT(style != nullptr);
    CSSStyleValuePair::ValueKind valueKind = value.valueKind();
    if (valueKind == CSSStyleValuePair::ValueKind::Initial ||
        valueKind == CSSStyleValuePair::ValueKind::Unset) {
        setter(style, Length(Length::Percent, 0.0f), layer);
    } else if (valueKind == CSSStyleValuePair::ValueKind::SideValueKind) {
        SideValue side = value.sideValue();
        if (side == SideValue::TopSideValue) {
            setter(style, Length(Length::Percent, 0.0f), layer);
        } else if (side == SideValue::BottomSideValue) {
            setter(style, Length(Length::Percent, 1.0f), layer);
        } else if (side == SideValue::CenterSideValue) {
            setter(style, Length(Length::Percent, 0.5f), layer);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (valueKind == CSSStyleValuePair::ValueKind::ValuePairKind) {
        ValuePair* pair = value.pairValue();
        const CSSStyleValuePair& side = pair->first();
        const CSSStyleValuePair& offset = pair->second();
        CSSStyleValuePair::ValueKind offsetValueKind = offset.valueKind();
        STARFISH_ASSERT(side.valueKind() ==
                        CSSStyleValuePair::ValueKind::SideValueKind);
        STARFISH_ASSERT(
            offsetValueKind == CSSStyleValuePair::ValueKind::Length ||
            offsetValueKind == CSSStyleValuePair::ValueKind::Percentage ||
            offsetValueKind == CSSStyleValuePair::ValueKind::CalcValueKind);
        if (side.sideValue() == SideValue::TopSideValue) {
            Optional<Length> maybeLength = LengthUtil::convertValueToLength(
                offsetValueKind, offset.value());
            if (maybeLength.hasValue()) {
                setter(style, maybeLength.getValue(), layer);
            }
        } else {
            STARFISH_ASSERT(side.sideValue() == SideValue::BottomSideValue);
            CalcTerm* term1 = new CalcTerm();
            if (offsetValueKind == CSSStyleValuePair::ValueKind::Length) {
                term1->appendValue(CalcValue(offset.cssLengthValue()));
            } else if (offsetValueKind ==
                       CSSStyleValuePair::ValueKind::Percentage) {
                term1->appendValue(CalcValue(offset.percentageValue(), true));
            } else {
                term1->appendValue(CalcValue(offset.calcValue()));
            }
            term1->appendValue(true, CalcValue(-1.0f, false));

            CalcTerm* term2 = new CalcTerm();
            term2->appendValue(CalcValue(1.0f, true));

            CalcData* calcData = new CalcData();
            calcData->appendTerm(term1);
            calcData->appendTerm(term2);
            setter(style, Length(calcData), layer);
        }
    } else if (valueKind == CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = value.multiValue();
        for (uint32_t i = 0; i < list->size(); i++) {
            setComputedStyleUnitPositionY(style, (*list)[i], i, setter);
        }
    } else {
        Optional<Length> len =
            LengthUtil::convertValueToLength(value.valueKind(), value.value());
        if (len.hasValue()) {
            setter(style, len.getValue(), layer);
        } else {
            setter(style, Length(Length::Percent, 0.0f), layer);
        }
    }
}

bool CSSTransformFunction::operator==(const CSSTransformFunction& src)
{
    return m_kind == src.m_kind && m_values->equals(src.m_values);
}

static bool removeCalcFuncNameIfNeeds(const CSSStyleValuePair& item,
                                      OptionalUTF8String& utf8String)
{
    if (item.varFunctionValue()->startsWith("calc(") &&
        item.varFunctionValue()->endsWith(")")) {
        utf8String.m_buffer = utf8String.m_buffer + 5;
        utf8String.m_bufferSize -= 4;
        return true;
    }

    return false;
}

void CSSTransformFunctions::toTransformDataGroup(Element* element,
                                                 ComputedStyle* style)
{
    STARFISH_ASSERT(style != nullptr);

    for (unsigned c = 0; c < this->size(); c++) {
        CSSTransformFunction f = this->at(c);
        size_t valueSize = f.values()->size();
        float* dValues = ALLOCA(valueSize * sizeof(float), float);
        ValueList convertedValueList(*f.values());

        for (size_t i = 0; i < valueSize; i++) {
            CSSStyleValuePair item = convertedValueList.at(i);
            if (item.valueKind() == CSSStyleValuePair::ValueKind::Number) {
                dValues[i] = item.numberValue();
            } else if (item.valueKind() ==
                       CSSStyleValuePair::ValueKind::Angle) {
                dValues[i] = item.angleValue().toDegreeValue();
            } else if (item.valueKind() ==
                       CSSStyleValuePair::ValueKind::CalcValueKind) {
                CalcData* calcData = item.calcValue();
                CalcValueType type = calcData->calcValueType();
                if (type.isAngle() == true) {
                    dValues[i] = calcData->angleValue().toDegreeValue();
                }
            } else if (item.valueKind() ==
                       CSSStyleValuePair::ValueKind::VarFunctionValueKind) {
                auto refValue =
                    element->styleResolver().resolveVarReferencedValue(element,
                                                                       item);
                ValueList* values = new ValueList(f.values()->separator());
                CSSStyleValuePair newPair;
                newPair.updateValueTransformFunction(refValue, f.kind(), false,
                                                     values);
                if (!values->size()) {
                    // resolved var value failed or resolved value is not valid
                    return;
                }

                if (values->size() == 1) {
                    convertedValueList[i] = values->at(0);
                } else {
                    valueSize = values->size();
                    dValues = ALLOCA(valueSize * sizeof(float), float);
                    convertedValueList.clear();
                    for (const auto& item : *values) {
                        convertedValueList.push_back(item);
                    }
                }
                i--;
                continue;
            }
        }

        switch (f.kind()) {
        case CSSTransformFunction::Kind::Matrix:
            style->setTransformMatrix(dValues[0], dValues[1], dValues[2],
                                      dValues[3], dValues[4], dValues[5]);

            if (dValues[0] != dValues[3] || dValues[1] != 0 ||
                dValues[2] != 0) {
                style->rareComputedStyleData()
                    ->ensureTransforms()
                    ->m_hasComplexTransform = true;
            }
            break;
        case CSSTransformFunction::Kind::Translate3D:
        case CSSTransformFunction::Kind::Translate: {
            Length a, b(Length::Fixed, 0);
            Optional<Length> nA = LengthUtil::convertValueToLength(
                convertedValueList[0].valueKind(),
                convertedValueList[0].value());
            if (nA.hasValue() == true) {
                a = nA.getValue();
            } else {
                break;
            }
            if (valueSize > 1) {
                Optional<Length> nB = LengthUtil::convertValueToLength(
                    convertedValueList[1].valueKind(),
                    convertedValueList[1].value());
                if (nB.hasValue()) {
                    b = nB.getValue();
                } else {
                    break;
                }
            }
            if (f.kind() == CSSTransformFunction::Kind::Translate3D) {
                style->rareComputedStyleData()
                    ->ensureTransforms()
                    ->m_has3DTransform = true;
                STARFISH_UNSUPPORTED("css function: translate3d");
            }
            style->setTransformTranslate(a, b);
            break;
        }
        case CSSTransformFunction::Kind::TranslateX: {
            Optional<Length> a = LengthUtil::convertValueToLength(
                convertedValueList[0].valueKind(),
                convertedValueList[0].value());
            if (a.hasValue() == true) {
                style->setTransformTranslate(a.getValue(),
                                             Length(Length::Fixed, 0));
            }
        } break;
        case CSSTransformFunction::Kind::TranslateY: {
            Optional<Length> a = LengthUtil::convertValueToLength(
                convertedValueList[0].valueKind(),
                convertedValueList[0].value());
            if (a.hasValue() == true) {
                style->setTransformTranslate(Length(Length::Fixed, 0),
                                             a.getValue());
            }
        } break;
        case CSSTransformFunction::Kind::TranslateZ: {
            style->rareComputedStyleData()
                ->ensureTransforms()
                ->m_has3DTransform = true;
            if (convertedValueList[0].valueKind() !=
                    CSSStyleValuePair::ValueKind::Length ||
                convertedValueList[0].lengthValue() !=
                    Length(Length::Fixed, 0)) {
                STARFISH_UNSUPPORTED("css function: translateZ");
            }
        } break;
        case CSSTransformFunction::Kind::Scale:
            if (valueSize == 1) {
                style->setTransformScale(dValues[0], dValues[0]);
            } else if (valueSize == 2) {
                style->setTransformScale(dValues[0], dValues[1]);
            } else {
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
            break;
        case CSSTransformFunction::Kind::ScaleX:
            style->setTransformScale(dValues[0], 1);
            break;
        case CSSTransformFunction::Kind::ScaleY:
            style->setTransformScale(1, dValues[0]);
            break;
        case CSSTransformFunction::Kind::Rotate:
            if (valueSize > 1) {
                Length a, b(Length::Fixed, 0);
                Optional<Length> nA = LengthUtil::convertValueToLength(
                    convertedValueList[1].valueKind(),
                    convertedValueList[1].value());
                if (nA.hasValue() == true) {
                    a = nA.getValue();
                } else {
                    break;
                }
                if (valueSize > 2) {
                    Optional<Length> nB = LengthUtil::convertValueToLength(
                        convertedValueList[2].valueKind(),
                        convertedValueList[2].value());
                    if (nB.hasValue()) {
                        b = nB.getValue();
                    } else {
                        break;
                    }
                }

                style->setTransformRotate(dValues[0], a, b);
            } else {
                style->setTransformRotate(dValues[0]);
            }

            style->rareComputedStyleData()
                ->ensureTransforms()
                ->m_hasComplexTransform = true;
            break;
        case CSSTransformFunction::Kind::Skew:
        case CSSTransformFunction::Kind::SkewX:
            if (valueSize == 2) {
                style->setTransformSkew(dValues[0], dValues[1]);
            } else {
                style->setTransformSkew(dValues[0], 0);
            }
            style->rareComputedStyleData()
                ->ensureTransforms()
                ->m_hasComplexTransform = true;
            break;
        case CSSTransformFunction::Kind::SkewY:
            style->setTransformSkew(0, dValues[0]);
            style->rareComputedStyleData()
                ->ensureTransforms()
                ->m_hasComplexTransform = true;
            break;
        default:
            style->rareComputedStyleData()
                ->ensureTransforms()
                ->m_hasComplexTransform = true;
            style->rareComputedStyleData()
                ->ensureTransforms()
                ->m_has3DTransform = true;
            STARFISH_UNSUPPORTED("Unsupported transform function: [%d]",
                                 static_cast<int>(f.kind()));
        }
    }
}

String* CSSTransformFunctions::toString()
{
    StringBuilder builder;
    for (unsigned i = 0; i < size(); i++) {
        CSSTransformFunction item = (*this)[i];
        builder.appendString(item.functionName());
        builder.appendString("(");
        ValueList* values = item.values();
        for (unsigned int j = 0; j < values->size(); j++) {
            const CSSStyleValuePair& subitem = (*values)[j];
            String* newstr = subitem.toString();
            builder.appendString(newstr);
            if (j != values->size() - 1) {
                builder.appendString(", ");
            } else {
                builder.appendString(")");
            }
        }
    }
    return builder.finalize();
}

String* CSSStyleValuePair::keyName() const
{
    switch (keyKind()) {
#define ADD_CASE_FOR_KEYNAME(Name, name, cssname) \
    case CSSStyleValuePair::KeyKind::Name:        \
        return String::createASCIIString(cssname);
        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ADD_CASE_FOR_KEYNAME)
#undef ADD_CASE_FOR_KEYNAME
    case CSSStyleValuePair::KeyKind::CustomProperty:
        // TODO
        return String::emptyString;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
}

static bool needsToFindVarFunction(const CSSTokenValue& token, int startPos,
                                   int* varFunctionStartPos,
                                   int* varFunctionEndPos)
{
    int start = 0, end = 0;
    start = token.find("var(", startPos);
    end = token.find(")", start);
    if (start >= 0 && end > 0) {
        *varFunctionStartPos = start;
        *varFunctionEndPos = end;
        return true;
    }
    return false;
}

static bool hasValidVarFunction(const CSSTokenValue& token)
{
    // we can give pass nullptr into CSSVariableSyntaxTreeBuilder
    // because, we don't need variables of this builder
    // we just want to know that the syntax is correct

    bool result = false;
    int start = 0, end = 0;
    CSSVariableSyntaxTreeBuilder variablesSyntaxBuilder(nullptr);
    while (needsToFindVarFunction(token, start, &start, &end)) {
        if (start == 0) {
            variablesSyntaxBuilder.build(token.data(), token.size());
        } else {
            const CSSTokenValue& nextToken =
                token.substring(start, end - start + 1);
            variablesSyntaxBuilder.reset();
            variablesSyntaxBuilder.build(nextToken.data(), nextToken.size());
        }
        if (!variablesSyntaxBuilder.isValid()) {
            return false;
        } else {
            result = true;
        }
        start = end + 1;
    }
    return result;
}

bool CSSStyleValuePair::updateValueForAttributeBasic(
    Document* document, CSSStyleValuePair::KeyKind keyKind,
    const CSSTokenVector& tokens)
{
    // Use macros to prevent missing attribute basic.
    switch (keyKind) {
#define UPDATE_VALUE(name, ...)            \
    case CSSStyleValuePair::KeyKind::name: \
        return updateValue##name(document, tokens);
        FOR_EACH_STYLE_ATTRIBUTE_BASIC(UPDATE_VALUE)
#undef UPDATE_VALUE
    default:
        STARFISH_LOG_WARN("keykind is not attribute basic.");
        return false;
    }
    return false;
}

bool CSSStyleValuePair::updateValueVarReferences(const CSSTokenVector& tokens)
{
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& token = tokens[i];
        if (hasValidVarFunction(token)) {
            m_valueKind = CSSStyleValuePair::ValueKind::VarFunctionValueKind;
            return true;
        }
    }

    return false;
}

bool CSSStyleValuePair::updateValueCommon(const CSSTokenVector& tokens)
{
    // NOTE: set common value (e.g. initial, inherit, "")
    if (tokens.size() != 1) {
        return false;
    }
    const char* value = tokens[0].data();

    if (compareCString("inherit", value)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Inherit;
    } else if (compareCString("initial", value)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Initial;
    } else if (compareCString("unset", value)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Unset;
    } else {
        return false;
    }
    return true;
}

String* CSSStyleValuePair::urlValue(ResourceURL* urlOfStyleSheet) const
{
    STARFISH_ASSERT(urlOfStyleSheet != nullptr);

    STARFISH_ASSERT(m_valueKind == UrlValueKind);
    return ResourceURL::mergeDocumentURIWithURIString(
        urlOfStyleSheet->baseURI(), m_value.m_stringValue);
}

unsigned CSSSelector::specificityForOneSelector() const
{
    unsigned specificity = 0;

    switch (m_type) {
    case Tag:
    case PseudoElement:
        return specificity + 0x000001;
    case Id:
        return specificity + 0x010000;
    case Class:
    case PseudoClass: {
        if (UNLIKELY(m_pseudotype == CSSSelector::PseudoType::PseudoHost)) {
            return 0;
        }
        return specificity + 0x000100;
    }
    case CSSSelector::AttributeExact:   // Example: E[foo="bar"]
    case CSSSelector::AttributeSet:     // Example: E[foo]
    case CSSSelector::AttributeHyphen:  // Example: E[foo|="bar"]
    case CSSSelector::AttributeList:    // Example: E[foo~="bar"]
    case CSSSelector::AttributeContain: // css3: E[foo*="bar"]
    case CSSSelector::AttributeBegin:   // css3: E[foo^="bar"]
    case CSSSelector::AttributeEnd:     // css3: E[foo$="bar"]
        return specificity + 0x000100;
    default:
        break;
    }

    return 0;
}

String* CSSSelectorList::selectorText(CSSSelectorList* list, unsigned idx,
                                      String* rightSide)
{
    STARFISH_ASSERT(list != nullptr);
    STARFISH_ASSERT(rightSide != nullptr);

    StringBuilder str;
    CSSSelector* cs = list->at(idx).m_selector;
    STARFISH_ASSERT(cs != nullptr);

    if (cs->type() == CSSSelector::Tag ||
        cs->type() == CSSSelector::Universal) {
        str.appendString(cs->selectorText().string());
    }

    while (true) {
        if (cs->type() == CSSSelector::Id) {
            str.appendChar('#');
            str.appendString(cs->selectorText().string());
        } else if (cs->type() == CSSSelector::Class) {
            str.appendChar('.');
            str.appendString(cs->selectorText().string());
        } else if (cs->type() == CSSSelector::PseudoClass) {
            str.appendChar(':');
            str.appendString(cs->selectorText().string());

            CSSPseudoSelector* pcs = cs->asCSSPseudoSelector();

            switch (pcs->pseudoType()) {
            case CSSSelector::PseudoNthChild:
            case CSSSelector::PseudoNthLastChild:
            case CSSSelector::PseudoNthOfType:
            case CSSSelector::PseudoNthLastOfType: {
                str.appendChar('(');

                // http://dev.w3.org/csswg/css-syntax/#serializing-anb
                int a = pcs->nthAValue();
                int b = pcs->nthBValue();
                if (a == 0 && b == 0) {
                    str.appendChar('0');
                } else if (a == 0) {
                    str.appendString(String::fromInt(b));
                } else if (b == 0) {
                    str.appendString(String::fromInt(a));
                    str.appendChar('n');
                } else if (b < 0) {
                    str.appendString(String::fromInt(a));
                    str.appendChar('n');
                    str.appendString(String::fromInt(b));
                } else {
                    str.appendString(String::fromInt(a));
                    str.appendString("n+");
                    str.appendString(String::fromInt(b));
                }

                str.appendChar(')');
                break;
            }
            case CSSSelector::PseudoDir:
            case CSSSelector::PseudoLang:
                str.appendString(pcs->argument());
                str.appendChar(')');
                break;
            case CSSSelector::PseudoNot:
                STARFISH_ASSERT(pcs->pseudoSelectorList().size() > 0);
                str.appendString(pcs->pseudoSelectorList().selectorText());
                str.appendChar(')');
                break;
            default:
                break;
            }
        } else if (cs->type() == CSSSelector::PseudoElement) {
            str.appendString("::");
            str.appendString(cs->selectorText().string());
        } else if (cs->isAttributeSelector()) {
            CSSAttributeSelector* acs = cs->asCSSAttributeSelector();
            str.appendChar('[');
            str.appendString(acs->attribute().localName());
            switch (cs->type()) {
            case CSSSelector::AttributeExact:
                str.appendChar('=');
                break;
            case CSSSelector::AttributeSet:
                // set has no operator or value, just the attrName
                str.appendChar(']');
                break;
            case CSSSelector::AttributeList:
                str.appendString("~=");
                break;
            case CSSSelector::AttributeHyphen:
                str.appendString("|=");
                break;
            case CSSSelector::AttributeBegin:
                str.appendString("^=");
                break;
            case CSSSelector::AttributeEnd:
                str.appendString("$=");
                break;
            case CSSSelector::AttributeContain:
                str.appendString("*=");
                break;
            default:
                break;
            }
            if (acs->type() != CSSSelector::AttributeSet) {
                str.appendChar('\"');
                str.appendString(acs->selectorText().string());
                str.appendChar('\"');

                if (acs->attributeMatch() == CSSSelector::CaseInsensitive) {
                    str.appendString(" i");
                }
                str.appendChar(']');
            }
        }

        if (list->at(idx).m_relation != CSSSelectorListItem::SubSelector ||
            list->size() == (idx + 1)) {
            break;
        }
        cs = list->at(++idx).m_selector;
    }

    if (list->size() > (idx + 1)) {
        StringBuilder desc;
        switch (list->at(idx).m_relation) {
        case CSSSelectorListItem::Descendant:
            desc.appendString(" ");
            break;
        case CSSSelectorListItem::Child:
            desc.appendString(" > ");
            break;
        case CSSSelectorListItem::AdjacentSibling:
            desc.appendString(" + ");
            break;
        case CSSSelectorListItem::GeneralSibling:
            desc.appendString(" ~ ");
            break;
        case CSSSelectorListItem::SubSelector:
            STARFISH_ASSERT_NOT_REACHED();
            break;
        default:
            str.appendString(rightSide);
            return str.finalize();
        }

        desc.appendString(str.finalize());
        desc.appendString(rightSide);
        return selectorText(list, ++idx, desc.finalize());
    }

    str.appendString(rightSide);
    return str.finalize();
}

String* CSSSelectorList::selectorText()
{
    return selectorText(this, 0, String::emptyString);
}

bool CSSSelector::isSimple(CSSSelectorList* selectorList)
{
    STARFISH_ASSERT(selectorList != nullptr);

    if ((isPseudoSelector() &&
         asCSSPseudoSelector()->pseudoSelectorList().size()) ||
        type() == CSSSelector::PseudoElement) {
        return false;
    }

    if (selectorList->size() == 1) {
        return true;
    }
    /*
        if (m_selector->match() == CSSSelector::Tag) {
            // We can't check against anyQName() here because namespace may
            // not be nullAtom.
            // Example:
            //     @namespace "http://www.w3.org/2000/svg";
            //     svg:not(:root) { ...
            if (m_selector->tagQName().localName() == starAtom) {
                    return m_tagHistory->isSimple();
            }
        }
    */

    return false;
}

bool CSSSelector::isPseudoClassHostFamilySelector()
{
    return m_type == CSSSelector::Type::PseudoClass &&
           (m_pseudotype == CSSSelector::PseudoType::PseudoHost ||
            m_pseudotype == CSSSelector::PseudoType::PseudoHostFunction);
}

bool CSSPseudoSelector::matchNth(int count)
{
    if (!nthAValue()) {
        return count == nthBValue();
    }
    if (nthAValue() > 0) {
        if (count < nthBValue()) {
            return false;
        }
        return (count - nthBValue()) % nthAValue() == 0;
    }
    if (count > nthBValue()) {
        return false;
    }
    return (nthBValue() - count) % (-nthAValue()) == 0;
}

CSSSelector::PseudoType CSSPseudoSelector::parsePseudoType(
    Starfish* starfish, AtomicString pseudoName, bool hasArguments)
{
    STARFISH_ASSERT(starfish != nullptr);

    if (pseudoName.isEmptyAtomicString() ||
        !pseudoName.string()->containsOnlyASCIIChars()) {
        return CSSSelector::PseudoNone;
    }
    StaticStrings* sstrs = starfish->staticStrings();
    if (false) {
    }
#define SET_PSEUDO_TYPE(name, nameLower, ...)              \
    else if (pseudoName == sstrs->m_##nameLower##Selector) \
    {                                                      \
        return CSSSelector::PseudoType::Pseudo##name;      \
    }
    STARFISH_ENUM_PSEUDO_SELECTORS(SET_PSEUDO_TYPE)
#undef SET_PSEUDO_TYPE
    else
    {
        return CSSSelector::PseudoNone;
    }
}

void CSSPseudoSelector::updatePseudoType(Starfish* starfish, AtomicString name,
                                         bool hasArguments)
{
    STARFISH_ASSERT(starfish != nullptr);
    m_selectorText = name;
    m_pseudotype = parsePseudoType(starfish, name, hasArguments);

    switch (pseudoType()) {
    case PseudoAfter:
    case PseudoBefore:
    case PseudoFirstLetter:
    case PseudoFirstLine:
        // The spec says some pseudos allow both single and double colons like
        // :before for backwards compatability. Single colon becomes
        // PseudoClass,
        // but should be PseudoElement like double colon.
        if (type() == PseudoClass) {
            m_type = PseudoElement;
        }
        break;
    case PseudoBackdrop:
    case PseudoCue:
    case PseudoGrammarError:
    case PseudoMarker:
    case PseudoPlaceholder:
    /*
        case PseudoResizer:
        case PseudoScrollbar:
        case PseudoScrollbarCorner:
        case PseudoScrollbarButton:
        case PseudoScrollbarThumb:
        case PseudoScrollbarTrack:
        case PseudoScrollbarTrackPiece:
    */
    case PseudoSelection:
    case PseudoSpellingError:
        /*
            case PseudoWebKitCustomElement:
            case PseudoContent:
            case PseudoShadow:
            case PseudoSlotted:
        */
        if (type() != PseudoElement) {
            m_pseudotype = PseudoNone;
        }
        break;
    /*
        case PseudoFirstPage:
        case PseudoLeftPage:
        case PseudoRightPage:
            if (type() != PagePseudoClass) {
                setPseudoType(PseudoUnknown);
            }
            break;
    */
    case PseudoActive:
    case PseudoAnyLink:
    /*
        case PseudoAny:
        case PseudoAutofill:
    */
    case PseudoBlank:
    case PseudoChecked:
    // case PseudoCornerPresent:
    case PseudoCurrent:
    /*
        case PseudoDecrement:
    */
    case PseudoDefault:
    case PseudoDefined:
    case PseudoDir:
    case PseudoDisabled:
    /*
        case PseudoDoubleButton:
        case PseudoDrag:
    */
    case PseudoDrop:
    case PseudoEmpty:
    case PseudoEnabled:
    // case PseudoEnd:
    case PseudoFirstChild:
    case PseudoFirstOfType:
    case PseudoFocus:
    case PseudoFocusWithin:
    case PseudoFocusVisible:
    // case PseudoFullPageMedia:
    case PseudoFullScreen:
    // case PseudoFullScreenAncestor:
    case PseudoFuture:

    // case PseudoFutureCue:
    // case PseudoHorizontal:
    case PseudoHost:
    case PseudoHostFunction:
    // case PseudoHostContext:
    case PseudoHover:
    case PseudoInRange:
    /*
        case PseudoIncrement:
    */
    case PseudoIndeterminate:
    case PseudoInvalid:
    case PseudoLang:
    case PseudoLastChild:
    case PseudoLastOfType:
    case PseudoLink:
    case PseudoLocalLink:
    /*
        case PseudoListBox:
        case PseudoNoButton:
    */
    case PseudoNot:
    case PseudoNthChild:
    case PseudoNthLastChild:
    case PseudoNthLastOfType:
    case PseudoNthOfType:
    case PseudoOnlyChild:
    case PseudoOnlyOfType:
    case PseudoOptional:
    case PseudoOutOfRange:
    case PseudoPast:
    // case PseudoPastCue:
    case PseudoPaused:
    case PseudoPlaceholderShown:
    case PseudoPlaying:
    case PseudoReadOnly:
    case PseudoReadWrite:
    case PseudoRequired:
    case PseudoRoot:
    case PseudoScope:
    /*
        case PseudoSingleButton:
        case PseudoSpatialNavigationFocus:
        case PseudoStart:
    */
    case PseudoTarget:
    case PseudoTargetWithin:
    case PseudoNone:
    // case PseudoUnresolved:
    case PseudoUserInvalid:
    case PseudoValid:
    // case PseudoVertical:
    case PseudoVisited:
        // case PseudoWindowInactive:
        if (type() != PseudoClass) {
            m_pseudotype = PseudoNone;
        }
        break;
    case PseudoTotalCount:
        break;
    }
}

bool CSSStyleValuePair::valueEquals(const CSSStyleValuePair& src) const
{
    if (m_valueKind != src.m_valueKind) {
        return false;
    }

    switch (m_valueKind) {
    case Initial:
    case Inherit:
    case Unset:
    case Auto:
    case None:
    case Normal:
        return true;

    case Length:
        return m_value.m_length == src.m_value.m_length;
    case Percentage:
    case Number:
        return m_value.m_floatValue == src.m_value.m_floatValue;

    case Int32:
        return m_value.m_int32Value == src.m_value.m_int32Value;

    case Angle:
        return m_value.m_angle == src.m_value.m_angle;

    case Time:
        return m_value.m_time == src.m_value.m_time;

    case StringValueKind:
    case KeywordValueKind:
    case UrlValueKind:
    case PathFunctionValueKind:
    case Attr:
    case VarFunctionValueKind:
        return m_value.m_stringValue->equals(src.m_value.m_stringValue);

    case AtomicStringValueKind:
        return m_value.m_atomicStringValue == src.m_value.m_atomicStringValue;

    case ColorValueKind:
        return m_value.m_color == src.m_value.m_color;

    case NamedColorValueKind:
        return m_value.m_namedColor == src.m_value.m_namedColor;

    case CSSPropertyNameValueKind:
        return m_value.m_cssPropertyNameValue ==
               src.m_value.m_cssPropertyNameValue;

    case FilterFunctionValueKind:
        return m_value.m_filterFunction->equals(src.m_value.m_filterFunction);

    case CalcValueKind:
        return m_value.m_calc->equals(src.m_value.m_calc);

    case FontFaceSrcDataValueKind:
        return m_value.m_fontFaceSrcData->equals(src.m_value.m_fontFaceSrcData);

    case DisplayValueKind:
        return m_value.m_display == src.m_value.m_display;

    case PositionValueKind:
        return m_value.m_position == src.m_value.m_position;

    case FloatValueKind:
        return m_value.m_float == src.m_value.m_float;

    case ClearValueKind:
        return m_value.m_clear == src.m_value.m_clear;

    case VerticalAlignValueKind:
        return m_value.m_verticalAlign == src.m_value.m_verticalAlign;

    case TextAlignValueKind:
        return m_value.m_textAlign == src.m_value.m_textAlign;

    case SideValueKind:
        return m_value.m_side == src.m_value.m_side;

    case DirectionValueKind:
        return m_value.m_direction == src.m_value.m_direction;

    case WhiteSpaceValueKind:
        return m_value.m_whiteSpace == src.m_value.m_whiteSpace;

    case ValueListKind:
        return m_value.m_multiValue->equals(src.m_value.m_multiValue);

    case ValuePairKind:
        return m_value.m_pairValue->equals(src.m_value.m_pairValue);

    case BackgroundSizeValueKind:
        return m_value.m_backgroundSize == src.m_value.m_backgroundSize;

    case RepeatStyleValueKind:
        return m_value.m_repeatStyle == src.m_value.m_repeatStyle;

    case BackgroundAttachmentValueKind:
        return m_value.m_backgroundAttachment ==
               src.m_value.m_backgroundAttachment;

    case BoxValueKind:
        return m_value.m_box == src.m_value.m_box;

    case FontSizeValueKind:
        return m_value.m_fontSize == src.m_value.m_fontSize;

    case FontStyleValueKind:
        return m_value.m_fontStyle == src.m_value.m_fontStyle;

    case FontWeightValueKind:
        return m_value.m_fontWeight == src.m_value.m_fontWeight;

    case WordWrapValueKind:
        return m_value.m_wordWrap == src.m_value.m_wordWrap;

    case BorderStyleValueKind:
        return m_value.m_borderStyle == src.m_value.m_borderStyle;

    case BorderWidthValueKind:
        return m_value.m_borderWidth == src.m_value.m_borderWidth;

    case BorderImageRepeatValueKind:
        return m_value.m_borderImageRepeat == src.m_value.m_borderImageRepeat;

    case BorderCollapseValueKind:
        return m_value.m_borderCollapse == src.m_value.m_borderCollapse;

    case CaptionSideValueKind:
        return m_value.m_captionSide == src.m_value.m_captionSide;

    case TableLayoutValueKind:
        return m_value.m_tableLayout == src.m_value.m_tableLayout;

    case EmptyCellsValueKind:
        return m_value.m_emptyCells == src.m_value.m_emptyCells;

    case OverflowValueKind:
        return m_value.m_overflow == src.m_value.m_overflow;

    case TextDecorationLineValueKind:
        return m_value.m_textDecorationLine == src.m_value.m_textDecorationLine;

    case TextDecorationStyleValueKind:
        return m_value.m_textDecorationStyle ==
               src.m_value.m_textDecorationStyle;

    case TextUnderlinePositionValueKind:
        return m_value.m_textUnderlinePosition ==
               src.m_value.m_textUnderlinePosition;

    case ResizeValueKind:
        return m_value.m_resize == src.m_value.m_resize;

    case VisibilityValueKind:
        return m_value.m_visibility == src.m_value.m_visibility;

    case UnicodeBidiValueKind:
        return m_value.m_unicodeBidi == src.m_value.m_unicodeBidi;

    case BoxSizingValueKind:
        return m_value.m_boxSizing == src.m_value.m_boxSizing;

    case BoxDecorationBreakValueKind:
        return m_value.m_boxDecorationBreakValue ==
               src.m_value.m_boxDecorationBreakValue;

    case FlexDirectionValueKind:
        return m_value.m_flexDirection == src.m_value.m_flexDirection;

    case FlexWrapValueKind:
        return m_value.m_flexWrap == src.m_value.m_flexWrap;

    case JustifyContentValueKind:
        return m_value.m_justifyContent == src.m_value.m_justifyContent;

    case AlignItemValueKind:
        return m_value.m_alignItem == src.m_value.m_alignItem;

    case AlignContentValueKind:
        return m_value.m_alignContent == src.m_value.m_alignContent;

    case FlexBasisValueKind:
        return m_value.m_flexBasis == src.m_value.m_flexBasis;

    case TransformFunctions:
        return m_value.m_transforms->equals(src.m_value.m_transforms);

    case TimingFunctionValueKind:
        return m_value.m_timingFunctionValue ==
               src.m_value.m_timingFunctionValue;

    case TimingFunctionPointerKind:
        return *m_value.m_timingFunction == *src.m_value.m_timingFunction;

    case AnimationDirectionValueKind:
        return m_value.m_animationDirectionValue ==
               src.m_value.m_animationDirectionValue;

    case AnimationPlayStateValueKind:
        return m_value.m_animationPlayStateValue ==
               src.m_value.m_animationPlayStateValue;

    case AnimationFillModeValueKind:
        return m_value.m_animationFillModeValue ==
               src.m_value.m_animationFillModeValue;

    case QuoteValueKind:
        return m_value.m_quote == src.m_value.m_quote;

    case FillRuleValueKind:
        return m_value.m_fillRule == src.m_value.m_fillRule;

    case TextTransformValueKind:
        return m_value.m_textTransform == src.m_value.m_textTransform;

    case ObjectFitValueKind:
        return m_value.m_objectFit == src.m_value.m_objectFit;

    case ListStylePositionValueKind:
        return m_value.m_listStylePosition == src.m_value.m_listStylePosition;

    case CounterFunctionValueKind:
        return m_value.m_counterFunctionValue ==
               src.m_value.m_counterFunctionValue;

    case RectValueKind:
        return *m_value.m_rect == *src.m_value.m_rect;

    case UserSelectValueKind:
        return m_value.m_userSelect == src.m_value.m_userSelect;

    case GridTemplateUnits:
        return *m_value.m_gridTemplateUnits == *src.m_value.m_gridTemplateUnits;

    case ImageRenderingValueKind:
        return m_value.m_imageRendering == src.m_value.m_imageRendering;

    case TextOverflowValueKind:
        return *m_value.m_textOverflowData == *src.m_value.m_textOverflowData;

    case HyphensValueKind:
        return m_value.m_hyphens == src.m_value.m_hyphens;

    case LineBreakValueKind:
        return m_value.m_lineBreak == src.m_value.m_lineBreak;

    case WordBreakValueKind:
        return m_value.m_wordBreak == src.m_value.m_wordBreak;

    case AppearanceValueKind:
        return m_value.m_appearance == src.m_value.m_appearance;

    case GradientValueKind:
        return m_value.m_gradientValue->equals(src.m_value.m_gradientValue);

    case WidthHeightKeywordValueKind:
        return m_value.m_widthHeightKeywordValue ==
               src.m_value.m_widthHeightKeywordValue;

    case PointerEventsValueKind:
        return m_value.m_pointerEventsValue == src.m_value.m_pointerEventsValue;

    case BlendModeValueKind:
        return m_value.m_blendMode == src.m_value.m_blendMode;

    default:
        STARFISH_ASSERT_NOT_REACHED();
        break;
    }

    return true;
}

bool CSSStyleValuePair::operator==(const CSSStyleValuePair& src) const
{
    if (m_flagImportant != src.m_flagImportant) {
        return false;
    }
    if (m_keyKind != src.m_keyKind) {
        return false;
    }

    return valueEquals(src);
}

void* CSSStyleValuePair::toPointerValueIfPossible() const
{
    void* ptr = nullptr;
    switch (m_valueKind) {
    case StringValueKind:
    case KeywordValueKind:
    case UrlValueKind:
    case PathFunctionValueKind:
    case Attr:
    case VarFunctionValueKind:
        ptr = m_value.m_stringValue;
        break;
    case TransformFunctions:
        ptr = m_value.m_transforms;
        break;
    case CalcValueKind:
        ptr = m_value.m_calc;
        break;
    case FontFaceSrcDataValueKind:
        ptr = m_value.m_fontFaceSrcData;
        break;
    case RectValueKind:
        ptr = m_value.m_rect;
        break;
    case GridTemplateUnits:
        ptr = m_value.m_gridTemplateUnits;
        break;
    case GridTemplateAreasValueKind:
        ptr = m_value.m_gridTemplateAreas;
        break;
    case CounterFunctionValueKind:
        ptr = m_value.m_counterFunctionValue;
        break;
    case TextOverflowValueKind:
        ptr = m_value.m_textOverflowData;
        break;
    case GradientValueKind:
        ptr = m_value.m_gradientValue;
        break;
    case TimingFunctionPointerKind:
        ptr = m_value.m_timingFunction;
        break;
    case FilterFunctionValueKind:
        ptr = m_value.m_filterFunction;
        break;
    case ValuePairKind:
        ptr = m_value.m_pairValue;
        break;
    case ValueListKind:
        ptr = m_value.m_multiValue;
        break;
    default:
        break;
    }

    return ptr;
}

void CSSStyleValuePair::rootPointerValue(GCVector<void*>& rooter) const
{
    void* ptr = toPointerValueIfPossible();
    switch (m_valueKind) {
    case ValuePairKind:
        m_value.m_pairValue->first().rootPointerValue(rooter);
        m_value.m_pairValue->second().rootPointerValue(rooter);
        break;
    case ValueListKind:
        ptr = m_value.m_multiValue;
        for (size_t i = 0; i < m_value.m_multiValue->size(); i++) {
            m_value.m_multiValue->at(i).rootPointerValue(rooter);
        }
        break;
    default:
        break;
    }

    if (ptr) {
        rooter.push_back(ptr);
    }
}

void CSSStyleValuePair::unrootPointerValue(GCVector<void*>& rooter) const
{
    void* ptr = toPointerValueIfPossible();
    switch (m_valueKind) {
    case ValuePairKind:
        m_value.m_pairValue->first().unrootPointerValue(rooter);
        m_value.m_pairValue->second().unrootPointerValue(rooter);
        break;
    case ValueListKind:
        for (size_t i = 0; i < m_value.m_multiValue->size(); i++) {
            m_value.m_multiValue->at(i).unrootPointerValue(rooter);
        }
        break;
    default:
        break;
    }

    if (ptr) {
        rooter.erase(std::remove_if(rooter.begin(), rooter.end(),
                                    [ptr](void* p) { return ptr == p; }),
                     rooter.end());
    }
}

String* CSSStyleValuePair::toString() const
{
    switch (valueKind()) {
    case CSSStyleValuePair::ValueKind::Attr:
        return attrValue();
    case CSSStyleValuePair::ValueKind::Initial:
        return String::initialString;
    case CSSStyleValuePair::ValueKind::Inherit:
        return String::inheritString;
    case CSSStyleValuePair::ValueKind::Unset:
        return String::unsetString;
    case CSSStyleValuePair::ValueKind::Length:
        return cssLengthValue().toString();
    case CSSStyleValuePair::ValueKind::CalcValueKind:
        return calcValue()->toString();
    case CSSStyleValuePair::ValueKind::Percentage: {
        StringBuilder builder;
        builder.appendString(String::fromFloat(percentageValue() * 100.f));
        builder.appendChar('%');
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::Auto:
        return String::fromUTF8("auto");
    case CSSStyleValuePair::ValueKind::None:
        return String::fromUTF8("none");
    case CSSStyleValuePair::ValueKind::Number:
        return String::fromFloat(numberValue());
    case CSSStyleValuePair::ValueKind::Int32: {
#if defined(STARFISH_ANDROID)
        return String::fromInt(int32Value());
#else
        auto str = std::to_string(int32Value());
        return String::fromUTF8(str.data(), str.size());
#endif
    }
    case CSSStyleValuePair::ValueKind::Angle:
        return angleValue().toString();
    case CSSStyleValuePair::ValueKind::Normal:
        return String::fromUTF8("normal");
    case CSSStyleValuePair::ValueKind::StringValueKind: {
        StringBuilder builder;
        builder.appendString("\"");
        builder.appendString(stringValue());
        builder.appendString("\"");
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::AtomicStringValueKind:
        return atomicStringValue().string();
    case CSSStyleValuePair::ValueKind::KeywordValueKind:
        return keywordValue();
    case CSSStyleValuePair::ValueKind::ColorValueKind:
        return colorValue().toString();
    case CSSStyleValuePair::ValueKind::NamedColorValueKind:
        return NamedColor::namedColorToString(namedColorValue());
    case CSSStyleValuePair::ValueKind::UrlValueKind: {
        StringBuilder builder;
        builder.appendString("url(\"");
        builder.appendString(urlStringValue());
        builder.appendString("\")");
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::DisplayValueKind:
        switch (displayValue()) {
        case DisplayValue::InlineDisplayValue:
            return String::fromUTF8("inline");
        case DisplayValue::BlockDisplayValue:
            return String::fromUTF8("block");
        case DisplayValue::InlineListItemDisplayValue:
            return String::fromUTF8("inline-list-item");
        case DisplayValue::ListItemDisplayValue:
            return String::fromUTF8("list-item");
        case DisplayValue::InlineBlockDisplayValue:
            return String::fromUTF8("inline-block");
        case DisplayValue::TableDisplayValue:
            return String::fromUTF8("table");
        case DisplayValue::InlineTableDisplayValue:
            return String::fromUTF8("inline-table");
        case DisplayValue::TableRowGroupDisplayValue:
            return String::fromUTF8("table-row-group");
        case DisplayValue::TableHeaderGroupDisplayValue:
            return String::fromUTF8("table-header-group");
        case DisplayValue::TableFooterGroupDisplayValue:
            return String::fromUTF8("table-footer-group");
        case DisplayValue::TableRowDisplayValue:
            return String::fromUTF8("table-row");
        case DisplayValue::TableColumnGroupDisplayValue:
            return String::fromUTF8("table-column-group");
        case DisplayValue::TableColumnDisplayValue:
            return String::fromUTF8("table-column");
        case DisplayValue::TableCellDisplayValue:
            return String::fromUTF8("table-cell");
        case DisplayValue::TableCaptionDisplayValue:
            return String::fromUTF8("table-caption");
        case DisplayValue::FlexDisplayValue:
            return String::fromUTF8("flex");
        case DisplayValue::InlineFlexDisplayValue:
            return String::fromUTF8("inline-flex");
        case DisplayValue::GridDisplayValue:
            return String::fromUTF8("grid");
        case DisplayValue::InlineGridDisplayValue:
            return String::fromUTF8("inline-grid");
        case DisplayValue::BoxDisplayValue:
            return String::fromUTF8("-webkit-box");
        case DisplayValue::InlineBoxDisplayValue:
            return String::fromUTF8("-webkit-inline-box");
        case DisplayValue::NoneDisplayValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::PositionValueKind:
        switch (positionValue()) {
        case PositionValue::StaticPositionValue:
            return String::fromUTF8("static");
        case PositionValue::RelativePositionValue:
            return String::fromUTF8("relative");
        case PositionValue::AbsolutePositionValue:
            return String::fromUTF8("absolute");
        case PositionValue::FixedPositionValue:
            return String::fromUTF8("fixed");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FloatValueKind:
        switch (floatValue()) {
        case FloatValue::LeftFloatValue:
            return String::fromUTF8("left");
        case FloatValue::RightFloatValue:
            return String::fromUTF8("right");
        case FloatValue::NoneFloatValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::ClearValueKind:
        switch (clearValue()) {
        case ClearValue::LeftClearValue:
            return String::fromUTF8("left");
        case ClearValue::RightClearValue:
            return String::fromUTF8("right");
        case ClearValue::BothClearValue:
            return String::fromUTF8("both");
        case ClearValue::NoneClearValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::VerticalAlignValueKind:
        switch (verticalAlignValue()) {
        case VerticalAlignValue::BaselineVAlignValue:
            return String::fromUTF8("baseline");
        case VerticalAlignValue::SubVAlignValue:
            return String::fromUTF8("sub");
        case VerticalAlignValue::SuperVAlignValue:
            return String::fromUTF8("super");
        case VerticalAlignValue::TopVAlignValue:
            return String::fromUTF8("top");
        case VerticalAlignValue::TextTopVAlignValue:
            return String::fromUTF8("text-top");
        case VerticalAlignValue::MiddleVAlignValue:
            return String::fromUTF8("middle");
        case VerticalAlignValue::BottomVAlignValue:
            return String::fromUTF8("bottom");
        case VerticalAlignValue::TextBottomVAlignValue:
            return String::fromUTF8("text-bottom");
        case VerticalAlignValue::NumericVAlignValue:
            // FIXME:mh.byun
            // FIXED: NumericVAlignValue cannot be here. (only used in
            // ComputedStyle)
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            break;
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextAlignValueKind:
        switch (textAlignValue()) {
        case TextAlignValue::StartTextAlignValue:
            return String::fromUTF8("start");
        case TextAlignValue::EndTextAlignValue:
            return String::fromUTF8("end");
        case TextAlignValue::LeftTextAlignValue:
            return String::fromUTF8("left");
        case TextAlignValue::RightTextAlignValue:
            return String::fromUTF8("right");
        case TextAlignValue::CenterTextAlignValue:
            return String::fromUTF8("center");
        case TextAlignValue::WebKitCenterTextAlignValue:
            return String::fromUTF8("-webkit-center");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextTransformValueKind:
        switch (textTransformValue()) {
        case TextTransformValue::NoneTextTransformValue:
            return String::fromUTF8("none");
        case TextTransformValue::CapitalizeTextTransformValue:
            return String::fromUTF8("capitalize");
        case TextTransformValue::UppercaseTextTransformValue:
            return String::fromUTF8("uppercase");
        case TextTransformValue::LowercaseTextTransformValue:
            return String::fromUTF8("lowercase");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::SideValueKind:
        switch (sideValue()) {
        case SideValue::LeftSideValue:
            return String::fromUTF8("left");
        case SideValue::RightSideValue:
            return String::fromUTF8("right");
        case SideValue::CenterSideValue:
            return String::fromUTF8("center");
        case SideValue::TopSideValue:
            return String::fromUTF8("top");
        case SideValue::BottomSideValue:
            return String::fromUTF8("bottom");
        case SideValue::NoneSideValue:
            return String::emptyString;
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::DirectionValueKind:
        switch (directionValue()) {
        case LtrDirectionValue:
            return String::fromUTF8("ltr");
        case RtlDirectionValue:
            return String::fromUTF8("rtl");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::WhiteSpaceValueKind:
        switch (whiteSpaceValue()) {
        case NormalWhiteSpaceValue:
            return String::fromUTF8("normal");
        case NoWrapWhiteSpaceValue:
            return String::fromUTF8("nowrap");
        case PreWhiteSpaceValue:
            return String::fromUTF8("pre");
        case PreWrapWhiteSpaceValue:
            return String::fromUTF8("pre-wrap");
        case PreLineWhiteSpaceValue:
            return String::fromUTF8("pre-line");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::PointerEventsValueKind:
        switch (pointerEventsValue()) {
        case PointerEventsAutoValue:
            return String::fromUTF8("auto");
        case PointerEventsNoneValue:
            return String::fromUTF8("none");
        case PointerEventsVisibleFillValue:
            return String::fromUTF8("visibleFill");
        case PointerEventsVisiblePaintedValue:
            return String::fromUTF8("visiblePainted");
        case PointerEventsVisibleStrokeValue:
            return String::fromUTF8("visibleStroke");
        case PointerEventsVisibleValue:
            return String::fromUTF8("visible");
        case PointerEventsPaintedValue:
            return String::fromUTF8("painted");
        case PointerEventsFillValue:
            return String::fromUTF8("fill");
        case PointerEventsStrokeValue:
            return String::fromUTF8("stroke");
        case PointerEventsAllValue:
            return String::fromUTF8("all");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BlendModeValueKind: {
        auto p = CanvasBlend::blendModeNames[static_cast<unsigned>(
            blendModeValue())];
        return String::fromUTF8(p, strlen(p));
    }
    case CSSStyleValuePair::ValueKind::ObjectFitValueKind:
        switch (objectFitValue()) {
        case FillObjectFitValue:
            return String::fromUTF8("fill");
        case ContainObjectFitValue:
            return String::fromUTF8("contain");
        case CoverObjectFitValue:
            return String::fromUTF8("cover");
        case NoneObjectFitValue:
            return String::fromUTF8("none");
        case ScaledownObjectFitValue:
            return String::fromUTF8("scale-down");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BackgroundSizeValueKind:
        switch (backgroundSizeValue()) {
        case CoverBackgroundSizeValue:
            return String::fromUTF8("cover");
        case ContainBackgroundSizeValue:
            return String::fromUTF8("contain");
        }
        break;
    case CSSStyleValuePair::ValueKind::RepeatStyleValueKind:
        switch (repeatStyleValue()) {
        case RepeatRepeatValue:
            return String::fromUTF8("repeat");
        case NoRepeatRepeatValue:
            return String::fromUTF8("no-repeat");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BackgroundAttachmentValueKind:
        switch (backgroundAttachmentValue()) {
        case ScrollBackgroundAttachmentValue:
            return String::fromUTF8("scroll");
        case FixedBackgroundAttachmentValue:
            return String::fromUTF8("fixed");
        case LocalBackgroundAttachmentValue:
            return String::fromUTF8("local");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BoxValueKind:
        switch (boxValue()) {
        case BorderBoxBoxValue:
            return String::fromUTF8("border-box");
        case PaddingBoxBoxValue:
            return String::fromUTF8("padding-box");
        case ContentBoxBoxValue:
            return String::fromUTF8("content-box");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontSizeValueKind:
        switch (fontSizeValue()) {
        case FontSizeValue::XXSmallFontSizeValue:
            return String::fromUTF8("xx-small");
        case FontSizeValue::XSmallFontSizeValue:
            return String::fromUTF8("x-small");
        case FontSizeValue::SmallFontSizeValue:
            return String::fromUTF8("small");
        case FontSizeValue::MediumFontSizeValue:
            return String::fromUTF8("medium");
        case FontSizeValue::LargeFontSizeValue:
            return String::fromUTF8("large");
        case FontSizeValue::XLargeFontSizeValue:
            return String::fromUTF8("x-large");
        case FontSizeValue::XXLargeFontSizeValue:
            return String::fromUTF8("xx-large");
        case FontSizeValue::LargerFontSizeValue:
            return String::fromUTF8("larger");
        case FontSizeValue::SmallerFontSizeValue:
            return String::fromUTF8("smaller");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontStyleValueKind:
        switch (fontStyleValue()) {
        case FontStyleValue::NormalFontStyleValue:
            return String::fromUTF8("normal");
        case FontStyleValue::ItalicFontStyleValue:
            return String::fromUTF8("italic");
        case FontStyleValue::ObliqueFontStyleValue:
            return String::fromUTF8("oblique");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontWeightValueKind:
        switch (fontWeightValue()) {
        case FontWeightValue::NormalFontWeightValue:
            return String::fromUTF8("normal");
        case FontWeightValue::BoldFontWeightValue:
            return String::fromUTF8("bold");
        case FontWeightValue::BolderFontWeightValue:
            return String::fromUTF8("bolder");
        case FontWeightValue::LighterFontWeightValue:
            return String::fromUTF8("lighter");
        case FontWeightValue::OneHundredFontWeightValue:
            return String::fromUTF8("100");
        case FontWeightValue::TwoHundredsFontWeightValue:
            return String::fromUTF8("200");
        case FontWeightValue::ThreeHundredsFontWeightValue:
            return String::fromUTF8("300");
        case FontWeightValue::FourHundredsFontWeightValue:
            return String::fromUTF8("400");
        case FontWeightValue::FiveHundredsFontWeightValue:
            return String::fromUTF8("500");
        case FontWeightValue::SixHundredsFontWeightValue:
            return String::fromUTF8("600");
        case FontWeightValue::SevenHundredsFontWeightValue:
            return String::fromUTF8("700");
        case FontWeightValue::EightHundredsFontWeightValue:
            return String::fromUTF8("800");
        case FontWeightValue::NineHundredsFontWeightValue:
            return String::fromUTF8("900");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontKerningValueKind:
        switch (fontKerningValue()) {
        case FontKerningValue::FontKerningAutoValue:
            return String::fromUTF8("auto");
        case FontKerningValue::FontKerningNormalValue:
            return String::fromUTF8("normal");
        case FontKerningValue::FontKerningNoneValue:
            return String::fromUTF8("none");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::WordWrapValueKind:
        switch (wordWrapValue()) {
        case WordWrapValue::NormalWordWrapValue:
            return String::fromUTF8("normal");
        case WordWrapValue::BreakWordWordWrapValue:
            return String::fromUTF8("break-word");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BorderStyleValueKind:
        switch (borderStyleValue()) {
        case BorderStyleValue::NoneBorderStyleValue:
            return String::fromUTF8("none");
        case BorderStyleValue::HiddenBorderStyleValue:
            return String::fromUTF8("hidden");
        case BorderStyleValue::SolidBorderStyleValue:
            return String::fromUTF8("solid");
        case BorderStyleValue::DashedBorderStyleValue:
            return String::fromUTF8("dashed");
        case BorderStyleValue::DottedBorderStyleValue:
            return String::fromUTF8("dotted");
        case BorderStyleValue::InsetBorderStyleValue:
            return String::fromUTF8("inset");
        case BorderStyleValue::OutsetBorderStyleValue:
            return String::fromUTF8("outset");
        case BorderStyleValue::DoubleBorderStyleValue:
            return String::fromUTF8("double");
        case BorderStyleValue::GrooveBorderStyleValue:
            return String::fromUTF8("groove");
        case BorderStyleValue::RidgeBorderStyleValue:
            return String::fromUTF8("ridge");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BorderWidthValueKind:
        switch (borderWidthValue()) {
        case BorderWidthValue::ThinBorderWidthValue:
            return String::fromUTF8("thin");
        case BorderWidthValue::MediumBorderWidthValue:
            return String::fromUTF8("medium");
        case BorderWidthValue::ThickBorderWidthValue:
            return String::fromUTF8("thick");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BorderImageRepeatValueKind:
        switch (borderImageRepeatValue()) {
        case BorderImageRepeatValue::StretchValue:
            return String::fromUTF8("stretch");
        case BorderImageRepeatValue::RepeatValue:
            return String::fromUTF8("repeat");
        case BorderImageRepeatValue::RoundValue:
            return String::fromUTF8("round");
        case BorderImageRepeatValue::SpaceValue:
            return String::fromUTF8("space");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::OverflowValueKind:
        switch (overflowValue()) {
        case OverflowValue::VisibleOverflow:
            return String::fromUTF8("visible");
        case OverflowValue::HiddenOverflow:
            return String::fromUTF8("hidden");
        case OverflowValue::AutoOverflow:
            return String::fromUTF8("auto");
        case OverflowValue::ScrollOverflow:
            return String::fromUTF8("scroll");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextDecorationLineValueKind:
        switch (textDecorationLineValue()) {
        case TextDecorationLineValue::NoneTextDecorationLineValue:
            return String::fromUTF8("none");
        case TextDecorationLineValue::UnderlineTextDecorationLineValue:
            return String::fromUTF8("underline");
        case TextDecorationLineValue::OverlineTextDecorationLineValue:
            return String::fromUTF8("overline");
        case TextDecorationLineValue::LineThroughTextDecorationLineValue:
            return String::fromUTF8("line-through");
        case TextDecorationLineValue::BlinkTextDecorationLineValue:
            return String::fromUTF8("blink");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextDecorationStyleValueKind:
        switch (textDecorationStyleValue()) {
        case TextDecorationStyleValue::SolidTextDecorationStyleValue:
            return String::fromUTF8("solid");
        case TextDecorationStyleValue::DoubleTextDecorationStyleValue:
            return String::fromUTF8("double");
        case TextDecorationStyleValue::DottedTextDecorationStyleValue:
            return String::fromUTF8("dotted");
        case TextDecorationStyleValue::DashedTextDecorationStyleValue:
            return String::fromUTF8("dashed");
        case TextDecorationStyleValue::WavyTextDecorationStyleValue:
            return String::fromUTF8("wavy");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TextUnderlinePositionValueKind:
        switch (textUnderlinePositionValue()) {
        case TextUnderlinePositionValue::AutoTextUnderlinePositionValue:
            return String::fromUTF8("auto");
        case TextUnderlinePositionValue::UnderTextUnderlinePositionValue:
            return String::fromUTF8("under");
        case TextUnderlinePositionValue::LeftTextUnderlinePositionValue:
            return String::fromUTF8("left");
        case TextUnderlinePositionValue::RightTextUnderlinePositionValue:
            return String::fromUTF8("right");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::ResizeValueKind:
        switch (resizeValue()) {
        case ResizeValue::NoneResizeValue:
            return String::fromUTF8("none");
        case ResizeValue::BothResizeValue:
            return String::fromUTF8("both");
        case ResizeValue::HorizontalResizeValue:
            return String::fromUTF8("horizontal");
        case ResizeValue::VerticalResizeValue:
            return String::fromUTF8("vertical");
        case ResizeValue::BlockResizeValue:
            return String::fromUTF8("block");
        case ResizeValue::InlineResizeValue:
            return String::fromUTF8("inline");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::VisibilityValueKind:
        switch (visibilityValue()) {
        case VisibilityValue::VisibleVisibilityValue:
            return String::fromUTF8("visible");
        case VisibilityValue::CollapseVisibilityValue:
            return String::fromUTF8("collapse");
        case VisibilityValue::HiddenVisibilityValue:
            return String::fromUTF8("hidden");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::ImageRenderingValueKind:
        switch (imageRenderingValue()) {
        case ImageRenderingValue::ImageRenderingAutoValue:
            return String::fromUTF8("auto");
        case ImageRenderingValue::ImageRenderingPixelatedValue:
            return String::fromUTF8("pixelated");
        case ImageRenderingValue::ImageRenderingCrispEdgesValue:
            return String::fromUTF8("crisp-edges");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::UnicodeBidiValueKind:
        switch (unicodeBidiValue()) {
        case NormalUnicodeBidiValue:
            return String::fromUTF8("normal");
        case EmbedUnicodeBidiValue:
            return String::fromUTF8("embed");
        case IsolateUnicodeBidiValue:
            return String::fromUTF8("isolate");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TransformFunctions:
        return transformValue()->toString();
    case CSSStyleValuePair::ValueKind::ValueListKind: {
        ValueList* list = multiValue();
        return list->toString();
    }
    case CSSStyleValuePair::ValueKind::ValuePairKind: {
        return pairValue()->toString();
    }
    case CSSStyleValuePair::ValueKind::CSSPropertyNameValueKind:
        return CSSPropertyHelper::toGCString(cssPropertyNameValue());
    case CSSStyleValuePair::ValueKind::Time:
        return timeValue().toString();
    case CSSStyleValuePair::ValueKind::TimingFunctionValueKind:
        switch (timingFunctionValue()) {
        case TimingFunctionEaseValue:
            return String::fromUTF8("ease");
        case TimingFunctionLinearValue:
            return String::fromUTF8("linear");
        case TimingFunctionEaseInValue:
            return String::fromUTF8("ease-in");
        case TimingFunctionEaseOutValue:
            return String::fromUTF8("ease-out");
        case TimingFunctionEaseInOutValue:
            return String::fromUTF8("ease-in-out");
        case TimingFunctionStepStartValue:
            return String::fromUTF8("step-start");
        case TimingFunctionStepEndValue:
            return String::fromUTF8("step-end");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::AnimationDirectionValueKind:
        switch (animationDirectionValue()) {
        case AnimationDirectionValue::Normal:
            return String::fromUTF8("normal");
        case AnimationDirectionValue::Reverse:
            return String::fromUTF8("reverse");
        case AnimationDirectionValue::Alternate:
            return String::fromUTF8("alternate");
        case AnimationDirectionValue::AlternateReverse:
            return String::fromUTF8("alternate-reverse");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::AnimationPlayStateValueKind:
        switch (animationPlayStateValue()) {
        case AnimationPlayStateValue::Running:
            return String::fromUTF8("running");
        case AnimationPlayStateValue::Paused:
            return String::fromUTF8("paused");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::AnimationFillModeValueKind:
        switch (animationFillModeValue()) {
        case AnimationFillModeValue::None:
            return String::fromUTF8("none");
        case AnimationFillModeValue::Forwards:
            return String::fromUTF8("forwards");
        case AnimationFillModeValue::Backwards:
            return String::fromUTF8("backwards");
        case AnimationFillModeValue::Both:
            return String::fromUTF8("both");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BoxSizingValueKind:
        switch (boxSizingValue()) {
        case ContentBoxBoxSizingValue:
            return String::fromUTF8("content-box");
        case BorderBoxBoxSizingValue:
            return String::fromUTF8("border-box");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::BoxOrientValueKind:
        switch (boxOrientValue()) {
        case BoxOrientValue::HorizontalBoxOrientValue:
            return String::fromUTF8("horizontal");
        case BoxOrientValue::VerticalBoxOrientValue:
            return String::fromUTF8("vertical");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            break;
        }
        break;
    case CSSStyleValuePair::ValueKind::FlexDirectionValueKind:
        switch (flexDirectionValue()) {
        case RowFlexDirectionValue:
            return String::fromUTF8("row");
        case RowReverseFlexDirectionValue:
            return String::fromUTF8("row-reverse");
        case ColumnFlexDirectionValue:
            return String::fromUTF8("column");
        case ColumnReverseFlexDirectionValue:
            return String::fromUTF8("column-reverse");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FlexWrapValueKind:
        switch (flexWrapValue()) {
        case NoWrapFlexWrapValue:
            return String::fromUTF8("nowrap");
        case WrapFlexWrapValue:
            return String::fromUTF8("wrap");
        case WrapReverseFlexWrapValue:
            return String::fromUTF8("wrap-reverse");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::JustifyContentValueKind:
        switch (justifyContentValue()) {
        case NormalJustifyContentValue:
            return String::fromUTF8("normal");
        case FlexStartJustifyContentValue:
            return String::fromUTF8("flex-start");
        case FlexEndJustifyContentValue:
            return String::fromUTF8("flex-end");
        case StartJustifyContentValue:
            return String::fromUTF8("start");
        case CenterJustifyContentValue:
            return String::fromUTF8("center");
        case EndJustifyContentValue:
            return String::fromUTF8("end");
        case SpaceBetweenJustifyContentValue:
            return String::fromUTF8("space-between");
        case SpaceAroundJustifyContentValue:
            return String::fromUTF8("space-around");
        case StretchJustifyContentValue:
            return String::fromUTF8("stretch");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::AlignItemValueKind:
        switch (alignItemValue()) {
        case FlexStartAlignItemValue:
            return String::fromUTF8("flex-start");
        case FlexEndAlignItemValue:
            return String::fromUTF8("flex-end");
        case StartAlignItemValue:
            return String::fromUTF8("start");
        case CenterAlignItemValue:
            return String::fromUTF8("center");
        case EndAlignItemValue:
            return String::fromUTF8("end");
        case BaselineAlignItemValue:
            return String::fromUTF8("baseline");
        case StretchAlignItemValue:
            return String::fromUTF8("stretch");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::AlignContentValueKind:
        switch (alignContentValue()) {
        case FlexStartAlignContentValue:
            return String::fromUTF8("flex-start");
        case FlexEndAlignContentValue:
            return String::fromUTF8("flex-end");
        case SpaceBetweenAlignContentValue:
            return String::fromUTF8("space-between");
        case SpaceAroundAlignContentValue:
            return String::fromUTF8("space-around");
        case StretchAlignContentValue:
            return String::fromUTF8("stretch");
        case CenterAlignContentValue:
            return String::fromUTF8("center");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FlexBasisValueKind:
        switch (flexBasisValue()) {
        case AutoFlexBasisValue:
            return String::fromUTF8("auto");
        case ContentFlexBasisValue:
            return String::fromUTF8("content");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::TableLayoutValueKind:
        switch (tableLayoutValue()) {
        case TableLayoutValue::AutoTableLayoutValue:
            return String::fromUTF8("auto");
        case TableLayoutValue::FixedTableLayoutValue:
            return String::fromUTF8("fixed");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FillRuleValueKind:
        switch (fillRuleValue()) {
        case FillRuleNonZero:
            return String::fromUTF8("nonzero");
        case FillRuleEvenOdd:
            return String::fromUTF8("evenodd");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::FontFaceSrcDataValueKind:
        return fontFaceSrcDataValue()->toString();
    case CSSStyleValuePair::ValueKind::ListStylePositionValueKind:
        switch (listStylePositionValue()) {
        case ListStylePositionOutside:
            return String::fromUTF8("outside");
        case ListStylePositionInside:
            return String::fromUTF8("inside");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::RectValueKind:
        if (m_valueKind == CSSStyleValuePair::ValueKind::RectValueKind) {
            return clip()->toString();
        }
        return String::fromUTF8("auto");
    case CSSStyleValuePair::ValueKind::BorderCollapseValueKind:
        switch (borderCollapseValue()) {
        case SeparateBorderCollapseValue:
            return String::fromUTF8("separate");
        case CollapseBorderCollapseValue:
            return String::fromUTF8("collapse");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::CaptionSideValueKind:
        switch (captionSideValue()) {
        case TopCaptionSideValue:
            return String::fromUTF8("top");
        case BottomCaptionSideValue:
            return String::fromUTF8("bottom");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::EmptyCellsValueKind:
        switch (emptyCellsValue()) {
        case ShowEmptyCellsValue:
            return String::fromUTF8("show");
        case HideEmptyCellsValue:
            return String::fromUTF8("hide");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::UserSelectValueKind:
        switch (userSelectValue()) {
        case NoneUserSelectValue:
            return String::fromUTF8("none");
        case TextUserSelectValue:
            return String::fromUTF8("text");
        case ContainUserSelectValue:
            return String::fromUTF8("contain");
        case AllUserSelectValue:
            return String::fromUTF8("all");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::HyphensValueKind:
        switch (hyphensValue()) {
        case NoneHyphensValue:
            return String::fromUTF8("none");
        case ManualHyphensValue:
            return String::fromUTF8("manual");
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::ValueKind::LineBreakValueKind:
        switch (lineBreakValue()) {
        case LooseLineBreakValue:
            return String::fromUTF8("loose");
        case NormalLineBreakValue:
            return String::fromUTF8("normal");
        case StrictLineBreakValue:
            return String::fromUTF8("strict");
        }
        break;
    case CSSStyleValuePair::ValueKind::WordBreakValueKind:
        switch (wordBreakValue()) {
        case NormalWordBreakValue:
            return String::fromUTF8("normal");
        case BreakAllWordBreakValue:
            return String::fromUTF8("break-all");
        case KeepAllWordBreakValue:
            return String::fromUTF8("keep-all");
        case BreakWordWordBreakValue:
            return String::fromUTF8("break-word");
        }
        break;
    case CSSStyleValuePair::ValueKind::AppearanceValueKind:
        switch (appearanceValue()) {
        case AutoAppearanceValue:
            return String::fromUTF8("auto");
        case NoneAppearanceValue:
            return String::fromUTF8("none");
        }
        break;
    case CSSStyleValuePair::ValueKind::GridTemplateUnits:
        return GridTrackSize::toStringWithGridLengths(gridTemplateUnits());
    case CSSStyleValuePair::ValueKind::GridTemplateAreasValueKind:
        return gridTemplateAreas()->toString();
    case CSSStyleValuePair::ValueKind::CounterFunctionValueKind:
        return counterFunctionValue()->toString();
    case CSSStyleValuePair::ValueKind::VarFunctionValueKind:
        return varFunctionValue();
    case CSSStyleValuePair::ValueKind::TextOverflowValueKind: {
        auto value = textOverflowValue();
        if (value.hasClipValue()) {
            return String::fromUTF8("auto");
        } else if (value.hasEllipsisValue()) {
            return String::fromUTF8("ellipsis");
        } else if (value.hasStringValue()) {
            StringBuilder builder;
            builder.appendString("\"");
            builder.appendString(value.stringValue());
            builder.appendString("\"");
            return builder.finalize();
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    }
    case CSSStyleValuePair::ValueKind::GradientValueKind:
        return gradientValue()->toString();
    case CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind:
        switch (widthHeightKeywordValue()) {
        case AvailableValue:
            return String::fromUTF8("available");
        case MinContentValue:
            return String::fromUTF8("min-content");
        case MaxContentValue:
            return String::fromUTF8("max-content");
        case FitContentValue:
            return String::fromUTF8("fit-content");
        }
        break;
    case CSSStyleValuePair::ValueKind::QuoteValueKind:
        switch (quoteValue()) {
        case OpenQuoteValue:
            return String::fromUTF8("open-quote");
        case CloseQuoteValue:
            return String::fromUTF8("close-quote");
        case NoOpenQuoteValue:
            return String::fromUTF8("no-open-quote");
        case NoCloseQuoteValue:
            return String::fromUTF8("no-close-quote");
        }
        break;
    case CSSStyleValuePair::ValueKind::BoxDecorationBreakValueKind:
        switch (boxDecorationBreakValue()) {
        case CloneBoxDecorationBreakValue:
            return String::fromUTF8("clone");
        case SliceBoxDecorationBreakValue:
            return String::fromUTF8("slice");
        }
        break;

    case CSSStyleValuePair::ValueKind::PathFunctionValueKind: {
        StringBuilder builder;
        builder.appendString("path(\"");
        builder.appendString(pathFunctionValue());
        builder.appendString("\")");
        return builder.finalize();
    }
    case CSSStyleValuePair::ValueKind::FilterFunctionValueKind:
        STARFISH_ASSERT(filterFunctionValue());
        return filterFunctionValue()->toString();
    case CSSStyleValuePair::ValueKind::TimingFunctionPointerKind:
        STARFISH_ASSERT(timingFunctionPointerValue());
        return timingFunctionPointerValue()->toString();
    case CSSStyleValuePair::ValueKind::MaskTypeValueKind:
        switch (maskTypeValue()) {
        case MaskTypeValue::LuminanceMaskTypeValue:
            return String::fromUTF8("luminance");
        case MaskTypeValue::AlphaMaskTypeValue:
            return String::fromUTF8("alpha");
        }
        break;
    case CSSStyleValuePair::ValueKind::StrokeLineCapValueKind:
        return strokeLineCapToString(strokeLineCap());
    case CSSStyleValuePair::ValueKind::StrokeLineJoinValueKind:
        return strokeLineJoinToString(strokeLineJoin());
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
}

static bool attributeValueMatches(
    String* attrValue, CSSSelector::Type type, String* selectorValue,
    CSSSelector::AttributeMatchType caseSensitivity)
{
    STARFISH_ASSERT(attrValue != nullptr);
    STARFISH_ASSERT(selectorValue != nullptr);

    // For AttributeSet and AttributeExact attribute selectors, the attribute
    // value can be empty string.
    if (type != CSSSelector::AttributeSet &&
        type != CSSSelector::AttributeExact &&
        attrValue->equals(String::emptyString)) {
        return false;
    }

    switch (type) {
    case CSSSelector::AttributeSet: // Example: E[foo]
        return true;
    case CSSSelector::AttributeExact: // Example: E[foo="bar"]
        if (caseSensitivity) {
            return selectorValue->equals(attrValue);
        }
        return selectorValue->equalsIgnoreCase(attrValue);
    case CSSSelector::AttributeList: // Example: E[foo~="bar"]
    {
        if (selectorValue->equals(String::emptyString) ||
            selectorValue->containsWhitespace()) {
            return false;
        }

        unsigned startSearchAt = 0;
        while (true) {
            size_t foundPos =
                attrValue->find(selectorValue, startSearchAt, caseSensitivity);
            if (foundPos == SIZE_MAX) {
                return false;
            }
            if (!foundPos ||
                String::isASCIISpace(attrValue->charAt(foundPos - 1))) {
                unsigned endStr = foundPos + selectorValue->length();
                if (endStr == attrValue->length() ||
                    String::isASCIISpace(attrValue->charAt(endStr))) {
                    break; // We found a match.
                }
            }

            // No match. Keep looking.
            startSearchAt = foundPos + 1;
        }
        return true;
    }
    case CSSSelector::AttributeHyphen: // Example: E[foo|="bar"]
        if (attrValue->length() < selectorValue->length()) {
            return false;
        }
        if (!attrValue->startsWith(selectorValue, caseSensitivity)) {
            return false;
        }
        // It they start the same, check for exact match or following '-':
        if (attrValue->length() != selectorValue->length() &&
            attrValue->charAt(selectorValue->length()) != '-') {
            return false;
        }
        return true;
    case CSSSelector::AttributeContain: // css3: E[foo*="bar"]
        if (selectorValue->equals(String::emptyString)) {
            return false;
        }
        return attrValue->contains(selectorValue, caseSensitivity);
    case CSSSelector::AttributeBegin: // css3: E[foo^="bar"]
        if (selectorValue->equals(String::emptyString)) {
            return false;
        }
        return attrValue->startsWith(selectorValue, caseSensitivity);
    case CSSSelector::AttributeEnd: // css3: E[foo$="bar"]
        if (selectorValue->equals(String::emptyString)) {
            return false;
        }
        return attrValue->endsWith(selectorValue, caseSensitivity);
    default:
        break;
    }

    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    return false;
}

bool StyleResolver::anyAttributeMatches(Element* element,
                                        CSSSelector::Type type,
                                        CSSAttributeSelector* selector,
                                        MatchResult& result)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(selector != nullptr);

    const QualifiedName& selectorAttr = selector->attribute();
    STARFISH_ASSERT(!(selectorAttr.localName()->equals("*")));

    String* selectorValue = selector->value();

    Optional<String*> refAttr = element->getAttribute(selectorAttr);
    if (!refAttr.hasValue()) {
        return false;
    }

    CSSSelector::AttributeMatchType caseSensitivity =
        selector->attributeMatch();
    if (attributeValueMatches(refAttr.getValue(), type, selectorValue,
                              caseSensitivity)) {
        return true;
    }

    if (caseSensitivity == CSSSelector::CaseInsensitive) {
        if (!selectorAttr.namespaceURI().hasValue() &&
            selectorAttr.namespaceURI().getValue().string()->equals("*")) {
            return false;
        }
    }

    // Legacy dictates that values of some attributes should be compared in
    // a case-insensitive manner regardless of whether the case insensitive
    // flag is set or not.
    bool legacyCaseInsensitive =
        !HTMLDocument::isCaseSensitiveAttribute(m_document, selectorAttr);

    // If case-insensitive, re-check, and count if result differs.
    // See http://code.google.com/p/chromium/issues/detail?id=327060
    if (legacyCaseInsensitive &&
        attributeValueMatches(refAttr.getValue(), type, selectorValue,
                              CSSSelector::CaseInsensitive)) {
        return true;
    }
    if (!selectorAttr.namespaceURI().hasValue() &&
        selectorAttr.namespaceURI().getValue().string()->equals("*")) {
        return false;
    }

    return false;
}

bool CSSStyleValuePair::updateValueUnitTransitionProperty(
    const CSSTokenValue& value)
{
    CSSStyleValuePair::KeyKind kind =
        CSSStyleLookupTrie::lookupCSSStyle(value.data(), value.length());
    if (CSSPropertyHelper::isAnimatableProperty(kind)) {
        setCSSPropertyNameValue(kind);
        return true;
    }
    if (kind != CSSStyleValuePair::Unknown ||
        CSSPropertyParser::stringIsIdent(
            String::fromUTF8(value.data(), value.length()))) {
        // NOTE Accept non-property or non-animatable ident as `Unknown`
        setCSSPropertyNameValue(CSSStyleValuePair::Unknown);
        return true;
    }
    return false;
}

StyleResolver::StyleResolver(Document* document)
    : DocumentHoldable(document)
    , m_usesFirstLineRule(false)
    , m_needsRecalcRuleSet(true)
    , m_hasSimplePseudoClassHostSelector(false)
    , m_mediumFontSize(document->webView()->defaultFontSize())
    , m_mediaQueryEvaluator(nullptr)
    , m_ruleSet(new RuleSet())
    , m_nextRuleSetOrder(0)
{
}

ComputedStyle* StyleResolver::resolveDocumentStyle(Document* document)
{
    STARFISH_ASSERT(document != nullptr);

    ComputedStyle* ret = new ComputedStyle(m_mediumFontSize);
    ret->m_display = DisplayValue::BlockDisplayValue;
    ret->m_inheritedStyles.m_color = document->webView()->baseForegroundColor();
    ret->m_inheritedStyles.m_fontFamilyDatas =
        document->webView()->initialFontFamilyDatas();
    ret->m_inheritedStyles.m_textAlign = TextAlignValue::StartTextAlignValue;
    ret->m_inheritedStyles.m_direction = DirectionValue::LtrDirectionValue;
    ret->m_inheritedStyles.m_whiteSpace =
        WhiteSpaceValue::NormalWhiteSpaceValue;
    ret->loadResources(document);
    return ret;
}

StyleResolveContext::StyleResolveContext(
    Node* node, Optional<GCVector<ComputedStyle*>*> computedStylePool)
    : m_styleResolver(&node->styleResolver())
    , m_ancestorSelectorFilter(new AncestorSelectorFilter())
    , m_computedStylePool(computedStylePool ? computedStylePool.value()
                                            : new GCVector<ComputedStyle*>())
{
    VectorWithInlineStorage<16, Node*, std::allocator<Node*>> tree;

    auto n = node->parentNode();
    while (n) {
        tree.push_back(n);
        n = n->parentNode();
    }

    for (size_t i = tree.size(); i > 0; i--) {
        m_ancestorSelectorFilter->pushNode(tree[i - 1]);
    }
}

StyleResolveContext::StyleResolveContext(StyleResolver* sr,
                                         StyleResolveContext& origin)
    : m_styleResolver(sr)
    , m_ancestorSelectorFilter(new AncestorSelectorFilter())
    , m_computedStylePool(origin.m_computedStylePool)
{
}

StyleResolveContext::~StyleResolveContext()
{
}

void StyleResolveContext::pushIntoComputedStylePool(ComputedStyle* b)
{
    STARFISH_ASSERT(b != nullptr);
    memset(b, 0, sizeof(ComputedStyle));
    m_computedStylePool->push_back(b);
}

void* StyleResolveContext::allocateComputedStyle()
{
    if (hasItemInComputedStylePool()) {
        return takeFromComputedStylePool();
    } else {
        return ComputedStyle::operator new(sizeof(ComputedStyle));
    }
}

void StyleResolver::applyAllProperty(Element* element,
                                     CSSStyleValuePair::ValueKind valueKind,
                                     ResourceURL* origin, ComputedStyle*& style,
                                     ComputedStyle* parentStyle,
                                     bool isImportant)
{
    STARFISH_ASSERT(element);
    STARFISH_ASSERT(origin);
    STARFISH_ASSERT(parentStyle);
    for (size_t i = CSSStyleValuePair::KeyKind::Unknown + 1;
         i < CSSStyleValuePair::KeyKind::KeyKindSize; i++) {
        CSSStyleValuePair::KeyKind keyKind =
            static_cast<CSSStyleValuePair::KeyKind>(i);
        if (keyKind != CSSStyleValuePair::KeyKind::All &&
            keyKind != CSSStyleValuePair::KeyKind::Direction &&
            keyKind != CSSStyleValuePair::KeyKind::UnicodeBidi &&
            keyKind != CSSStyleValuePair::KeyKind::CustomProperty) {
            CSSStyleValuePair p;
            p.setKeyKind(keyKind);
            p.setValueKind(valueKind);
            p.setFlagImportant(isImportant);
            applyProperty(element, p, origin, style, parentStyle, isImportant);
        }
    }
}

ComputedStyle* StyleResolver::resolveStyle(StyleResolveContext& ctx,
                                           Element* element,
                                           ComputedStyle* parent)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(parent != nullptr);

    ComputedStyle* style =
        new (ctx.allocateComputedStyle()) ComputedStyle(parent);

    matchAllRules(ctx, element, style, parent);
    style->loadResources(element, element->style());
    style->arrangeStyleValues(parent, element);
    return style;
}

static void applyTransitionProperty(Element* element, ComputedStyle* style,
                                    ComputedStyle* parentStyle,
                                    const CSSStyleValuePair& item, size_t layer)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setTransitionProperty(CSSStyleValuePair::KeyKind::All, layer);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        STARFISH_ASSERT(layer == 0);
        style->setTransitionProperty(parentStyle->transitionProperty());
        break;
    case CSSStyleValuePair::CSSPropertyNameValueKind:
        style->setTransitionProperty(item.cssPropertyNameValue(), layer);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: transition with %d",
                             item.valueKind());
    }
}

static void applyTransitionDuration(Element* element, ComputedStyle* style,
                                    ComputedStyle* parentStyle,
                                    const CSSStyleValuePair& item, size_t layer)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setTransitionDuration(CSSTime(0), layer);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        STARFISH_ASSERT(layer == 0);
        style->setTransitionDuration(parentStyle->transitionDuration());
        break;
    case CSSStyleValuePair::Time:
        style->setTransitionDuration(item.timeValue(), layer);
        break;
    case CSSStyleValuePair::CalcValueKind: {
        CalcData* calcData = item.calcValue();
        CalcValueType type = calcData->calcValueType();
        if (type.isTime()) {
            style->setTransitionDuration(calcData->timeValue(), layer);
        } else {
            style->setTransitionDuration(CSSTime(0), layer);
        }
        break;
    }
    default:
        STARFISH_UNSUPPORTED("css property: transition-duration with %d",
                             item.valueKind());
    }
}

static void applyTransitionTimingFunction(Element* element,
                                          ComputedStyle* style,
                                          ComputedStyle* parentStyle,
                                          const CSSStyleValuePair& item,
                                          size_t layer)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setTransitionTimingFunction(TimingFunctionEaseValue, layer);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        STARFISH_ASSERT(layer == 0);
        style->setTransitionTimingFunction(
            parentStyle->transitionTimingFunction());
        break;
    case CSSStyleValuePair::TimingFunctionValueKind:
        style->setTransitionTimingFunction(item.timingFunctionValue(), layer);
        break;
    case CSSStyleValuePair::TimingFunctionPointerKind:
        style->setTransitionTimingFunction(item.timingFunctionPointerValue(),
                                           layer);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: transition-timing-function with %d",
                             item.valueKind());
    }
}

static void applyTransitionDelay(Element* element, ComputedStyle* style,
                                 ComputedStyle* parentStyle,
                                 const CSSStyleValuePair& item, size_t layer)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setTransitionDelay(CSSTime(0), layer);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        STARFISH_ASSERT(layer == 0);
        style->setTransitionDelay(parentStyle->transitionDelay());
        break;
    case CSSStyleValuePair::Time:
        style->setTransitionDelay(item.timeValue(), layer);
        break;
    case CSSStyleValuePair::CalcValueKind: {
        CalcData* calcData = item.calcValue();
        CalcValueType type = calcData->calcValueType();
        if (type.isTime()) {
            style->setTransitionDelay(calcData->timeValue(), layer);
        } else {
            style->setTransitionDelay(CSSTime(0), layer);
        }
        break;
    }
    default:
        STARFISH_UNSUPPORTED("css property: transition-delay with %d",
                             item.valueKind());
    }
}

static void applyAnimationName(Element* element, ComputedStyle* style,
                               ComputedStyle* parentStyle,
                               const CSSStyleValuePair& item, size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationName(String::fromUTF8("none"), index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationName(parentStyle->animationName(), index);
        break;
    case CSSStyleValuePair::StringValueKind:
        style->setAnimationName(item.stringValue(), index);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: animation-name with %d",
                             item.valueKind());
    }
}

static void applyAnimationDuration(Element* element, ComputedStyle* style,
                                   ComputedStyle* parentStyle,
                                   const CSSStyleValuePair& item, size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationDuration(CSSTime(0), index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setTransitionDuration(parentStyle->animationDuration(), index);
        break;
    case CSSStyleValuePair::Time:
        style->setAnimationDuration(item.timeValue(), index);
        break;
    case CSSStyleValuePair::CalcValueKind: {
        CalcData* calcData = item.calcValue();
        CalcValueType type = calcData->calcValueType();
        if (type.isTime() == true) {
            style->setAnimationDuration(calcData->timeValue(), index);
        } else {
            style->setAnimationDuration(CSSTime(0), index);
        }
        break;
    }
    default:
        STARFISH_UNSUPPORTED("css property: animation-duration with %d",
                             item.valueKind());
    }
}

static void applyAnimationDelay(Element* element, ComputedStyle* style,
                                ComputedStyle* parentStyle,
                                const CSSStyleValuePair& item, size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationDelay(CSSTime(0), index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationDelay(parentStyle->animationDelay(), index);
        break;
    case CSSStyleValuePair::Time:
        style->setAnimationDelay(item.timeValue(), index);
        break;
    case CSSStyleValuePair::CalcValueKind: {
        CalcData* calcData = item.calcValue();
        CalcValueType type = calcData->calcValueType();
        if (type.isTime() == true) {
            style->setAnimationDelay(calcData->timeValue(), index);
        } else {
            style->setAnimationDelay(CSSTime(0), index);
        }
        break;
    }
    default:
        STARFISH_UNSUPPORTED("css property: animation-delay with %d",
                             item.valueKind());
    }
}

static void applyAnimationTimingFunction(Element* element, ComputedStyle* style,
                                         ComputedStyle* parentStyle,
                                         const CSSStyleValuePair& item,
                                         size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationTimingFunction(TimingFunctionEaseValue, index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationTimingFunction(
            parentStyle->animationTimingFunction(), index);
        break;
    case CSSStyleValuePair::TimingFunctionValueKind:
        style->setAnimationTimingFunction(item.timingFunctionValue(), index);
        break;
    case CSSStyleValuePair::TimingFunctionPointerKind:
        style->setAnimationTimingFunction(item.timingFunctionPointerValue(),
                                          index);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: animation-timing-function with %d",
                             item.valueKind());
    }
}

static void applyAnimationIterationCount(Element* element, ComputedStyle* style,
                                         ComputedStyle* parentStyle,
                                         const CSSStyleValuePair& item,
                                         size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationIterationCount(1.0, index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationIterationCount(
            parentStyle->animationIterationCount(), index);
        break;
    case CSSStyleValuePair::Number:
        style->setAnimationIterationCount(item.numberValue(), index);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: animation-iteration-count with %d",
                             item.valueKind());
    }
}

static void applyAnimationDirection(Element* element, ComputedStyle* style,
                                    ComputedStyle* parentStyle,
                                    const CSSStyleValuePair& item, size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationDirection(AnimationDirectionValue::Normal, index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationDirection(parentStyle->animationDirect(), index);
        break;
    case CSSStyleValuePair::AnimationDirectionValueKind:
        style->setAnimationDirection(item.animationDirectionValue(), index);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: animation-direction with %d",
                             item.valueKind());
    }
}

static void applyAnimationPlayState(Element* element, ComputedStyle* style,
                                    ComputedStyle* parentStyle,
                                    const CSSStyleValuePair& item, size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationPlayState(AnimationPlayStateValue::Running, index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationPlayState(parentStyle->animationPlayState(), index);
        break;
    case CSSStyleValuePair::AnimationPlayStateValueKind:
        style->setAnimationPlayState(item.animationPlayStateValue(), index);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: animation-play-state with %d",
                             item.valueKind());
    }
}

static void applyAnimationFillMode(Element* element, ComputedStyle* style,
                                   ComputedStyle* parentStyle,
                                   const CSSStyleValuePair& item, size_t index)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(parentStyle != nullptr);

    switch (item.valueKind()) {
    case CSSStyleValuePair::Initial:
    case CSSStyleValuePair::Unset:
        style->setAnimationFillMode(AnimationFillModeValue::None, index);
        break;
    case CSSStyleValuePair::Inherit:
        element->parentNode()
            ->style()
            ->markSomeNonInheritMemberExplicitlyInherited();
        style->setAnimationFillMode(parentStyle->animationFillMode(), index);
        break;
    case CSSStyleValuePair::AnimationFillModeValueKind:
        style->setAnimationFillMode(item.animationFillModeValue(), index);
        break;
    default:
        STARFISH_UNSUPPORTED("css property: animation-fill-mode with %d",
                             item.valueKind());
    }
}

typedef VectorWithInlineStorage<32, char, std::allocator<char>> Token;
typedef VectorWithInlineStorage<4, Token, std::allocator<Token>> TokenVector;

static void tokenize(TokenVector& tokens, const char* data, size_t length)
{
    Token str;
    bool inParenthesis = false;
    size_t numberOfnesting = 0;
    bool inQuotes = false;
    bool isWhiteSpaceState = false;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == '(' && !inQuotes) {
            inParenthesis = true;
            numberOfnesting++;
        } else if (data[i] == ')' && !inQuotes) {
            if (numberOfnesting) {
                numberOfnesting--;
            }
        } else if (data[i] == '"' || data[i] == '\'') {
            inQuotes = !inQuotes;
        }

        if (isWhiteSpaceState && String::isSpaceOrNewline(data[i])) {
            continue;
        }

        isWhiteSpaceState = false;
        str.push_back(data[i]);
        if ((inParenthesis || inQuotes) && String::isSpaceOrNewline(data[i])) {
            str[str.size() - 1] = ' ';
            isWhiteSpaceState = true;
            continue;
        }
        bool hasSepChar = false;
        if (!inParenthesis) {
            hasSepChar = data[i] == ',';
        }

        if ((!inParenthesis && !inQuotes &&
             (String::isSpaceOrNewline(data[i]) || hasSepChar)) ||
            (data[i] == '(' && hasSepChar) || (data[i] == ')' && hasSepChar)) {
            str.pop_back();
            bool onlyWhiteSpace = true;
            for (size_t i = 0; i < str.size(); i++) {
                if (!String::isASCIISpace(str[i])) {
                    onlyWhiteSpace = false;
                }
            }

            if (!onlyWhiteSpace && !numberOfnesting) {
                tokens.push_back(Token(std::move(str)));
            }
            isWhiteSpaceState = true;
            if (hasSepChar && !numberOfnesting) {
                Token n;
                n.push_back(data[i]);
                tokens.push_back(std::move(n));
            }
        } else if (((inParenthesis && !numberOfnesting) && data[i] == ')') ||
                   i == length - 1) {
            if (str.size() != 0) {
                if (!numberOfnesting) {
                    tokens.push_back(std::move(str));
                }
            }
            inParenthesis = false;
            isWhiteSpaceState = true;
        }
    }
}

CSSTokenValue StyleResolver::resolveVarReferencedValue(
    Node* node, const CSSStyleValuePair& cssValuePair)
{
    OptionalUTF8String utf8String =
        cssValuePair.varFunctionValue()->toOptionalUTF8String();
    size_t startIndex = 0;
    size_t size = utf8String.m_bufferSize;
    if (cssValuePair.temporaryValueKind() ==
        CSSStyleValuePair::ValueKind::CalcValueKind) {
        // This is the length of "calc(".
        startIndex = 5;
        // consider ')' too
        size -= 6;
    }
    // we should keep reference of utf8String.m_buffer
    // for bdwgc find the pointer of `utf8String.m_buffer`
    volatile const char* forceKeepPointer = utf8String.m_buffer;
    utf8String.m_buffer = utf8String.m_buffer + startIndex;
    utf8String.m_bufferSize = size;
    CSSTokenValue newCssValue =
        resolveVarReferencedValue(node, utf8String, cssCustomValues());

    if (cssValuePair.temporaryValueKind() ==
        CSSStyleValuePair::ValueKind::CalcValueKind) {
        newCssValue = "calc(" + newCssValue + ")";
    }
    return newCssValue;
}

CSSTokenValue StyleResolver::resolveVarReferencedValue(
    Node* node, OptionalUTF8String utf8String,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    CSSTokenValue newCssValue;
    TokenVector cssValueTokens;
    tokenize(cssValueTokens, utf8String.m_buffer, utf8String.m_bufferSize);

    for (size_t i = 0; i < cssValueTokens.size(); ++i) {
        CSSVariableSyntaxTreeBuilder variablesSyntaxBuilder(node->starfish());
        variablesSyntaxBuilder.build(cssValueTokens[i].data(),
                                     cssValueTokens[i].size());
        if (variablesSyntaxBuilder.isValid()) {
            auto styleValue =
                variablesSyntaxBuilder.generateStyle(node, cssCustomValues);
            TokenVector tempCssValueTokens;
            tokenize(tempCssValueTokens, styleValue.data(),
                     styleValue.length());

            for (size_t j = 0; j < tempCssValueTokens.size(); ++j) {
                newCssValue.append(tempCssValueTokens[j].data(),
                                   tempCssValueTokens[j].data() +
                                       tempCssValueTokens[j].size());
                newCssValue.append(" ");
            }
        } else {
            bool isSuccess = false;
            std::string newCssValueCandidate;
            {
                int start = 0, end = 0, previous_end = 0;
                CSSTokenValue currentToken(cssValueTokens[i].data(),
                                           cssValueTokens[i].size());
                while (
                    needsToFindVarFunction(currentToken, start, &start, &end)) {
                    if (newCssValueCandidate.length() == 0) {
                        newCssValueCandidate.append(
                            currentToken.substring(0, start));
                    } else if (start != 0 && previous_end + 1 != start) {
                        newCssValueCandidate.append(currentToken.substring(
                            previous_end + 1, start - previous_end - 1));
                    }
                    const CSSTokenValue& nextToken =
                        currentToken.substring(start, end - start + 1);
                    variablesSyntaxBuilder.reset();
                    variablesSyntaxBuilder.build(nextToken.data(),
                                                 nextToken.size());
                    if (variablesSyntaxBuilder.isValid()) {
                        auto styleValue = variablesSyntaxBuilder.generateStyle(
                            node, cssCustomValues);
                        TokenVector tempCssValueTokens;
                        tokenize(tempCssValueTokens, styleValue.data(),
                                 styleValue.length());

                        for (size_t j = 0; j < tempCssValueTokens.size(); ++j) {
                            newCssValueCandidate.append(
                                tempCssValueTokens[j].data(),
                                tempCssValueTokens[j].data() +
                                    tempCssValueTokens[j].size());
                            newCssValueCandidate.append(" ");
                        }
                        isSuccess = true;
                    } else {
                        isSuccess = false;
                        break;
                    }
                    start = end + 1;
                    previous_end = end;
                }
                newCssValueCandidate.append(currentToken.substring(
                    start, currentToken.length() - start + 1));
            }
            if (isSuccess) {
                newCssValue.append(newCssValueCandidate);
            } else {
                newCssValue.append(cssValueTokens[i].data(),
                                   cssValueTokens[i].data() +
                                       cssValueTokens[i].size());
                newCssValue.append(" ");
            }
        }
    }

    return newCssValue.trim();
}

void StyleResolver::clearCssCustomValues()
{
    m_cssCustomValues = Optional<MutablePropertyValueList*>();
}

Optional<const MutablePropertyValueList*> StyleResolver::cssCustomValues()
{
    if (m_cssCustomValues.hasValue()) {
        return Optional<const MutablePropertyValueList*>(
            m_cssCustomValues.value());
    }
    return Optional<const MutablePropertyValueList*>();
}

CSSStyleDeclaration* StyleResolver::resolveVarValue(
    Element* element, const CSSStyleValuePair& cssValuePair,
    CSSStyleValuePair::KeyKind keyKind, bool isImportant)
{
    auto newCssValue = resolveVarReferencedValue(element, cssValuePair);

    CSSStyleDeclaration* declaration = new CSSStyleDeclaration(document());
    declaration->setPropertyInternal(keyKind, newCssValue.c_str(),
                                     newCssValue.size(), isImportant);
    const GCAtomicVector<CSSStyleValuePair>& cssValues =
        declaration->cssValues();
    if (cssValues.size() == 1 &&
        cssValues[0].valueKind() ==
            CSSStyleValuePair::ValueKind::VarFunctionValueKind) {
        STARFISH_ASSERT(declaration->cssValues().size() == 1);
        CSSStyleValuePair newCssValuePair = declaration->cssValues()[0];
        if (newCssValue.find("calc(") == 0) {
            newCssValuePair.setTemporaryValueKind(
                CSSStyleValuePair::ValueKind::CalcValueKind);
        }
        return resolveVarValue(element, newCssValuePair,
                               newCssValuePair.keyKind(),
                               newCssValuePair.flagImportant());
    }
#ifndef NDEBUG
    for (auto& pair : cssValues) {
        STARFISH_ASSERT(pair.valueKind() !=
                        CSSStyleValuePair::ValueKind::VarFunctionValueKind);
    }
#endif
    return declaration;
}

void StyleResolver::apply(Element* element,
                          const GCAtomicVector<CSSStyleValuePair>& cssValues,
                          ResourceURL* origin, ComputedStyle* style,
                          ComputedStyle* parentStyle, bool isImportant)
{
    STARFISH_ASSERT(element);
    STARFISH_ASSERT(origin);
    STARFISH_ASSERT(style);
    STARFISH_ASSERT(parentStyle);

    for (const auto& cssValue : cssValues) {
        if (isImportant != cssValue.flagImportant()) {
            continue;
        }

        if (cssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::VarFunctionValueKind) {
            CSSStyleDeclaration* resolvedDeclaration =
                resolveVarValue(element, cssValue, cssValue.keyKind(),
                                cssValue.flagImportant());
            for (const auto& resolvedCssValues :
                 resolvedDeclaration->cssValues()) {
                applyProperty(element, resolvedCssValues, origin, style,
                              parentStyle, isImportant);
            }
        } else {
            applyProperty(element, cssValue, origin, style, parentStyle,
                          isImportant);
        }
    }
}

void StyleResolver::applyProperty(Element* element,
                                  const CSSStyleValuePair& newCssValue,
                                  ResourceURL* origin, ComputedStyle* style,
                                  ComputedStyle* parentStyle, bool isImportant)
{
    STARFISH_ASSERT(element);
    STARFISH_ASSERT(origin);
    STARFISH_ASSERT(style);
    STARFISH_ASSERT(parentStyle);

#define MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED()                \
    if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) { \
        element->parentNode()                                               \
            ->style()                                                       \
            ->markSomeNonInheritMemberExplicitlyInherited();                \
    }

    switch (newCssValue.keyKind()) {
    case CSSStyleValuePair::KeyKind::Display:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_display = parentStyle->m_display;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_display = DisplayValue::InlineDisplayValue;
        } else {
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::DisplayValueKind ==
                            newCssValue.valueKind());
            style->m_display = newCssValue.displayValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::Position:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_position = parentStyle->m_position;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_position = PositionValue::StaticPositionValue;
        } else {
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::PositionValueKind ==
                            newCssValue.valueKind());
            style->m_position = newCssValue.positionValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::All:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();

            applyAllProperty(element, newCssValue.valueKind(), origin, style,
                             parentStyle, isImportant);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::Float:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_float = parentStyle->m_float;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_float = FloatValue::NoneFloatValue;
        } else {
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::FloatValueKind ==
                            newCssValue.valueKind());
            style->m_float = newCssValue.floatValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::Clear:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_clear = parentStyle->m_clear;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_clear = ClearValue::NoneClearValue;
        } else {
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::ClearValueKind ==
                            newCssValue.valueKind());
            style->m_clear = newCssValue.clearValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::Width:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setWidth(parentStyle->width());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setWidth(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind) {
            switch (newCssValue.widthHeightKeywordValue()) {
            case WidthHeightKeywordValue::FitContentValue:
                style->setWidth(Length(Length::Type::FitContent));
                break;
            default:
                break;
            }
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setWidth(length.getValue());
            } else {
                style->setWidth(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MaxWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMaxWidth(parentStyle->maxWidth());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMaxWidth(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::None) {
            style->setMaxWidth(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind) {
            // TODO: max-width can have the following keyword values
            // stretch | fit-content | contain
            STARFISH_UNIMPLEMENTED("MinWidth: css keyword value kind.");
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMaxWidth(length.getValue());
            } else {
                style->setMaxWidth(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MinWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMinWidth(parentStyle->minWidth());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMinWidth(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind) {
            STARFISH_UNIMPLEMENTED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMinWidth(length.getValue());
            } else {
                style->setMinWidth(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::Height:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setHeight(parentStyle->height());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setHeight(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind) {
            STARFISH_UNIMPLEMENTED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setHeight(length.getValue());
            } else {
                style->setHeight(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MaxHeight:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMaxHeight(parentStyle->maxHeight());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMaxHeight(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::None) {
            style->setMaxHeight(Length());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind) {
            STARFISH_UNIMPLEMENTED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMaxHeight(length.getValue());
            } else {
                style->setMaxHeight(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MinHeight:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMinHeight(parentStyle->minHeight());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMinHeight(Length());
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMinHeight(length.getValue());
            } else {
                style->setMinHeight(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::Color:
        style->m_gotInheritedColor = false;
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setColor(parentStyle->m_inheritedStyles.m_color);
            style->m_gotInheritedColor = true;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setColor(Unit::Color(0, 0, 0, 255));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setColor(newCssValue.colorValue());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->m_inheritedStyles.m_color =
                    parentStyle->m_inheritedStyles.m_color;
            } else {
                style->setColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::FontSize:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setFontSize(parentStyle->fontSize());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setFontSize(
                parseAbsoluteFontSize(3, this->m_mediumFontSize));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FontSizeValueKind) {
            if (newCssValue.fontSizeValue() ==
                FontSizeValue::XXSmallFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(0, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::XSmallFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(1, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::SmallFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(2, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::MediumFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(3, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::LargeFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(4, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::XLargeFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(5, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::XXLargeFontSizeValue) {
                style->setFontSize(
                    parseAbsoluteFontSize(6, this->m_mediumFontSize));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::XXXLargeFontSizeValue) {
                style->setFontSize(Length(
                    Length::Fixed,
                    parseAbsoluteFontSize(6, this->m_mediumFontSize).fixed() *
                        1.5f));
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::LargerFontSizeValue) {
                style->setFontSize(parentStyle->fontSize() * 1.2f);
            } else if (newCssValue.fontSizeValue() ==
                       FontSizeValue::SmallerFontSizeValue) {
                style->setFontSize(parentStyle->fontSize() / 1.2f);
            }
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                Length l = length.getValue();
                if (l.isViewportPercent()) {
                    style->m_seenViewPortUnitInStyle = true;
                }
                style->setFontSize(l);
            } else {
                style->setFontSize(
                    parseAbsoluteFontSize(3, this->m_mediumFontSize));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::FontStyle:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_fontStyle =
                parentStyle->m_inheritedStyles.m_fontStyle;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_fontStyle =
                FontStyleValue::NormalFontStyleValue;
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::FontStyleValueKind);
            style->m_inheritedStyles.m_fontStyle = newCssValue.fontStyleValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::FontFamily:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_fontFamilyDatas =
                parentStyle->m_inheritedStyles.m_fontFamilyDatas;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_fontFamilyDatas =
                element->document()->webView()->initialFontFamilyDatas();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::KeywordValueKind) {
            FontFamilyData* data =
                (FontFamilyData*)GC_MALLOC_ATOMIC(sizeof(FontFamilyData) * 2);
            data[0].m_length = 1;
            data[1].m_familyName = AtomicString::createAtomicString(
                element->starfish(), newCssValue.keywordValue());
            style->m_inheritedStyles.m_fontFamilyDatas = data;
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* val = newCssValue.multiValue();
            FontFamilyData* data = (FontFamilyData*)GC_MALLOC_ATOMIC(
                sizeof(FontFamilyData) * (val->size() + 1));
            data[0].m_length = val->size();
            for (size_t i = 0; i < val->size(); i++) {
                data[i + 1].m_familyName = AtomicString::createAtomicString(
                    element->starfish(), val->at(i).keywordValue());
            }
            style->m_inheritedStyles.m_fontFamilyDatas = data;
        }
        break;
    case CSSStyleValuePair::KeyKind::FontKerning:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setFontKerning(parentStyle->fontKerning());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setFontKerning(FontKerningValue::FontKerningAutoValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FontKerningValueKind) {
            if (newCssValue.fontKerningValue() ==
                FontKerningValue::FontKerningAutoValue) {
                style->setFontKerning(FontKerningValue::FontKerningAutoValue);
            } else if (newCssValue.fontKerningValue() ==
                       FontKerningValue::FontKerningNormalValue) {
                style->setFontKerning(FontKerningValue::FontKerningNormalValue);
            } else {
                style->setFontKerning(FontKerningValue::FontKerningNoneValue);
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FontWeight:
        // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500
        // | 600 | 700 | 800 | 900 | inherit // initial -> normal
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_fontWeight =
                parentStyle->m_inheritedStyles.m_fontWeight;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_fontWeight =
                FontWeightValue::NormalFontWeightValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FontWeightValueKind) {
            if (newCssValue.fontWeightValue() ==
                FontWeightValue::FourHundredsFontWeightValue) {
                style->m_inheritedStyles.m_fontWeight =
                    FontWeightValue::NormalFontWeightValue;
            } else if (newCssValue.fontWeightValue() ==
                       FontWeightValue::SevenHundredsFontWeightValue) {
                style->m_inheritedStyles.m_fontWeight =
                    FontWeightValue::BoldFontWeightValue;
            } else if (newCssValue.fontWeightValue() ==
                       FontWeightValue::BolderFontWeightValue) {
                style->m_inheritedStyles.m_fontWeight =
                    bolderWeight(parentStyle->m_inheritedStyles.m_fontWeight);
            } else if (newCssValue.fontWeightValue() ==
                       FontWeightValue::LighterFontWeightValue) {
                style->m_inheritedStyles.m_fontWeight =
                    lighterWeight(parentStyle->m_inheritedStyles.m_fontWeight);
            } else {
                style->m_inheritedStyles.m_fontWeight =
                    newCssValue.fontWeightValue();
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::WordWrap:
    case CSSStyleValuePair::KeyKind::OverflowWrap:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_wordWrap =
                parentStyle->m_inheritedStyles.m_wordWrap;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_wordWrap =
                WordWrapValue::NormalWordWrapValue;
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::WordWrapValueKind);
            style->m_inheritedStyles.m_wordWrap = newCssValue.wordWrapValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::VerticalAlign:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setVerticalAlign(parentStyle->verticalAlign());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            if (style->isNumericVerticalAlign())
                style->setVerticalAlignLength(
                    parentStyle->verticalAlignLength());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setVerticalAlign(VerticalAlignValue::BaselineVAlignValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::VerticalAlignValueKind) {
            STARFISH_ASSERT(newCssValue.verticalAlignValue() !=
                            VerticalAlignValue::NumericVAlignValue);
            style->setVerticalAlign(newCssValue.verticalAlignValue());
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setVerticalAlignLength(length.getValue());
            } else {
                style->setVerticalAlign(
                    VerticalAlignValue::BaselineVAlignValue);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::ImageRendering:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setImageRendering(parentStyle->imageRendering());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setImageRendering(
                ImageRenderingValue::ImageRenderingAutoValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ImageRenderingValueKind) {
            if (newCssValue.imageRenderingValue() ==
                ImageRenderingValue::ImageRenderingAutoValue) {
                style->setImageRendering(
                    ImageRenderingValue::ImageRenderingAutoValue);
            } else if (newCssValue.imageRenderingValue() ==
                       ImageRenderingValue::ImageRenderingCrispEdgesValue) {
                style->setImageRendering(
                    ImageRenderingValue::ImageRenderingCrispEdgesValue);
            } else {
                style->setImageRendering(
                    ImageRenderingValue::ImageRenderingPixelatedValue);
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::TableLayout:
        // auto | fixed | initial | inherit
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            style->m_tableLayout = parentStyle->m_tableLayout;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setTableLayout(TableLayoutValue::AutoTableLayoutValue);
            break;
        default:
            STARFISH_ASSERT(
                CSSStyleValuePair::ValueKind::TableLayoutValueKind ==
                newCssValue.valueKind());
            style->setTableLayout(newCssValue.tableLayoutValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TextAlign:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setTextAlign(parentStyle->textAlign());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setTextAlign(TextAlignValue::StartTextAlignValue);
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::TextAlignValueKind);
            style->setTextAlign(newCssValue.textAlignValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TextIndent:
        // length | percentage | inherit
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setTextIndent(parentStyle->textIndent());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setTextIndent(Length(Length::Fixed, 0));
        } else {
            Optional<Length> len = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (len.hasValue()) {
                style->setTextIndent(len.getValue());
            } else {
                style->setTextIndent(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::TextOverflow:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit)) {
            style->setTextOverflow(parentStyle->textOverflow());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setTextOverflow(TextOverflowData());
        } else {
            style->setTextOverflow(newCssValue.textOverflowValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TextDecorationLine:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setTextDecorationLine(parentStyle->textDecorationLine());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            CSSStyleValuePair p;
            p.setValueKind(CSSStyleValuePair::TextDecorationLineValueKind);
            p.setValue(NoneTextDecorationLineValue);
            style->ensureTextDecorationLine()->push_back(p);
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            style->setTextDecorationLine(newCssValue.multiValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TextDecorationColor:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            style->setTextDecorationColor(parentStyle->textDecorationColor());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setTextDecorationColor(Unit::Color(0, 0, 0, 255));
            break;
        case CSSStyleValuePair::ValueKind::ColorValueKind:
            style->setTextDecorationColor(newCssValue.colorValue());
            break;
        default:
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->setTextDecorationColor(
                    parentStyle->textDecorationColor());
            } else {
                style->setTextDecorationColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::TextDecorationStyle:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            style->setTextDecorationStyle(parentStyle->textDecorationStyle());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setTextDecorationStyle(
                TextDecorationStyleValue::SolidTextDecorationStyleValue);
            break;
        default:
            STARFISH_ASSERT(
                newCssValue.valueKind() ==
                CSSStyleValuePair::ValueKind::TextDecorationStyleValueKind);
            style->setTextDecorationStyle(
                newCssValue.textDecorationStyleValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TextUnderlinePosition:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setTextUnderlinePosition(
                parentStyle->textUnderlinePosition());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->setTextUnderlinePosition(
                TextUnderlinePositionValue::AutoTextUnderlinePositionValue);
            break;
        default:
            STARFISH_ASSERT(
                newCssValue.valueKind() ==
                CSSStyleValuePair::ValueKind::TextUnderlinePositionValueKind);
            style->setTextUnderlinePosition(
                newCssValue.textUnderlinePositionValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::Resize:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            style->setResize(parentStyle->resize());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setResize(ResizeValue::NoneResizeValue);
            break;
        default:
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ResizeValueKind);
            style->setResize(newCssValue.resizeValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TextShadow: {
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            if (parentStyle->textShadow()) {
                style->setTextShadow(*parentStyle->textShadow());
            } else {
                style->setTextShadow(ShadowDataList());
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::None)) {
            style->setTextShadow(ShadowDataList());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* shadows = newCssValue.multiValue();
            style->setTextShadow(ShadowDataList());

            for (size_t i = 0; i < shadows->size(); i++) {
                STARFISH_ASSERT((*shadows)[i].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);

                ValueList* shadow = (*shadows)[i].multiValue();
                ShadowData sd;
                for (size_t j = 0; j < shadow->size(); j++) {
                    switch ((*shadow)[j].valueKind()) {
                    case CSSStyleValuePair::ValueKind::ValueListKind: {
                        sd.setLengths((*shadow)[j].multiValue());
                    } break;
                    case CSSStyleValuePair::ValueKind::NamedColorValueKind: {
                        Unit::Color color = NamedColor::namedColorToColor(
                            (*shadow)[j].namedColorValue());
                        sd.setColor(color);
                    } break;
                    case CSSStyleValuePair::ValueKind::ColorValueKind: {
                        Unit::Color color = (*shadow)[j].colorValue();
                        sd.setColor(color);
                    } break;
                    case CSSStyleValuePair::ValueKind::KeywordValueKind: {
                        sd.setInset();
                    } break;
                    default:
                        STARFISH_ASSERT_NOT_REACHED();
                        break;
                    }
                }
                style->addTextShadow(sd);
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::BoxShadow: {
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setBoxShadow(ShadowDataList());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            if (parentStyle->boxShadow()) {
                style->setBoxShadow(*parentStyle->boxShadow());
            } else {
                style->setBoxShadow(ShadowDataList());
            }
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* shadows = newCssValue.multiValue();
            style->setBoxShadow(ShadowDataList());

            for (size_t i = 0; i < shadows->size(); i++) {
                STARFISH_ASSERT((*shadows)[i].valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);

                ValueList* shadow = (*shadows)[i].multiValue();
                ShadowData sd;
                for (size_t j = 0; j < shadow->size(); j++) {
                    switch ((*shadow)[j].valueKind()) {
                    case CSSStyleValuePair::ValueKind::ValueListKind: {
                        sd.setLengths((*shadow)[j].multiValue());
                    } break;
                    case CSSStyleValuePair::ValueKind::NamedColorValueKind: {
                        Unit::Color color = NamedColor::namedColorToColor(
                            (*shadow)[j].namedColorValue());
                        sd.setColor(color);
                    } break;
                    case CSSStyleValuePair::ValueKind::ColorValueKind: {
                        Unit::Color color = (*shadow)[j].colorValue();
                        sd.setColor(color);
                    } break;
                    case CSSStyleValuePair::ValueKind::KeywordValueKind: {
                        sd.setInset();
                    } break;
                    default:
                        STARFISH_ASSERT_NOT_REACHED();
                        break;
                    }
                }
                style->addBoxShadow(sd);
            }
        } else {
            style->setBoxShadow(ShadowDataList());
        }
    } break;
    case CSSStyleValuePair::KeyKind::PointerEvents:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setPointerEvents(parentStyle->pointerEvents());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setPointerEvents(PointerEventsAutoValue);
        } else {
            style->setPointerEvents(newCssValue.pointerEventsValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::MixBlendMode:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setMixBlendMode(parentStyle->mixBlendMode());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setMixBlendMode(BlendMode::Normal);
        } else {
            style->setMixBlendMode(newCssValue.blendModeValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::Direction:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_direction =
                parentStyle->m_inheritedStyles.m_direction;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_direction =
                DirectionValue::LtrDirectionValue;
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::DirectionValueKind);
            style->m_inheritedStyles.m_direction = newCssValue.directionValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::WhiteSpace:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_whiteSpace =
                parentStyle->m_inheritedStyles.m_whiteSpace;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_whiteSpace =
                WhiteSpaceValue::NormalWhiteSpaceValue;
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::WhiteSpaceValueKind);
            style->m_inheritedStyles.m_whiteSpace =
                newCssValue.whiteSpaceValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::WordSpacing:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setWordSpacing(parentStyle->wordSpacing());
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Normal) {
            style->setWordSpacing(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Length ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::CalcValueKind) {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setWordSpacing(length.getValue());
            } else {
                style->setWordSpacing(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::LetterSpacing:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setLetterSpacing(parentStyle->wordSpacing());
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Normal) {
            style->setLetterSpacing(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Length ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::CalcValueKind) {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setLetterSpacing(length.getValue());
            } else {
                style->setLetterSpacing(Length(Length::Fixed, 0));
            }
        }
        break;

    case CSSStyleValuePair::KeyKind::BackgroundColor:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setBackgroundColor(parentStyle->backgroundColor());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBackgroundColor(Unit::Color(0, 0, 0, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setBackgroundColor(newCssValue.colorValue());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                // currentColor : represents the calculated value of the
                // element's color property
                // --> change to valid value when arrangeStyleValues()
                style->setBackgroundColorToCurrentColor();
            } else {
                style->setBackgroundColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundImage:
        style->resetBackgroundImages();
        // NOTE Do nothing for Initial, Unset, None
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundImage(parentStyle->backgroundImage(i), i);
            }
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                auto vKind = item.valueKind();
                if (vKind == CSSStyleValuePair::ValueKind::UrlValueKind) {
                    style->setBackgroundImage(
                        new ImageValue(item.urlValue(origin)), i);
                } else if (vKind ==
                           CSSStyleValuePair::ValueKind::GradientValueKind) {
                    style->setBackgroundImage(
                        new ImageValue(
                            item.gradientValue()->convertToGradientData()),
                        i);
                } else {
                    // NOTE Do nothing for Initial, None
                    STARFISH_RELEASE_ASSERT(
                        vKind == CSSStyleValuePair::ValueKind::Initial ||
                        vKind == CSSStyleValuePair::ValueKind::None);
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundPositionX:
        style->resetBackgroundPositionXs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundPositionX(
                    parentStyle->backgroundPositionX(i), i);
            }
        } else {
            setComputedStyleUnitPositionX(
                style, newCssValue, 0,
                [](ComputedStyle* style, const Length& length, uint32_t layer) {
                    style->setBackgroundPositionX(length, layer);
                });
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundPositionY:
        style->resetBackgroundPositionYs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundPositionY(
                    parentStyle->backgroundPositionY(i), i);
            }
        } else {
            setComputedStyleUnitPositionY(
                style, newCssValue, 0,
                [](ComputedStyle* style, const Length& length, uint32_t layer) {
                    style->setBackgroundPositionY(length, layer);
                });
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundSize:
        style->resetBackgroundSizes();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                if (parentStyle->backgroundSizeIsLength(i)) {
                    style->setBackgroundSize(
                        parentStyle->backgroundSizeLengthValue(i), i);
                } else {
                    style->setBackgroundSize(
                        parentStyle->backgroundSizeTypeValue(i), i);
                }
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBackgroundSize(LengthSize(), 0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* layers = newCssValue.multiValue();
            for (unsigned int l = 0; l < layers->size(); l++) {
                const CSSStyleValuePair& layer = (*layers)[l];
                if (layer.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundSize(LengthSize(), l);
                } else if (layer.valueKind() == CSSStyleValuePair::ValueKind::
                                                    BackgroundSizeValueKind) {
                    style->setBackgroundSize(layer.backgroundSizeValue(), l);
                } else if (layer.valueKind() ==
                           CSSStyleValuePair::ValueKind::Auto) {
                    style->setBackgroundSize(LengthSize(), l);
                } else if (layer.valueKind() ==
                           CSSStyleValuePair::ValueListKind) {
                    ValueList* list = layer.multiValue();
                    LengthSize result;
                    if (list->size() >= 1) {
                        Optional<Length> width =
                            LengthUtil::convertValueToLength(
                                (*list)[0].valueKind(), (*list)[0].value());
                        if (width.hasValue()) {
                            result.m_width = width.getValue();
                        }
                    }
                    if (list->size() >= 2) {
                        Optional<Length> height =
                            LengthUtil::convertValueToLength(
                                (*list)[1].valueKind(), (*list)[1].value());
                        if (height.hasValue()) {
                            result.m_height = height.getValue();
                        }
                    }
                    style->setBackgroundSize(result, l);
                } else {
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundRepeatX:
        style->resetBackgroundRepeatXs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundRepeatX(parentStyle->backgroundRepeatX(i),
                                            i);
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBackgroundRepeatX(RepeatStyleValue::RepeatRepeatValue, 0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
            style->setBackgroundRepeatX(newCssValue.repeatStyleValue(), 0);
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundRepeatX(
                        RepeatStyleValue::RepeatRepeatValue, i);
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
                    style->setBackgroundRepeatX(item.repeatStyleValue(), i);
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundRepeatY:
        style->resetBackgroundRepeatYs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundRepeatY(parentStyle->backgroundRepeatY(i),
                                            i);
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBackgroundRepeatY(RepeatStyleValue::RepeatRepeatValue, 0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
            style->setBackgroundRepeatY(newCssValue.repeatStyleValue(), 0);
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundRepeatY(
                        RepeatStyleValue::RepeatRepeatValue, i);
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
                    style->setBackgroundRepeatY(item.repeatStyleValue(), i);
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundAttachment:
        style->resetBackgroundAttachments();
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setBackgroundAttachment(
                BackgroundAttachmentValue::ScrollBackgroundAttachmentValue, 0);
            break;
        case CSSStyleValuePair::ValueKind::Inherit: {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundAttachment(
                    parentStyle->backgroundAttachment(i), i);
            }
        } break;
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundAttachment(
                        BackgroundAttachmentValue::
                            ScrollBackgroundAttachmentValue,
                        i);
                } else {
                    STARFISH_ASSERT(item.valueKind() ==
                                    CSSStyleValuePair::ValueKind::
                                        BackgroundAttachmentValueKind);
                    style->setBackgroundAttachment(
                        item.backgroundAttachmentValue(), i);
                }
            }
            break;
        }
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundClip:
        style->resetBackgroundClips();
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setBackgroundClip(BoxValue::BorderBoxBoxValue, 0);
            break;
        case CSSStyleValuePair::ValueKind::Inherit: {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundClip(parentStyle->backgroundClip(i), i);
            }
        } break;
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundClip(BoxValue::BorderBoxBoxValue, i);
                } else {
                    STARFISH_ASSERT(item.valueKind() ==
                                    CSSStyleValuePair::ValueKind::BoxValueKind);
                    style->setBackgroundClip(item.boxValue(), i);
                }
            }
            break;
        }
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BackgroundOrigin:
        style->resetBackgroundOrigins();
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setBackgroundOrigin(BoxValue::PaddingBoxBoxValue, 0);
            break;
        case CSSStyleValuePair::ValueKind::Inherit: {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->backgroundLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setBackgroundOrigin(parentStyle->backgroundOrigin(i), i);
            }
        } break;
        case CSSStyleValuePair::ValueKind::ValueListKind: {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setBackgroundOrigin(BoxValue::PaddingBoxBoxValue, i);
                } else {
                    STARFISH_ASSERT(item.valueKind() ==
                                    CSSStyleValuePair::ValueKind::BoxValueKind);
                    style->setBackgroundOrigin(item.boxValue(), i);
                }
            }
            break;
        }
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::ColumnGap: {
        CSSStyleValuePair::ValueKind valueKind = newCssValue.valueKind();
        if (valueKind == CSSStyleValuePair::ValueKind::Inherit) {
            style->setColumnGap(parentStyle->columnGap());
        } else if (valueKind == CSSStyleValuePair::ValueKind::Initial ||
                   valueKind == CSSStyleValuePair::ValueKind::Normal) {
            style->setColumnGap(Length(Length::Fixed, 0));
        } else if (valueKind == CSSStyleValuePair::ValueKind::Length ||
                   valueKind == CSSStyleValuePair::ValueKind::Percentage ||
                   valueKind == CSSStyleValuePair::ValueKind::CalcValueKind) {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setColumnGap(length.getValue());
            } else {
                style->setColumnGap(Length(Length::Fixed, 0));
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::RowGap: {
        CSSStyleValuePair::ValueKind valueKind = newCssValue.valueKind();
        if (valueKind == CSSStyleValuePair::ValueKind::Inherit) {
            style->setRowGap(parentStyle->rowGap());
        } else if (valueKind == CSSStyleValuePair::ValueKind::Initial ||
                   valueKind == CSSStyleValuePair::ValueKind::Normal) {
            style->setRowGap(Length(Length::Fixed, 0));
        } else if (valueKind == CSSStyleValuePair::ValueKind::Length ||
                   valueKind == CSSStyleValuePair::ValueKind::Percentage ||
                   valueKind == CSSStyleValuePair::ValueKind::CalcValueKind) {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setRowGap(length.getValue());
            } else {
                style->setRowGap(Length(Length::Fixed, 0));
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::MaskImage:
        // NOTE Do nothing for Initial, Unset, None, Inherit
        style->resetMaskImage();
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                auto vKind = item.valueKind();
                ImageValue* imageValue = nullptr;
                if (vKind == CSSStyleValuePair::ValueKind::UrlValueKind) {
                    imageValue = new ImageValue(item.urlValue(origin));
                    style->setMaskImage(imageValue, i);
                } else if (vKind ==
                           CSSStyleValuePair::ValueKind::GradientValueKind) {
                    imageValue = new ImageValue(
                        item.gradientValue()->convertToGradientData());
                    style->setMaskImage(imageValue, i);
                } else {
                    STARFISH_UNSUPPORTED("css property: mask-image with %d",
                                         item.valueKind());
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MaskSize:
        style->resetMaskSizes();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->maskLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                if (parentStyle->maskSizeIsLength(i)) {
                    style->setMaskSize(parentStyle->maskSizeLengthValue(i), i);
                } else {
                    style->setMaskSize(parentStyle->maskSizeTypeValue(i), i);
                }
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMaskSize(LengthSize(), 0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* layers = newCssValue.multiValue();
            for (unsigned int l = 0; l < layers->size(); l++) {
                const CSSStyleValuePair& layer = (*layers)[l];
                if (layer.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) {
                    style->setMaskSize(LengthSize(), l);
                } else if (layer.valueKind() == CSSStyleValuePair::ValueKind::
                                                    BackgroundSizeValueKind) {
                    style->setMaskSize(layer.backgroundSizeValue(), l);
                } else if (layer.valueKind() ==
                           CSSStyleValuePair::ValueKind::Auto) {
                    style->setMaskSize(LengthSize(), l);
                } else if (layer.valueKind() ==
                           CSSStyleValuePair::ValueListKind) {
                    ValueList* list = layer.multiValue();
                    LengthSize result;
                    if (list->size() >= 1) {
                        Optional<Length> width =
                            LengthUtil::convertValueToLength(
                                (*list)[0].valueKind(), (*list)[0].value());
                        if (width.hasValue()) {
                            result.m_width = width.getValue();
                        }
                    }
                    if (list->size() >= 2) {
                        Optional<Length> height =
                            LengthUtil::convertValueToLength(
                                (*list)[1].valueKind(), (*list)[1].value());
                        if (height.hasValue()) {
                            result.m_height = height.getValue();
                        }
                    }
                    style->setMaskSize(result, l);
                } else {
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::MaskPositionX:
        style->resetMaskPositionXs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->maskLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setMaskPositionX(parentStyle->maskPositionX(i), i);
            }
        } else {
            setComputedStyleUnitPositionX(
                style, newCssValue, 0,
                [](ComputedStyle* style, const Length& length, uint32_t layer) {
                    style->setMaskPositionX(length, layer);
                });
        }
        break;
    case CSSStyleValuePair::KeyKind::MaskPositionY:
        style->resetMaskPositionYs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->maskLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setMaskPositionY(parentStyle->maskPositionY(i), i);
            }
        } else {
            setComputedStyleUnitPositionY(
                style, newCssValue, 0,
                [](ComputedStyle* style, const Length& length, uint32_t layer) {
                    style->setMaskPositionY(length, layer);
                });
        }
        break;
    case CSSStyleValuePair::KeyKind::MaskRepeatX:
        style->resetMaskRepeatXs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->maskLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setMaskRepeatX(parentStyle->maskRepeatX(i), i);
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMaskRepeatX(RepeatStyleValue::RepeatRepeatValue, 0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
            style->setMaskRepeatX(newCssValue.repeatStyleValue(), 0);
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setMaskRepeatX(RepeatStyleValue::RepeatRepeatValue,
                                          i);
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
                    style->setMaskRepeatX(item.repeatStyleValue(), i);
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MaskRepeatY:
        style->resetMaskRepeatYs();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            uint32_t size = parentStyle->maskLayerSize();
            for (uint32_t i = 0; i < size; i++) {
                style->setMaskRepeatY(parentStyle->maskRepeatY(i), i);
            }
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMaskRepeatY(RepeatStyleValue::RepeatRepeatValue, 0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
            style->setMaskRepeatY(newCssValue.repeatStyleValue(), 0);
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
                    style->setMaskRepeatY(RepeatStyleValue::RepeatRepeatValue,
                                          i);
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::RepeatStyleValueKind) {
                    style->setMaskRepeatY(item.repeatStyleValue(), i);
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MaskType:
        style->resetMaskTypes();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setMaskType(parentStyle->maskType());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMaskType(MaskTypeValue::LuminanceMaskTypeValue);
        } else {
            style->setMaskType(newCssValue.maskTypeValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeOpacity:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setStrokeOpacity(parentStyle->strokeOpacity());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStrokeOpacity(1);
        } else {
            float beforeClip = newCssValue.numberValue();
            style->setStrokeOpacity(
                beforeClip < 0 ? 0 : (beforeClip > 1.0 ? 1.0 : beforeClip));
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeLineCap:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setStrokeLineCap(parentStyle->strokeLineCap());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStrokeLineCap(StrokeLineCap::Butt);
        } else {
            style->setStrokeLineCap(newCssValue.strokeLineCap());
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeLineJoin:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setStrokeLineJoin(parentStyle->strokeLineJoin());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStrokeLineJoin(StrokeLineJoin::Miter);
        } else {
            style->setStrokeLineJoin(newCssValue.strokeLineJoin());
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeMiterLimit:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setStrokeMiterLimit(parentStyle->strokeMiterLimit());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStrokeMiterLimit(4);
        } else {
            style->setStrokeMiterLimit(newCssValue.numberValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeDasharray:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setStrokeDashArray(parentStyle->strokeDasharray());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
        } else {
            ValueList* list = newCssValue.multiValue();
            GCAtomicVector<double> array;
            for (unsigned int i = 0; i < list->size(); i++) {
                if ((*list)[i].valueKind() == CSSStyleValuePair::Number) {
                    array.push_back((*list)[i].numberValue());
                }
            }
            style->setStrokeDashArray(array);
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeDashoffset:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setStrokeDashoffset(parentStyle->strokeDashoffset());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStrokeDashoffset(0);
        } else {
            style->setStrokeDashoffset(newCssValue.numberValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::TransitionProperty:
        style->resetTransitionProperties();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyTransitionProperty(element, style, parentStyle, newCssValue,
                                    0);
        } else {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                applyTransitionProperty(element, style, parentStyle, (*list)[i],
                                        i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::TransitionDuration:
        style->resetTransitionDurations();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyTransitionDuration(element, style, parentStyle, newCssValue,
                                    0);
        } else {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                applyTransitionDuration(element, style, parentStyle, (*list)[i],
                                        i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::TransitionTimingFunction:
        style->resetTransitionTimingFunctions();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyTransitionTimingFunction(element, style, parentStyle,
                                          newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                applyTransitionTimingFunction(element, style, parentStyle,
                                              (*list)[i], i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::TransitionDelay:
        style->resetTransitionDelays();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyTransitionDelay(element, style, parentStyle, newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                applyTransitionDelay(element, style, parentStyle, (*list)[i],
                                     i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationName:
        style->resetAnimationNames();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationName(element, style, parentStyle, newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationName(element, style, parentStyle, (*list)[i], i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationDuration:
        style->resetAnimationDurations();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationDuration(element, style, parentStyle, newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationDuration(element, style, parentStyle, (*list)[i],
                                       i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationTimingFunction:
        style->resetAnimationTimingFunctions();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationTimingFunction(element, style, parentStyle,
                                         newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationTimingFunction(element, style, parentStyle,
                                             (*list)[i], i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationDelay:
        style->resetAnimationDelays();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationDelay(element, style, parentStyle, newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationDelay(element, style, parentStyle, (*list)[i], i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationIterationCount:
        style->resetAnimationIterationCount();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationIterationCount(element, style, parentStyle,
                                         newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationIterationCount(element, style, parentStyle,
                                             (*list)[i], i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationDirection:
        style->resetAnimationDirection();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationDirection(element, style, parentStyle, newCssValue,
                                    0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationDirection(element, style, parentStyle, (*list)[i],
                                        i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationPlayState:
        style->resetAnimationPlayState();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationPlayState(element, style, parentStyle, newCssValue,
                                    0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationPlayState(element, style, parentStyle, (*list)[i],
                                        i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::AnimationFillMode:
        style->resetAnimationFillMode();
        if (newCssValue.valueKind() != CSSStyleValuePair::ValueListKind) {
            applyAnimationFillMode(element, style, parentStyle, newCssValue, 0);
        } else {
            ValueList* list = newCssValue.multiValue();
            STARFISH_ASSERT(list != nullptr);
            for (unsigned int i = 0; i < list->size(); i++) {
                applyAnimationFillMode(element, style, parentStyle, (*list)[i],
                                       i);
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderImageSlice:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderImageSlices(
                BorderImageLengthBox(Length(Length::Percent, 1.0)));
            style->setBorderImageSliceFill(false);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setBorderImageSlices(parentStyle->border().image().slices());
            style->setBorderImageSliceFill(
                parentStyle->border().image().sliceFill());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            BorderImageLength t, r, b, l;
            ValueList* values = newCssValue.multiValue();
            unsigned int size = values->size();
            if ((*values)[size - 1].valueKind() ==
                CSSStyleValuePair::ValueKind::KeywordValueKind) {
                style->setBorderImageSliceFill(true);
                size--;
            }

            if ((*values)[0].valueKind() ==
                CSSStyleValuePair::ValueKind::Number) {
                t.setValue((*values)[0].numberValue());
            } else {
                Optional<Length> nTop = LengthUtil::convertValueToLength(
                    (*values)[0].valueKind(), (*values)[0].value());
                if (nTop.hasValue()) {
                    t = nTop.getValue();
                }
            }
            if (size > 1) {
                if ((*values)[1].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    r.setValue((*values)[1].numberValue());
                } else {
                    Optional<Length> nRight = LengthUtil::convertValueToLength(
                        (*values)[1].valueKind(), (*values)[1].value());
                    if (nRight.hasValue()) {
                        r = nRight.getValue();
                    }
                }
            } else {
                r = t;
            }
            if (size > 2) {
                if ((*values)[2].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    b.setValue((*values)[2].numberValue());
                } else {
                    Optional<Length> nBottom = LengthUtil::convertValueToLength(
                        (*values)[2].valueKind(), (*values)[2].value());
                    if (nBottom.hasValue()) {
                        b = nBottom.getValue();
                    }
                }
            } else {
                b = t;
            }
            if (size > 3) {
                if ((*values)[3].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    l.setValue((*values)[3].numberValue());
                } else {
                    Optional<Length> nLeft = LengthUtil::convertValueToLength(
                        (*values)[3].valueKind(), (*values)[3].value());
                    if (nLeft.hasValue()) {
                        l = nLeft.getValue();
                    }
                }
            } else {
                l = r;
            }
            style->setBorderImageSlices(BorderImageLengthBox(l, r, t, b));
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderImageSource:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            BorderData pBorder = parentStyle->border();
            style->setBorderImageSource(pBorder.image().url());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderImageSource(String::emptyString);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::None) {
            style->setBorderImageSource(String::emptyString);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::GradientValueKind) {
            style->setBorderImageSource(newCssValue.gradientValue());
        } else {
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::UrlValueKind ==
                            newCssValue.valueKind());
            style->setBorderImageSource(newCssValue.urlValue(origin));
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderImageRepeat:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            BorderImageData borderImage = parentStyle->border().image();
            style->setBorderImageRepeatX(borderImage.repeatX());
            style->setBorderImageRepeatY(borderImage.repeatY());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderImageRepeatX(BorderImageRepeatValue::StretchValue);
            style->setBorderImageRepeatY(BorderImageRepeatValue::StretchValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* list = newCssValue.multiValue();
            if (list->size() == 1) {
                style->setBorderImageRepeatX(
                    (*list)[0].borderImageRepeatValue());
                style->setBorderImageRepeatY(
                    (*list)[0].borderImageRepeatValue());
            } else if (list->size() == 2) {
                style->setBorderImageRepeatX(
                    (*list)[0].borderImageRepeatValue());
                style->setBorderImageRepeatY(
                    (*list)[1].borderImageRepeatValue());
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderImageOutset:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setBorderImageOutsets(
                parentStyle->border().image().outsets());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderImageOutsets(
                BorderImageLengthBox(Length(Length::Fixed, 0)));
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            BorderImageLength t, r, b, l;
            ValueList* values = newCssValue.multiValue();
            unsigned int size = values->size();

            if ((*values)[0].valueKind() ==
                CSSStyleValuePair::ValueKind::Number) {
                t.setValue((*values)[0].numberValue());
            } else {
                t = (*values)[0].toLengthValue();
            }
            if (size > 1) {
                if ((*values)[1].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    r.setValue((*values)[1].numberValue());
                } else {
                    r = (*values)[1].toLengthValue();
                }
            } else {
                r = t;
            }
            if (size > 2) {
                if ((*values)[2].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    b.setValue((*values)[2].numberValue());
                } else {
                    b = (*values)[2].toLengthValue();
                }
            } else {
                b = t;
            }
            if (size > 3) {
                if ((*values)[3].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    l.setValue((*values)[3].numberValue());
                } else {
                    l = (*values)[3].toLengthValue();
                }
            } else {
                l = r;
            }
            style->setBorderImageOutsets(BorderImageLengthBox(l, r, t, b));
        }

        break;
    case CSSStyleValuePair::KeyKind::BorderImageWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setBorderImageWidths(parentStyle->border().image().widths());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderImageWidths(BorderImageLengthBox(1.0));
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            BorderImageLength t, r, b, l;
            ValueList* values = newCssValue.multiValue();
            unsigned int size = values->size();

            if ((*values)[0].valueKind() ==
                CSSStyleValuePair::ValueKind::Number) {
                t.setValue((*values)[0].numberValue());
            } else {
                Optional<Length> nTop = LengthUtil::convertValueToLength(
                    (*values)[0].valueKind(), (*values)[0].value());
                if (nTop.hasValue()) {
                    t = nTop.getValue();
                }
            }
            if (size > 1) {
                if ((*values)[1].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    r.setValue((*values)[1].numberValue());
                } else {
                    Optional<Length> nRight = LengthUtil::convertValueToLength(
                        (*values)[1].valueKind(), (*values)[1].value());
                    if (nRight.hasValue()) {
                        r = nRight.getValue();
                    }
                }
            } else {
                r = t;
            }
            if (size > 2) {
                if ((*values)[2].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    b.setValue((*values)[2].numberValue());
                } else {
                    Optional<Length> nBottom = LengthUtil::convertValueToLength(
                        (*values)[2].valueKind(), (*values)[2].value());
                    if (nBottom.hasValue()) {
                        b = nBottom.getValue();
                    }
                }
            } else {
                b = t;
            }
            if (size > 3) {
                if ((*values)[3].valueKind() ==
                    CSSStyleValuePair::ValueKind::Number) {
                    l.setValue((*values)[3].numberValue());
                } else {
                    Optional<Length> nLeft = LengthUtil::convertValueToLength(
                        (*values)[3].valueKind(), (*values)[3].value());
                    if (nLeft.hasValue()) {
                        l = nLeft.getValue();
                    }
                }
            } else {
                l = r;
            }
            style->setBorderImageWidths(BorderImageLengthBox(l, r, t, b));
        }

        break;
    case CSSStyleValuePair::KeyKind::BorderCollapse:
        // separate | collapse | initial | inherit

        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->m_inheritedStyles.m_borderCollapse =
                parentStyle->m_inheritedStyles.m_borderCollapse;
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->m_inheritedStyles.m_borderCollapse =
                BorderCollapseValue::SeparateBorderCollapseValue;
            break;
        default:
            STARFISH_ASSERT(
                CSSStyleValuePair::ValueKind::BorderCollapseValueKind ==
                newCssValue.valueKind());
            style->m_inheritedStyles.m_borderCollapse =
                newCssValue.borderCollapseValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderSpacing:
        // Length | initial | inherit

        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setHorizontalBorderSpacing(
                parentStyle->horizontalBorderSpacing());
            style->setVerticalBorderSpacing(
                parentStyle->verticalBorderSpacing());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->setHorizontalBorderSpacing(Length(Length::Fixed, 0));
            style->setVerticalBorderSpacing(Length(Length::Fixed, 0));
            break;
        case CSSStyleValuePair::ValueKind::Length:
        case CSSStyleValuePair::ValueKind::CalcValueKind: {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setHorizontalBorderSpacing(length.getValue());
                style->setVerticalBorderSpacing(length.getValue());
            } else {
                style->setHorizontalBorderSpacing(Length(Length::Fixed, 0));
                style->setVerticalBorderSpacing(Length(Length::Fixed, 0));
            }
            break;
        }
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::ValueListKind ==
                            newCssValue.valueKind());
            ValueList* list = newCssValue.multiValue();
            Optional<Length> hbLength = LengthUtil::convertValueToLength(
                (*list)[0].valueKind(), (*list)[0].value());
            Optional<Length> vbLength = LengthUtil::convertValueToLength(
                (*list)[1].valueKind(), (*list)[1].value());
            if (hbLength.hasValue()) {
                style->setHorizontalBorderSpacing(hbLength.getValue());
            } else {
                style->setHorizontalBorderSpacing(Length(Length::Fixed, 0));
            }

            if (vbLength.hasValue()) {
                style->setVerticalBorderSpacing(vbLength.getValue());
            } else {
                style->setVerticalBorderSpacing(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::CaptionSide:
        // top | bottom | initial | inherit
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->m_inheritedStyles.m_captionSide =
                parentStyle->m_inheritedStyles.m_captionSide;
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->m_inheritedStyles.m_captionSide =
                CaptionSideValue::TopCaptionSideValue;
            break;
        default:
            STARFISH_ASSERT(
                CSSStyleValuePair::ValueKind::CaptionSideValueKind ==
                newCssValue.valueKind());
            style->m_inheritedStyles.m_captionSide =
                newCssValue.captionSideValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::EmptyCells:
        // show | hide | initial | inherit
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->m_inheritedStyles.m_emptyCells =
                parentStyle->m_inheritedStyles.m_emptyCells;
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->m_inheritedStyles.m_emptyCells =
                EmptyCellsValue::ShowEmptyCellsValue;
            break;
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::EmptyCellsValueKind ==
                            newCssValue.valueKind());
            style->m_inheritedStyles.m_emptyCells =
                newCssValue.emptyCellsValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::LineHeight:
        // <normal> | number | length | percentage | inherit
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setLineHeight(parentStyle->lineHeight());
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Normal) {
            // The compute value should be 'normal'.
            // https://developer.mozilla.org/ko/docs/Web/CSS/line-height.
            style->setLineHeight(Length(Length::Percent, -100));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Number) {
            // The computed value should be same as the specified value.
            style->setLineHeight(
                Length(Length::InheritableNumber, newCssValue.numberValue()));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                auto value = length.getValue();
                if (value.isCalc() &&
                    value.calcData()->calcValueType().m_type ==
                        CalcValueType::ValueKind::kNumber) {
                    // The computed value should be same as the specified
                    // value.
                    style->setLineHeight(
                        Length(Length::InheritableNumber,
                               value.calcData()->specifiedValue(0, element)));
                } else {
                    style->setLineHeight(length.getValue());
                }
            } else {
                style->setLineHeight(Length(Length::Percent, -100));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::LineClamp:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::None) {
            style->setLineClamp(0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Number) {
            style->setLineClamp(newCssValue.numberValue());
        }
        break;
#define ADD_RESOLVE_STYLE_POS(POS, pos)                                  \
    case CSSStyleValuePair::KeyKind::POS:                                \
        if (newCssValue.valueKind() ==                                   \
            CSSStyleValuePair::ValueKind::Inherit) {                     \
            style->set##POS(parentStyle->pos());                         \
            element->parentNode()                                        \
                ->style()                                                \
                ->markSomeNonInheritMemberExplicitlyInherited();         \
        } else if (newCssValue.valueKind() ==                            \
                       CSSStyleValuePair::ValueKind::Initial ||          \
                   newCssValue.valueKind() ==                            \
                       CSSStyleValuePair::ValueKind::Unset ||            \
                   newCssValue.valueKind() ==                            \
                       CSSStyleValuePair::ValueKind::Auto) {             \
            style->set##POS(Length());                                   \
        } else if (newCssValue.valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::Length) {               \
            style->set##POS(newCssValue.cssLengthValue().toLength());    \
        } else if (newCssValue.valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::Percentage) {           \
            style->set##POS(                                             \
                Length(Length::Percent, newCssValue.percentageValue())); \
        } else if (newCssValue.valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::CalcValueKind) {        \
            Optional<Length> length = LengthUtil::convertValueToLength(  \
                newCssValue.valueKind(), newCssValue.value());           \
            if (length.hasValue()) {                                     \
                style->set##POS(length.getValue());                      \
            } else {                                                     \
                style->set##POS(Length());                               \
            }                                                            \
        } else {                                                         \
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();                \
        }                                                                \
        break;
        ADD_RESOLVE_STYLE_POS(Top, top)
        ADD_RESOLVE_STYLE_POS(Right, right)
        ADD_RESOLVE_STYLE_POS(Bottom, bottom)
        ADD_RESOLVE_STYLE_POS(Left, left)
#undef ADD_RESOLVE_STYLE_POS
#define ADD_RESOLVE_STYLE_BORDER_STYLE(POS, pos)                          \
    case CSSStyleValuePair::KeyKind::Border##POS##Style:                  \
        if (element->isHTMLTableElement() &&                              \
            style->pseudoType() != PseudoElementNone) {                   \
            break;                                                        \
        } else if (newCssValue.valueKind() ==                             \
                   CSSStyleValuePair::ValueKind::Inherit) {               \
            element->parentNode()                                         \
                ->style()                                                 \
                ->markSomeNonInheritMemberExplicitlyInherited();          \
            BorderData pBorder = parentStyle->border();                   \
            style->setBorder##POS##Style(pBorder.pos().style());          \
        } else if ((newCssValue.valueKind() ==                            \
                    CSSStyleValuePair::ValueKind::Initial) ||             \
                   (newCssValue.valueKind() ==                            \
                    CSSStyleValuePair::ValueKind::Unset)) {               \
            style->setBorder##POS##Style(                                 \
                BorderStyleValue::NoneBorderStyleValue);                  \
        } else if (newCssValue.valueKind() ==                             \
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {  \
            style->setBorder##POS##Style(newCssValue.borderStyleValue()); \
        } else {                                                          \
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();                 \
        }                                                                 \
        break;
        ADD_RESOLVE_STYLE_BORDER_STYLE(Top, top)
        ADD_RESOLVE_STYLE_BORDER_STYLE(Right, right)
        ADD_RESOLVE_STYLE_BORDER_STYLE(Bottom, bottom)
        ADD_RESOLVE_STYLE_BORDER_STYLE(Left, left)
#undef ADD_RESOLVE_STYLE_BORDER_STYLE
    case CSSStyleValuePair::KeyKind::BorderBlockStartStyle:
        if (element->isHTMLTableElement() &&
            style->pseudoType() != PseudoElementNone) {
            break;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderBlockStartStyle(
                parentStyle->borderBlockStart().borderValue().style());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderBlockStartStyle(
                BorderStyleValue::NoneBorderStyleValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {
            style->setBorderBlockStartStyle(newCssValue.borderStyleValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderBlockEndStyle:
        if (element->isHTMLTableElement() &&
            style->pseudoType() != PseudoElementNone) {
            break;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderBlockEndStyle(
                parentStyle->borderBlockEnd().borderValue().style());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderBlockEndStyle(
                BorderStyleValue::NoneBorderStyleValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {
            style->setBorderBlockEndStyle(newCssValue.borderStyleValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartStyle:
        if (element->isHTMLTableElement() &&
            style->pseudoType() != PseudoElementNone) {
            break;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderInlineStartStyle(
                parentStyle->borderInlineStart().borderValue().style());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderInlineStartStyle(
                BorderStyleValue::NoneBorderStyleValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {
            style->setBorderInlineStartStyle(newCssValue.borderStyleValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineEndStyle:
        if (element->isHTMLTableElement() &&
            style->pseudoType() != PseudoElementNone) {
            break;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderInlineEndStyle(
                parentStyle->borderInlineEnd().borderValue().style());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderInlineEndStyle(
                BorderStyleValue::NoneBorderStyleValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {
            style->setBorderInlineEndStyle(newCssValue.borderStyleValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
#define ADD_RESOLVE_STYLE_BORDER_WIDTH(POS, pos)                         \
    case CSSStyleValuePair::KeyKind::Border##POS##Width:                 \
        if (newCssValue.valueKind() ==                                   \
            CSSStyleValuePair::ValueKind::Inherit) {                     \
            BorderData pBorder = parentStyle->border();                  \
            style->setBorder##POS##Width(pBorder.pos().width());         \
            element->parentNode()                                        \
                ->style()                                                \
                ->markSomeNonInheritMemberExplicitlyInherited();         \
        } else if ((newCssValue.valueKind() ==                           \
                    CSSStyleValuePair::ValueKind::Initial) ||            \
                   (newCssValue.valueKind() ==                           \
                    CSSStyleValuePair::ValueKind::Unset)) {              \
            style->setBorder##POS##Width(Length(Length::Fixed, 3));      \
        } else if (newCssValue.valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::Length) {               \
            style->setBorder##POS##Width(                                \
                newCssValue.cssLengthValue().toLength());                \
        } else if (newCssValue.valueKind() ==                            \
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) { \
            if (newCssValue.borderWidthValue() ==                        \
                BorderWidthValue::ThinBorderWidthValue) {                \
                style->setBorder##POS##Width(Length(Length::Fixed, 1));  \
            } else if (newCssValue.borderWidthValue() ==                 \
                       BorderWidthValue::MediumBorderWidthValue) {       \
                style->setBorder##POS##Width(Length(Length::Fixed, 3));  \
            } else if (newCssValue.borderWidthValue() ==                 \
                       BorderWidthValue::ThickBorderWidthValue) {        \
                style->setBorder##POS##Width(Length(Length::Fixed, 5));  \
            }                                                            \
        } else {                                                         \
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();                \
        }                                                                \
        break;
        ADD_RESOLVE_STYLE_BORDER_WIDTH(Top, top)
        ADD_RESOLVE_STYLE_BORDER_WIDTH(Right, right)
        ADD_RESOLVE_STYLE_BORDER_WIDTH(Bottom, bottom)
        ADD_RESOLVE_STYLE_BORDER_WIDTH(Left, left)
#undef ADD_RESOLVE_STYLE_BORDER_WIDTH
    case CSSStyleValuePair::KeyKind::BorderBlockStartWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderBlockStartWidth(
                parentStyle->borderBlockStart().borderValue().width());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderBlockStartWidth(Length(Length::Fixed, 3));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            style->setBorderBlockStartWidth(
                newCssValue.cssLengthValue().toLength());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) {
            if (newCssValue.borderWidthValue() ==
                BorderWidthValue::ThinBorderWidthValue) {
                style->setBorderBlockStartWidth(Length(Length::Fixed, 1));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::MediumBorderWidthValue) {
                style->setBorderBlockStartWidth(Length(Length::Fixed, 3));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::ThickBorderWidthValue) {
                style->setBorderBlockStartWidth(Length(Length::Fixed, 5));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderBlockEndWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderBlockEndWidth(
                parentStyle->borderBlockStart().borderValue().width());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderBlockEndWidth(Length(Length::Fixed, 3));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            style->setBorderBlockEndWidth(
                newCssValue.cssLengthValue().toLength());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) {
            if (newCssValue.borderWidthValue() ==
                BorderWidthValue::ThinBorderWidthValue) {
                style->setBorderBlockEndWidth(Length(Length::Fixed, 1));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::MediumBorderWidthValue) {
                style->setBorderBlockEndWidth(Length(Length::Fixed, 3));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::ThickBorderWidthValue) {
                style->setBorderBlockEndWidth(Length(Length::Fixed, 5));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderInlineStartWidth(
                parentStyle->borderInlineStart().borderValue().width());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderInlineStartWidth(Length(Length::Fixed, 3));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            style->setBorderInlineStartWidth(
                newCssValue.cssLengthValue().toLength());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) {
            if (newCssValue.borderWidthValue() ==
                BorderWidthValue::ThinBorderWidthValue) {
                style->setBorderInlineStartWidth(Length(Length::Fixed, 1));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::MediumBorderWidthValue) {
                style->setBorderInlineStartWidth(Length(Length::Fixed, 3));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::ThickBorderWidthValue) {
                style->setBorderInlineStartWidth(Length(Length::Fixed, 5));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BorderInlineEndWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderInlineEndWidth(
                parentStyle->borderInlineEnd().borderValue().width());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setBorderInlineEndWidth(Length(Length::Fixed, 3));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            style->setBorderInlineEndWidth(
                newCssValue.cssLengthValue().toLength());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) {
            if (newCssValue.borderWidthValue() ==
                BorderWidthValue::ThinBorderWidthValue) {
                style->setBorderInlineEndWidth(Length(Length::Fixed, 1));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::MediumBorderWidthValue) {
                style->setBorderInlineEndWidth(Length(Length::Fixed, 3));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::ThickBorderWidthValue) {
                style->setBorderInlineEndWidth(Length(Length::Fixed, 5));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
#define ADD_RESOLVE_STYLE_BORDER_COLOR(POS, pos)                            \
    case CSSStyleValuePair::KeyKind::Border##POS##Color:                    \
        if (newCssValue.valueKind() ==                                      \
            CSSStyleValuePair::ValueKind::Inherit) {                        \
            BorderData pBorder = parentStyle->border();                     \
            style->setBorder##POS##Color(pBorder.pos().color());            \
            element->parentNode()                                           \
                ->style()                                                   \
                ->markSomeNonInheritMemberExplicitlyInherited();            \
        } else if ((newCssValue.valueKind() ==                              \
                    CSSStyleValuePair::ValueKind::Initial) ||               \
                   (newCssValue.valueKind() ==                              \
                    CSSStyleValuePair::ValueKind::Unset)) {                 \
            style->clearBorder##POS##Color();                               \
        } else if (newCssValue.valueKind() ==                               \
                   CSSStyleValuePair::ValueKind::ColorValueKind) {          \
            style->setBorder##POS##Color(newCssValue.colorValue());         \
        } else {                                                            \
            STARFISH_ASSERT(                                                \
                newCssValue.valueKind() ==                                  \
                CSSStyleValuePair::ValueKind::NamedColorValueKind);         \
            if (newCssValue.namedColorValue() ==                            \
                NamedColor::NamedColorValue::currentColor) {                \
                style->clearBorder##POS##Color();                           \
            } else {                                                        \
                style->setBorder##POS##Color(NamedColor::namedColorToColor( \
                    newCssValue.namedColorValue()));                        \
            }                                                               \
        }                                                                   \
        break;
        ADD_RESOLVE_STYLE_BORDER_COLOR(Top, top)
        ADD_RESOLVE_STYLE_BORDER_COLOR(Right, right)
        ADD_RESOLVE_STYLE_BORDER_COLOR(Bottom, bottom)
        ADD_RESOLVE_STYLE_BORDER_COLOR(Left, left)
#undef ADD_RESOLVE_STYLE_BORDER_COLOR
    case CSSStyleValuePair::KeyKind::BorderBlockStartColor: {
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderBlockStartColor(
                parentStyle->borderBlockStart().borderValue().color());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->clearBorderBlockStartColor();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setBorderBlockStartColor(newCssValue.colorValue());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->clearBorderBlockStartColor();
            } else {
                style->setBorderBlockStartColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderBlockEndColor: {
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderBlockEndColor(
                parentStyle->borderBlockEnd().borderValue().color());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->clearBorderBlockEndColor();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setBorderBlockEndColor(newCssValue.colorValue());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->clearBorderBlockEndColor();
            } else {
                style->setBorderBlockEndColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineStartColor: {
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderInlineStartColor(
                parentStyle->borderInlineStart().borderValue().color());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->clearBorderInlineStartColor();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setBorderInlineStartColor(newCssValue.colorValue());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->clearBorderInlineStartColor();
            } else {
                style->setBorderInlineStartColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineEndColor: {
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setBorderInlineEndColor(
                parentStyle->borderInlineEnd().borderValue().color());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->clearBorderInlineEndColor();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setBorderInlineEndColor(newCssValue.colorValue());
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->clearBorderInlineEndColor();
            } else {
                style->setBorderInlineEndColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
    } break;
    case CSSStyleValuePair::KeyKind::BorderBlockStart: {
        // Generally, shorthand properties are divided into longhand
        // properties and registered in cssvalues. so, there is no need to
        // directly handle it. But not in this case.
        // to handle special case for color of border-block-start, marks that it
        // is derived from a shorthand. But it still don't need to apply this
        // value directly to computed style.
        style->setBorderBlockStartFromShorthand(true);
    } break;
    case CSSStyleValuePair::KeyKind::BorderBlockEnd: {
        style->setBorderBlockEndFromShorthand(true);
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineStart: {
        style->setBorderInlineStartFromShorthand(true);
    } break;
    case CSSStyleValuePair::KeyKind::BorderInlineEnd: {
        style->setBorderInlineEndFromShorthand(true);
    } break;
#define ADD_RESOLVE_STYLE_MARGIN(POS, pos)                              \
    case CSSStyleValuePair::KeyKind::Margin##POS:                       \
        if (newCssValue.valueKind() ==                                  \
            CSSStyleValuePair::ValueKind::Inherit) {                    \
            LengthData pMargin = parentStyle->margin();                 \
            style->setMargin##POS(pMargin.pos());                       \
            element->parentNode()                                       \
                ->style()                                               \
                ->markSomeNonInheritMemberExplicitlyInherited();        \
        } else if ((newCssValue.valueKind() ==                          \
                    CSSStyleValuePair::ValueKind::Initial) ||           \
                   (newCssValue.valueKind() ==                          \
                    CSSStyleValuePair::ValueKind::Unset)) {             \
            style->setMargin##POS(Length(Length::Fixed, 0));            \
        } else {                                                        \
            Optional<Length> length = LengthUtil::convertValueToLength( \
                newCssValue.valueKind(), newCssValue.value());          \
            if (length.hasValue()) {                                    \
                style->setMargin##POS(length.getValue());               \
            } else {                                                    \
                style->setMargin##POS(Length(Length::Fixed, 0));        \
            }                                                           \
        }                                                               \
        break;
        ADD_RESOLVE_STYLE_MARGIN(Top, top)
        ADD_RESOLVE_STYLE_MARGIN(Right, right)
        ADD_RESOLVE_STYLE_MARGIN(Bottom, bottom)
        ADD_RESOLVE_STYLE_MARGIN(Left, left)
#undef ADD_RESOLVE_STYLE_MARGIN
    case CSSStyleValuePair::KeyKind::MarginBlockStart:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMarginBlockStart(parentStyle->marginBlockStart());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMarginBlockStart(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMarginBlockStart(length.getValue());
            } else {
                style->setMarginBlockStart(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MarginBlockEnd:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMarginBlockEnd(parentStyle->marginBlockEnd());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMarginBlockEnd(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMarginBlockEnd(length.getValue());
            } else {
                style->setMarginBlockEnd(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MarginInlineEnd:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMarginInlineEnd(parentStyle->marginInlineEnd());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMarginInlineEnd(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMarginInlineEnd(length.getValue());
            } else {
                style->setMarginInlineEnd(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::MarginInlineStart:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setMarginInlineStart(parentStyle->marginInlineStart());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setMarginInlineStart(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setMarginInlineStart(length.getValue());
            } else {
                style->setMarginInlineStart(Length(Length::Fixed, 0));
            }
        }
        break;
#define ADD_RESOLVE_STYLE_PADDING(POS, pos)                             \
    case CSSStyleValuePair::KeyKind::Padding##POS:                      \
        if (newCssValue.valueKind() ==                                  \
            CSSStyleValuePair::ValueKind::Inherit) {                    \
            LengthData pPadding = parentStyle->padding();               \
            style->setPadding##POS(pPadding.pos());                     \
            element->parentNode()                                       \
                ->style()                                               \
                ->markSomeNonInheritMemberExplicitlyInherited();        \
        } else if ((newCssValue.valueKind() ==                          \
                    CSSStyleValuePair::ValueKind::Initial) ||           \
                   (newCssValue.valueKind() ==                          \
                    CSSStyleValuePair::ValueKind::Unset)) {             \
            style->setPadding##POS(Length(Length::Fixed, 0));           \
        } else {                                                        \
            Optional<Length> length = LengthUtil::convertValueToLength( \
                newCssValue.valueKind(), newCssValue.value());          \
            if (length.hasValue()) {                                    \
                style->setPadding##POS(length.getValue());              \
            } else {                                                    \
                style->setPadding##POS(Length(Length::Fixed, 0));       \
            }                                                           \
        }                                                               \
        break;
        ADD_RESOLVE_STYLE_PADDING(Top, top)
        ADD_RESOLVE_STYLE_PADDING(Right, right)
        ADD_RESOLVE_STYLE_PADDING(Bottom, bottom)
        ADD_RESOLVE_STYLE_PADDING(Left, left)
#undef ADD_RESOLVE_STYLE_PADDING
    case CSSStyleValuePair::KeyKind::PaddingBlockStart:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setPaddingBlockStart(parentStyle->paddingBlockStart());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setPaddingBlockStart(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setPaddingBlockStart(length.getValue());
            } else {
                style->setPaddingBlockStart(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingBlockEnd:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setPaddingBlockEnd(parentStyle->paddingBlockEnd());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setPaddingBlockEnd(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setPaddingBlockEnd(length.getValue());
            } else {
                style->setPaddingBlockEnd(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingInlineEnd:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setPaddingInlineEnd(parentStyle->paddingInlineEnd());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setPaddingInlineEnd(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setPaddingInlineEnd(length.getValue());
            } else {
                style->setPaddingInlineEnd(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::PaddingInlineStart:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setPaddingInlineStart(parentStyle->paddingInlineStart());
            element->parentNode()
                ->style()
                ->markSomeNonInheritMemberExplicitlyInherited();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setPaddingInlineStart(Length(Length::Fixed, 0));
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setPaddingInlineStart(length.getValue());
            } else {
                style->setPaddingInlineStart(Length(Length::Fixed, 0));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::Opacity:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setOpacity(parentStyle->opacity());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setOpacity(1.0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Number) {
            float beforeClip = newCssValue.numberValue();
            style->setOpacity(
                beforeClip < 0 ? 0 : (beforeClip > 1.0 ? 1.0 : beforeClip));
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::OverflowX:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_overflowX = parentStyle->overflowX();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_overflowX = OverflowValue::VisibleOverflow;
        } else {
            style->m_overflowX = newCssValue.overflowValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::OverflowY:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_overflowY = parentStyle->overflowY();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_overflowY = OverflowValue::VisibleOverflow;
        } else {
            style->m_overflowY = newCssValue.overflowValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::Visibility:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_inheritedStyles.m_visibility = parentStyle->visibility();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->m_inheritedStyles.m_visibility =
                VisibilityValue::VisibleVisibilityValue;
        } else {
            style->m_inheritedStyles.m_visibility =
                newCssValue.visibilityValue();
        }
        break;
    case CSSStyleValuePair::KeyKind::ZIndex:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setZIndex(parentStyle->zIndex());
            style->m_zIndexSpecifiedByUser = false;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Unset ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Auto) {
            style->setZIndex(0);
            style->m_zIndexSpecifiedByUser = false;
        } else {
            style->setZIndex(newCssValue.int32Value());
            style->m_zIndexSpecifiedByUser = true;
        }
        break;
    case CSSStyleValuePair::KeyKind::Transform:
        style->clearTransform();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setTransform(parentStyle->transforms());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Unset ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::None) {
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::TransformFunctions);
            CSSTransformFunctions* funcs = newCssValue.transformValue();
            funcs->toTransformDataGroup(element, style);
        }
        break;
    case CSSStyleValuePair::KeyKind::TransformOrigin:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->setTransformOrigin(parentStyle->transformOrigin());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setTransformOriginValue(Length(Length::Percent, 0.5f),
                                           Length(Length::Percent, 0.5f),
                                           Length(Length::Fixed, 0.f));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* list = newCssValue.multiValue();
            Length xAxis, yAxis, zAxis;

            xAxis = Length(Length::Percent, 0.5f);
            yAxis = Length(Length::Percent, 0.5f);
            zAxis = Length(Length::Fixed, 0.f);

            for (unsigned int i = 0; i < std::min(list->size(), (size_t)2);
                 i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() ==
                    CSSStyleValuePair::ValueKind::SideValueKind) {
                    if (item.sideValue() == SideValue::LeftSideValue) {
                        xAxis = Length(Length::Percent, 0.0f);
                    } else if (item.sideValue() == SideValue::RightSideValue) {
                        xAxis = Length(Length::Percent, 1.0f);
                    } else if (item.sideValue() == SideValue::CenterSideValue) {
                    } else if (item.sideValue() == SideValue::TopSideValue) {
                        yAxis = Length(Length::Percent, 0.0f);
                    } else if (item.sideValue() == SideValue::BottomSideValue) {
                        yAxis = Length(Length::Percent, 1.0f);
                    }
                } else {
                    if (i == 0) {
                        Optional<Length> nXAxis =
                            LengthUtil::convertValueToLength(item.valueKind(),
                                                             item.value());
                        if (nXAxis.hasValue()) {
                            xAxis = nXAxis.getValue();
                        }
                    } else {
                        Optional<Length> nYAxis =
                            LengthUtil::convertValueToLength(item.valueKind(),
                                                             item.value());
                        if (nYAxis.hasValue()) {
                            yAxis = nYAxis.getValue();
                        }
                    }
                }
            }

            if (list->size() == 3) {
                Optional<Length> nZAxis = LengthUtil::convertValueToLength(
                    (*list)[2].valueKind(), (*list)[2].value());
                if (nZAxis.hasValue()) {
                    zAxis = nZAxis.getValue();
                }
            }

            style->setTransformOriginValue(xAxis, yAxis, zAxis);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }

        break;
    case CSSStyleValuePair::KeyKind::UnicodeBidi:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_unicodeBidi = parentStyle->m_unicodeBidi;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::UnicodeBidiValueKind) {
            style->setUnicodeBidi(newCssValue.unicodeBidiValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BoxSizing:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            style->m_boxSizing = parentStyle->m_boxSizing;
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->m_boxSizing = BoxSizingValue::ContentBoxBoxSizingValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BoxSizingValueKind) {
            style->setBoxSizing(newCssValue.boxSizingValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::BoxOrient:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_boxOrient = BoxOrientValue::HorizontalBoxOrientValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_boxOrient = parentStyle->m_boxOrient;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BoxOrientValueKind) {
            style->setBoxOrient(newCssValue.boxOrientValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::Content:
        // Initial value is normal and it computes to 'none' for the
        // :before and :after pseudo-elements.
        style->clearContent();
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::ValueListKind);
            ValueList* list = newCssValue.multiValue();
            for (unsigned int i = 0; i < list->size(); i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() == CSSStyleValuePair::ValueKind::None ||
                    item.valueKind() == CSSStyleValuePair::ValueKind::Normal) {
                    return;
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::UrlValueKind) {
                    style->setContentImage(item.urlValue(origin));
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::StringValueKind) {
                    style->setContentText(item.stringValue());
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::Attr) {
                    Optional<String*> attrValue = element->getAttribute(
                        element->document()->createAttributeName(
                            item.attrValue()));
                    if (attrValue.hasValue()) {
                        style->setContentText(attrValue.getValue());
                    }
                    style->m_styleDamageSource = (StyleDamageSource)(
                        style->m_styleDamageSource | StyleDamageFromAttribute);

                    m_ruleSetAttrFilter.push_back(
                        element->document()
                            ->createAttributeName(item.attrValue())
                            .localNameAtomic());
                } else if (item.valueKind() == CSSStyleValuePair::ValueKind::
                                                   CounterFunctionValueKind) {
                    CSSCounterFunction* v = item.counterFunctionValue();
                    Optional<String*> sp = v->separator();
                    Optional<AtomicString> counterName = v->style();
                    const CounterStyle* counter = nullptr;
                    if (counterName.hasValue()) {
                        counter = CounterStyle::getKnownCounter(
                            counterName.getValue().string());
                    }
                    if (!counter) {
                        counter = CounterStyle::getDecimalCounter();
                    }
                    if (sp.hasValue()) {
                        style->setContentCounters(v->name(), sp.getValue(),
                                                  counter);
                    } else {
                        style->setContentCounter(v->name(), counter);
                    }
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::QuoteValueKind) {
                    style->setContentQuote(item.quoteValue());
                } else {
                    STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
                }
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::FlexDirection:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_flexDirection = FlexDirectionValue::RowFlexDirectionValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_flexDirection = parentStyle->m_flexDirection;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FlexDirectionValueKind) {
            style->setFlexDirection(newCssValue.flexDirectionValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FlexWrap:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_flexWrap = FlexWrapValue::NoWrapFlexWrapValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_flexWrap = parentStyle->m_flexWrap;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FlexWrapValueKind) {
            style->setFlexWrap(newCssValue.flexWrapValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::Order:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setOrder(0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setOrder(parentStyle->order());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Int32) {
            style->setOrder(newCssValue.int32Value());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::JustifyContent:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_justifyContent =
                JustifyContentValue::NormalJustifyContentValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_justifyContent = parentStyle->m_justifyContent;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::JustifyContentValueKind) {
            style->setJustifyContent(newCssValue.justifyContentValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::AlignItems:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_alignItems = AlignItemValue::StretchAlignItemValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_alignItems = parentStyle->m_alignItems;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::AlignItemValueKind) {
            style->setAlignItems(newCssValue.alignItemValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::AlignSelf:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Auto) {
            style->m_alignSelfSpecifiedByUser = false;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_alignSelfSpecifiedByUser = false;
            style->m_alignSelf = parentStyle->m_alignSelf;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::AlignItemValueKind) {
            style->m_alignSelfSpecifiedByUser = true;
            style->setAlignSelf(newCssValue.alignItemValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::AlignContent:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->m_alignContent = AlignContentValue::StretchAlignContentValue;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->m_alignContent = parentStyle->m_alignContent;
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::AlignContentValueKind) {
            style->setAlignContent(newCssValue.alignContentValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FlexGrow:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setFlexGrow(0);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setFlexGrow(parentStyle->flexGrow());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Number) {
            style->setFlexGrow(newCssValue.numberValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FlexShrink:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setFlexShrink(1);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setFlexShrink(parentStyle->flexShrink());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Number) {
            style->setFlexShrink(newCssValue.numberValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FlexBasis:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Auto) {
            style->setFlexBasis(FlexBasisData());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setFlexBasis(parentStyle->flexBasis());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FlexBasisValueKind) {
            style->setFlexBasis(FlexBasisData(FlexBasisData::Width));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            Length length = newCssValue.cssLengthValue().toLength();
            if (length.isAuto()) {
                style->setFlexBasis(FlexBasisData(FlexBasisData::Width));
            } else {
                style->setFlexBasis(
                    FlexBasisData(FlexBasisData::Width, length));
            }
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Percentage) {
            Length length =
                Length(Length::Percent, newCssValue.percentageValue());
            style->setFlexBasis(FlexBasisData(FlexBasisData::Width, length));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::CalcValueKind) {
            Optional<Length> maybeLength = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (maybeLength) {
                style->setFlexBasis(
                    FlexBasisData(FlexBasisData::Width, maybeLength.value()));
            } else {
                style->setFlexBasis(FlexBasisData(FlexBasisData::Width,
                                                  Length(Length::Fixed, 0)));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::Fill:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
            style->setFill(new StylePaintData());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setFill(parentStyle->fill());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::None) {
            style->setFill(new StylePaintData(Unit::Color(0, 0, 0, 0)));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setFill(new StylePaintData(newCssValue.colorValue()));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::NamedColorValueKind) {
            if (newCssValue.namedColorValue() == NamedColor::currentColor) {
                style->setFill(new StylePaintData(NamedColor::currentColor));
            } else {
                style->setFill(new StylePaintData(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue())));
            }
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::UrlValueKind) {
            String* urlString = newCssValue.urlStringValue();
            if (urlString->startsWith("#")) {
                AtomicString as = AtomicString::createAtomicString(
                    element->starfish(),
                    urlString->substring(1, urlString->length() - 1));
                style->setFill(new StylePaintData(urlString, as));
            } else {
                style->setFill(new StylePaintData(urlString, nullptr));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FillRule:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
            style->setFillRule(FillRuleValue::FillRuleNonZero);
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setFillRule(parentStyle->fillRule());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::FillRuleValueKind) {
            style->setFillRule(newCssValue.fillRuleValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::FillOpacity:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
            style->setFillOpacity(1);
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setFillOpacity(parentStyle->fillOpacity());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Number) {
            style->setFillOpacity(newCssValue.numberValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::StopColor:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::None:
            style->setStopColor(new StylePaintData(Unit::Color(0, 0, 0, 0xff)));
            break;
        case CSSStyleValuePair::ValueKind::ColorValueKind:
            style->setStopColor(new StylePaintData(newCssValue.colorValue()));
            break;
        case CSSStyleValuePair::ValueKind::NamedColorValueKind:
            style->setStopColor(new StylePaintData(
                NamedColor::namedColorToColor(newCssValue.namedColorValue())));
            break;
        default:
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::StopOpacity: {
        CSSStyleValuePair::ValueKind valueKind = newCssValue.valueKind();
        if ((valueKind == CSSStyleValuePair::ValueKind::Inherit) ||
            (valueKind == CSSStyleValuePair::ValueKind::Unset) ||
            (valueKind == CSSStyleValuePair::ValueKind::Initial)) {
            style->setStopOpacity(1.0);
        } else if (valueKind == CSSStyleValuePair::ValueKind::Number) {
            float rawValue = newCssValue.numberValue();
            style->setStopOpacity(
                rawValue < 0 ? 0 : (rawValue > 1.0 ? 1.0 : rawValue));
        } else if (valueKind == CSSStyleValuePair::ValueKind::Percentage) {
            float rawValue = newCssValue.percentageValue();
            style->setStopOpacity(
                rawValue < 0 ? 0 : (rawValue > 1.0 ? 1.0 : rawValue));
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } break;
    case CSSStyleValuePair::KeyKind::Stroke:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
            style->setStroke(new StylePaintData());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStroke(parentStyle->stroke());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::None) {
            style->setStroke(new StylePaintData(Unit::Color(0, 0, 0, 0)));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setStroke(new StylePaintData(newCssValue.colorValue()));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::NamedColorValueKind) {
            if (newCssValue.namedColorValue() == NamedColor::currentColor) {
                style->setStroke(new StylePaintData(NamedColor::currentColor));
            } else {
                style->setStroke(
                    new StylePaintData(NamedColor::namedColorToColor(
                        newCssValue.namedColorValue())));
            }
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::UrlValueKind) {
            String* urlString = newCssValue.urlStringValue();
            if (urlString->startsWith("#")) {
                AtomicString as = AtomicString::createAtomicString(
                    element->starfish(),
                    urlString->substring(1, urlString->length() - 1));
                style->setStroke(new StylePaintData(urlString, as));
            } else {
                style->setStroke(new StylePaintData(urlString, nullptr));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::StrokeWidth:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Initial) {
            style->setStrokeWidth(Length(Length::Fixed, 1));
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Inherit) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setStrokeWidth(parentStyle->strokeWidth());
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setStrokeWidth(length.getValue());
            } else {
                style->setStrokeWidth(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::X:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setX(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setX(parentStyle->x());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setX(length.getValue());
            } else {
                style->setX(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::Y:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setY(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setY(parentStyle->y());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setY(length.getValue());
            } else {
                style->setY(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::X1:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setX1(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setX1(parentStyle->x1());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setX1(length.getValue());
            } else {
                style->setX1(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::Y1:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setY1(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setY1(parentStyle->y1());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setY1(length.getValue());
            } else {
                style->setY1(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::X2:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setX2(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setX2(parentStyle->x2());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setX2(length.getValue());
            } else {
                style->setX2(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::Y2:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setY2(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setY2(parentStyle->y2());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setY2(length.getValue());
            } else {
                style->setY2(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::R:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setR(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setR(parentStyle->r());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setR(length.getValue());
            } else {
                style->setR(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::D:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setD(String::emptyString);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setD(parentStyle->d());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            style->setD(newCssValue.pathFunctionValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::CX:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setCX(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setCX(parentStyle->cx());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setCX(length.getValue());
            } else {
                style->setCX(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::CY:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setCY(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setCY(parentStyle->cy());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setCY(length.getValue());
            } else {
                style->setCY(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::RX:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setRX(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setRX(parentStyle->rx());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setRX(length.getValue());
            } else {
                style->setRX(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::RY:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setRY(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setRY(parentStyle->ry());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else {
            Optional<Length> length = LengthUtil::convertValueToLength(
                newCssValue.valueKind(), newCssValue.value());
            if (length.hasValue()) {
                style->setRY(length.getValue());
            } else {
                style->setRY(Length());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::ObjectFit:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setObjectFit(ObjectFitValue::FillObjectFitValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setObjectFit(parentStyle->objectFit());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ObjectFitValueKind) {
            style->setObjectFit(newCssValue.objectFitValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::ObjectPosition:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Inherit) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setObjectPosition(parentStyle->objectPositionX(),
                                     parentStyle->objectPositionY());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setObjectPosition(Length(Length::Percent, 0.5f),
                                     Length(Length::Percent, 0.5f));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* list = newCssValue.multiValue();
            Length x, y;

            x = Length(Length::Percent, 0.5f);
            y = Length(Length::Percent, 0.5f);

            for (unsigned int i = 0; i < std::min(list->size(), (size_t)2);
                 i++) {
                const CSSStyleValuePair& item = (*list)[i];
                if (item.valueKind() ==
                    CSSStyleValuePair::ValueKind::SideValueKind) {
                    if (item.sideValue() == SideValue::LeftSideValue) {
                        x = Length(Length::Percent, 0.0f);
                    } else if (item.sideValue() == SideValue::RightSideValue) {
                        x = Length(Length::Percent, 1.0f);
                    } else if (item.sideValue() == SideValue::CenterSideValue) {
                    } else if (item.sideValue() == SideValue::TopSideValue) {
                        y = Length(Length::Percent, 0.0f);
                    } else if (item.sideValue() == SideValue::BottomSideValue) {
                        y = Length(Length::Percent, 1.0f);
                    }
                } else if (item.valueKind() ==
                           CSSStyleValuePair::ValueKind::ValuePairKind) {
                    const CSSStyleValuePair& first = item.pairValue()->first();
                    const CSSStyleValuePair& second =
                        item.pairValue()->second();
                    if (first.valueKind() ==
                        CSSStyleValuePair::ValueKind::SideValueKind) {
                        if (i == 0) {
                            if (first.sideValue() == SideValue::LeftSideValue) {
                                Optional<Length> nX =
                                    LengthUtil::convertValueToLength(
                                        second.valueKind(), second.value());
                                if (nX.hasValue()) {
                                    x = nX.getValue();
                                }
                            } else if (first.sideValue() ==
                                       SideValue::RightSideValue) {
                                CalcData* data = new CalcData();
                                CalcValue val;
                                CalcTerm* term1 = new CalcTerm();
                                if (second.valueKind() ==
                                    CSSStyleValuePair::ValueKind::Percentage) {
                                    val.setType(
                                        CalcValueType::ValueKind::kPercentage);
                                    val.setValue(-1 * second.percentageValue());
                                } else {
                                    val.setType(
                                        CalcValueType::ValueKind::kLength);
                                    val.setValue(-1 * second.cssLengthValue());
                                }
                                term1->appendValue(val);

                                CalcTerm* term2 = new CalcTerm();
                                val.setType(
                                    CalcValueType::ValueKind::kPercentage);
                                val.setValue(1.0f);
                                term2->appendValue(val);

                                data->appendTerm(term1);
                                data->appendTerm(term2);
                                x = Length(data);
                            }
                        } else {
                            if (first.sideValue() == SideValue::TopSideValue) {
                                Optional<Length> nY =
                                    LengthUtil::convertValueToLength(
                                        second.valueKind(), second.value());
                                if (nY.hasValue()) {
                                    y = nY.getValue();
                                }
                            } else if (first.sideValue() ==
                                       SideValue::BottomSideValue) {
                                CalcData* data = new CalcData();
                                CalcValue val;
                                CalcTerm* term1 = new CalcTerm();
                                if (second.valueKind() ==
                                    CSSStyleValuePair::ValueKind::Percentage) {
                                    val.setType(
                                        CalcValueType::ValueKind::kPercentage);
                                    val.setValue(-1 * second.percentageValue());
                                } else {
                                    val.setType(
                                        CalcValueType::ValueKind::kLength);
                                    val.setValue(-1 * second.cssLengthValue());
                                }
                                term1->appendValue(val);

                                CalcTerm* term2 = new CalcTerm();
                                val.setType(
                                    CalcValueType::ValueKind::kPercentage);
                                val.setValue(1.0f);
                                term2->appendValue(val);

                                data->appendTerm(term1);
                                data->appendTerm(term2);
                                y = Length(data);
                            }
                        }
                    }
                } else {
                    if (i == 0) {
                        Optional<Length> nX = LengthUtil::convertValueToLength(
                            item.valueKind(), item.value());
                        if (nX.hasValue()) {
                            x = nX.getValue();
                        }
                    } else {
                        Optional<Length> nY = LengthUtil::convertValueToLength(
                            item.valueKind(), item.value());
                        if (nY.hasValue()) {
                            y = nY.getValue();
                        }
                    }
                }
            }

            style->setObjectPosition(x, y);
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::OutlineWidth:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setOutlineWidth(Length(Length::Fixed, 2));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setOutlineWidth(parentStyle->outlineWidth());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            style->setOutlineWidth(newCssValue.lengthValue());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderWidthValueKind) {
            if (newCssValue.borderWidthValue() ==
                BorderWidthValue::ThinBorderWidthValue) {
                style->setOutlineWidth(Length(Length::Fixed, 1));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::MediumBorderWidthValue) {
                style->setOutlineWidth(Length(Length::Fixed, 3));
            } else if (newCssValue.borderWidthValue() ==
                       BorderWidthValue::ThickBorderWidthValue) {
                style->setOutlineWidth(Length(Length::Fixed, 5));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::OutlineColor:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            if (style->hasRareComputeStyleData()) {
                OutlineData* outline =
                    style->rareComputedStyleData()->outline();
                if (outline) {
                    outline->border().clearColor();
                }
            }
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setOutlineColor(parentStyle->outlineColor());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::ColorValueKind) {
            style->setOutlineColor(newCssValue.colorValue());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::NamedColorValueKind) {
            if (newCssValue.namedColorValue() == NamedColor::currentColor) {
                if (style->hasRareComputeStyleData()) {
                    OutlineData* outline =
                        style->rareComputedStyleData()->outline();
                    if (outline) {
                        outline->border().clearColor();
                    }
                }
            } else {
                style->setOutlineColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::OutlineStyle:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setOutlineStyle(parentStyle->outlineStyle());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Unset)) {
            style->setOutlineStyle(BorderStyleValue::NoneBorderStyleValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::BorderStyleValueKind) {
            style->setOutlineStyle(newCssValue.borderStyleValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::OutlineOffset:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setOutlineOffset(Length(Length::Fixed, 0));
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setOutlineOffset(parentStyle->outlineOffset());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Length) {
            style->setOutlineOffset(newCssValue.lengthValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;
    case CSSStyleValuePair::KeyKind::TextTransform:
        if ((newCssValue.valueKind() ==
             CSSStyleValuePair::ValueKind::Initial) ||
            (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset)) {
            style->setTextTransform(NoneTextTransformValue);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setTextTransform(parentStyle->textTransform());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::TextTransformValueKind) {
            style->setTextTransform(newCssValue.textTransformValue());
        } else {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        break;

#define BORDER_RADIUS_APPLY(AB, ab, AA, BB)                                   \
    case CSSStyleValuePair::KeyKind::Border##AB##Radius:                      \
        if ((newCssValue.valueKind() ==                                       \
             CSSStyleValuePair::ValueKind::Initial) ||                        \
            (newCssValue.valueKind() ==                                       \
             CSSStyleValuePair::ValueKind::Unset)) {                          \
            style->setBorder##AB##Radius(Length(Length::Fixed, 0),            \
                                         Length(Length::Fixed, 0));           \
        } else if (newCssValue.valueKind() ==                                 \
                   CSSStyleValuePair::ValueKind::Inherit) {                   \
            element->parentNode()                                             \
                ->style()                                                     \
                ->markSomeNonInheritMemberExplicitlyInherited();              \
            const auto& p = parentStyle->borderRadius();                      \
            style->setBorder##AB##Radius(p.m_##ab##AA, p.m_##ab##BB);         \
        } else if (newCssValue.valueKind() ==                                 \
                   CSSStyleValuePair::ValueKind::ValueListKind) {             \
            const auto& vl = newCssValue.multiValue();                        \
            STARFISH_ASSERT(vl->size() == 1 || vl->size() == 2);              \
            Length v1;                                                        \
            Length v2;                                                        \
            const auto& v = vl->at(0);                                        \
            if (v.valueKind() == CSSStyleValuePair::ValueKind::Initial) {     \
                v1 = v2 = Length(Length::Fixed, 0);                           \
            } else if (v.valueKind() ==                                       \
                       CSSStyleValuePair::ValueKind::Inherit) {               \
                v1 = parentStyle->borderRadius().m_##ab##AA;                  \
                v2 = parentStyle->borderRadius().m_##ab##BB;                  \
            } else {                                                          \
                v1 = v2 = v.toLengthValue();                                  \
            }                                                                 \
            if (vl->size() == 2) {                                            \
                const auto& v = vl->at(1);                                    \
                if (v.valueKind() == CSSStyleValuePair::ValueKind::Initial) { \
                    v2 = Length(Length::Fixed, 0);                            \
                } else if (v.valueKind() ==                                   \
                           CSSStyleValuePair::ValueKind::Inherit) {           \
                    v2 = parentStyle->borderRadius().m_##ab##BB;              \
                } else {                                                      \
                    v2 = v.toLengthValue();                                   \
                }                                                             \
            }                                                                 \
            style->setBorder##AB##Radius(v1, v2);                             \
        } else {                                                              \
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();                     \
        }                                                                     \
        break;
        BORDER_RADIUS_APPLY(TopLeft, topLeft, Horizontal, Vertical)
        BORDER_RADIUS_APPLY(TopRight, topRight, Horizontal, Vertical)
        BORDER_RADIUS_APPLY(BottomRight, bottomRight, Horizontal, Vertical)
        BORDER_RADIUS_APPLY(BottomLeft, bottomLeft, Horizontal, Vertical)
    case CSSStyleValuePair::KeyKind::Clip:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::RectValueKind) {
            style->setClip(newCssValue.clip());
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Unset) {
            style->setClip(nullptr);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setClip(parentStyle->clip());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Auto) {
            style->setClip(nullptr);
        }
        break;
    case CSSStyleValuePair::KeyKind::ClipPath:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::UrlValueKind) {
            style->setClipPath(newCssValue.urlStringValue());
        } else if (newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Initial ||
                   newCssValue.valueKind() ==
                       CSSStyleValuePair::ValueKind::Unset) {
            style->setClipPath(String::emptyString);
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Inherit) {
            style->setClipPath(String::emptyString);
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Auto) {
            style->setClipPath(String::emptyString);
        }
        break;
    case CSSStyleValuePair::KeyKind::ListStyleType:
        if (newCssValue.valueKind() == CSSStyleValuePair::Inherit ||
            newCssValue.valueKind() == CSSStyleValuePair::Unset) {
            style->setListStyleType(parentStyle->listStyleData().typeData());
        } else if (newCssValue.valueKind() == CSSStyleValuePair::None) {
            style->setListStyleType(CounterStyle::getNoneCounter());
        } else if (newCssValue.valueKind() == CSSStyleValuePair::Initial) {
            style->setListStyleType(CounterStyle::getDiscCounter());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::AtomicStringValueKind) {
            const AtomicString& counterName = newCssValue.atomicStringValue();
            auto counterStyle =
                CounterStyle::getKnownCounter(counterName.string());
            if (counterStyle) {
                style->setListStyleType(counterStyle);
            } else {
                style->setListStyleType(CounterStyle::getDecimalCounter());
            }
        } else {
            style->setListStyleType(newCssValue.stringValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::ListStyleImage:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset) {
            style->setListStyleImage(parentStyle->listStyleData().image());
        } else if ((newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::Initial) ||
                   (newCssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::None)) {
            style->setListStyleImage(String::emptyString);
        } else {
            style->setListStyleImage(newCssValue.urlValue(origin));
        }
        break;
    case CSSStyleValuePair::KeyKind::ListStylePosition:
        if (newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Inherit ||
            newCssValue.valueKind() == CSSStyleValuePair::ValueKind::Unset) {
            style->setListStylePosition(parentStyle->listStylePosition());
        } else if (newCssValue.valueKind() ==
                   CSSStyleValuePair::ValueKind::Initial) {
            style->setListStylePosition(
                ListStylePositionValue::ListStylePositionOutside);
        } else {
            style->setListStylePosition(newCssValue.listStylePositionValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::UserSelect:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            style->setUserSelect(parentStyle->userSelect());
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Unset:
        case CSSStyleValuePair::ValueKind::Auto:
            style->setUserSelect(UserSelectValue::NoneUserSelectValue);
            break;
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::UserSelectValueKind ==
                            newCssValue.valueKind());
            style->setUserSelect(newCssValue.userSelectValue());
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::Hyphens:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setHyphens(parentStyle->hyphens());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Auto:
            style->setHyphens(HyphensValue::NoneHyphensValue);
            break;
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::HyphensValueKind ==
                            newCssValue.valueKind());
            style->setHyphens(newCssValue.hyphensValue());
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::LineBreak:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setLineBreak(parentStyle->lineBreak());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Auto:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setLineBreak(LineBreakValue::NormalLineBreakValue);
            break;
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::LineBreakValueKind ==
                            newCssValue.valueKind());
            style->setLineBreak(newCssValue.lineBreakValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::WordBreak:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setWordBreak(parentStyle->wordBreak());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->setWordBreak(WordBreakValue::NormalWordBreakValue);
            break;
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::WordBreakValueKind ==
                            newCssValue.valueKind());
            style->setWordBreak(newCssValue.wordBreakValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::Appearance:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setAppearance(parentStyle->appearance());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->setAppearance(AppearanceValue::AutoAppearanceValue);
            break;
        default:
            STARFISH_ASSERT(CSSStyleValuePair::ValueKind::AppearanceValueKind ==
                            newCssValue.valueKind());
            style->setAppearance(newCssValue.appearanceValue());
        }
        break;
    case CSSStyleValuePair::KeyKind::GridTemplateColumns:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::GridTemplateUnits) {
            style->setGridTemplateColumns(newCssValue.gridTemplateUnits());
        }
        break;
    case CSSStyleValuePair::KeyKind::GridTemplateRows:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::GridTemplateUnits) {
            style->setGridTemplateRows(newCssValue.gridTemplateUnits());
        }
        break;
    case CSSStyleValuePair::KeyKind::CaretColor:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setCaretColor(parentStyle->caretColor());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->setCaretColor(Unit::Color(0, 0, 0, 255));
            break;
        case CSSStyleValuePair::ValueKind::ColorValueKind:
            style->setCaretColor(newCssValue.colorValue());
            break;
        default:
            STARFISH_ASSERT(newCssValue.valueKind() ==
                            CSSStyleValuePair::ValueKind::NamedColorValueKind);
            if (newCssValue.namedColorValue() ==
                NamedColor::NamedColorValue::currentColor) {
                style->ensureInheritedRareData()->m_caretColor =
                    parentStyle->ensureInheritedRareData()->m_caretColor;
            } else {
                style->setCaretColor(NamedColor::namedColorToColor(
                    newCssValue.namedColorValue()));
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::CounterReset:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setCounterReset(parentStyle->counterReset());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::None:
            style->setCounterReset(nullptr);
            break;
        default:
            ValueList* list = newCssValue.multiValue();
            size_t size = list->size();
            STARFISH_ASSERT(size % 2 == 0);
            for (size_t i = 0; i < size; i += 2) {
                style->setCounterResetItem(list->at(i).atomicStringValue(),
                                           list->at(i + 1).int32Value());
            }
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::CounterIncrement:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setCounterIncrement(parentStyle->counterIncrement());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::None:
            style->setCounterIncrement(nullptr);
            break;
        default:
            ValueList* list = newCssValue.multiValue();
            size_t size = list->size();
            STARFISH_ASSERT(size % 2 == 0);
            for (size_t i = 0; i < size; i += 2) {
                style->setCounterIncrementItem(list->at(i).atomicStringValue(),
                                               list->at(i + 1).int32Value());
            }
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::GridColumnStart:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::StringValueKind:
            style->setGridColumnStart(newCssValue.stringValue());
            break;
        default:
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::GridColumnEnd:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::StringValueKind:
            style->setGridColumnEnd(newCssValue.stringValue());
            break;
        default:
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::GridRowStart:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::StringValueKind:
            style->setGridRowStart(newCssValue.stringValue());
            break;
        default:
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::GridRowEnd:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::StringValueKind:
            style->setGridRowEnd(newCssValue.stringValue());
            break;
        default:
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::GridGap:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::ValueListKind) {
            ValueList* list = newCssValue.multiValue();
            if (list->size() == 1) {
                CSSLength length = list->at(0).cssLengthValue();
                style->setRowGap(length.toLength());
                style->setColumnGap(length.toLength());
            } else if (list->size() == 2) {
                CSSLength row = list->at(0).cssLengthValue();
                CSSLength column = list->at(1).cssLengthValue();

                style->setRowGap(row.toLength());
                style->setColumnGap(column.toLength());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::GridTemplateAreas:
        style->resetGridTemplateAreas();
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::GridTemplateAreasValueKind) {
            style->setGridTemplateAreas(newCssValue.gridTemplateAreas());
        }
        break;
    case CSSStyleValuePair::KeyKind::GridArea:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::ValueListKind) {
            auto list = newCssValue.multiValue();
            if (list) {
                style->setGridRowStart(list->at(0).stringValue());
                style->setGridColumnStart(list->at(1).stringValue());
                style->setGridRowEnd(list->at(2).stringValue());
                style->setGridColumnEnd(list->at(3).stringValue());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::GridRow:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::ValueListKind) {
            auto list = newCssValue.multiValue();
            if (list) {
                style->setGridRowStart(list->at(0).stringValue());
                style->setGridRowEnd(list->at(1).stringValue());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::GridColumn:
        if (newCssValue.valueKind() ==
            CSSStyleValuePair::ValueKind::ValueListKind) {
            auto list = newCssValue.multiValue();
            if (list) {
                style->setGridColumnStart(list->at(0).stringValue());
                style->setGridColumnEnd(list->at(1).stringValue());
            }
        }
        break;
    case CSSStyleValuePair::KeyKind::WillChange:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setWillChange(parentStyle->willChange());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::Auto:
            style->setWillChange(nullptr);
            break;
        default:
            ValueList* list = newCssValue.multiValue();
            size_t size = list->size();
            auto willChangeData = new WillChangeData();
            for (size_t i = 0; i < size; i++) {
                const AtomicString& item = list->at(i).atomicStringValue();
                if (item.string()->equals("contents")) {
                    willChangeData->setContents();
                    continue;
                }
                if (item.string()->equals("scroll-position")) {
                    willChangeData->setScrollPosition();
                    continue;
                }

                if (item.string()->equals("transform")) {
                    willChangeData->setTransform();
                }

                if (item.string()->equals("opacity")) {
                    willChangeData->setOpacity();
                }

                willChangeData->push_back(item);
            }
            style->setWillChange(willChangeData);
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::BoxDecorationBreak:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
        case CSSStyleValuePair::ValueKind::Unset:
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setBoxDecorationBreak(parentStyle->boxDecorationBreak());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
            style->setBoxDecorationBreak(SliceBoxDecorationBreakValue);
            break;
        default:
            style->setBoxDecorationBreak(newCssValue.boxDecorationBreakValue());
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::Filter:
        switch (newCssValue.valueKind()) {
        case CSSStyleValuePair::ValueKind::Inherit:
            MARK_SOME_NONE_INHERIT_MEMBER_EXPLICITLY_INHERITED();
            style->setFilter(parentStyle->filter());
            break;
        case CSSStyleValuePair::ValueKind::Initial:
        case CSSStyleValuePair::ValueKind::None:
        case CSSStyleValuePair::ValueKind::Unset:
            style->setFilter(nullptr);
            break;
        case CSSStyleValuePair::ValueKind::ValueListKind:
            style->setFilter(FilterFunctions::create(newCssValue));
            break;
        default:
            break;
        }
        break;
    case CSSStyleValuePair::KeyKind::Unknown:
        break;
    default:
        break;
    }
}

void StyleResolver::collectMatchingRulesFromAuthorSheet(
    StyleResolveContext& ctx,
    const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& begin,
    const GCVector<std::pair<StyleRule*, ResourceURL*>>::iterator& end,
    CSSSelector::Type type, Element* element, AtomicString elementName,
    AtomicString elementId,
    const GCAtomicTightVector<AtomicString>& elementClasses,
    MatchedStyleRules<32>& authorRules, ComputedStyle* ret,
    PseudoElementType pseudoElementType)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(ret != nullptr);

    bool canUseAncestorSelectorFilter =
        ctx.m_ancestorSelectorFilter->canUseAncestorSelectorFilter(element);

    for (auto it = begin; it != end; ++it) {
        StyleRule* rule = it->first;
        ResourceURL* url = it->second;

        if (pseudoElementType == PseudoElementType::PseudoElementNone) {
            if (type == CSSSelector::Type::Id && rule->isSimpleIDSelector()) {
                authorRules.push_back(std::make_pair(rule, url));
                continue;
            }
            if (type == CSSSelector::Type::Class &&
                rule->isSimpleClassSelector()) {
                authorRules.push_back(std::make_pair(rule, url));
                continue;
            }
            if (type == CSSSelector::Type::Tag && rule->isSimpleTagSelector()) {
                authorRules.push_back(std::make_pair(rule, url));
                continue;
            }
        }

        if (canUseAncestorSelectorFilter &&
            ctx.m_ancestorSelectorFilter->canIgnoreSelector(rule, element)) {
            const CSSSelectorList& selectorList = rule->selectorList();

            for (size_t i = 1; i < selectorList.size(); i++) {
                if (selectorList[i].m_selector->type() == CSSSelector::Class) {
                    ret->setStyleDamageSource(
                        StyleResolver::StyleDamageSource::StyleDamageFromClass);
                } else if (selectorList[i].m_selector->type() ==
                           CSSSelector::Id) {
                    ret->setStyleDamageSource(
                        StyleResolver::StyleDamageSource::StyleDamageFromID);
                }
            }
            continue;
        }

        const CSSSelectorList& selectorList = rule->selectorList();

        MatchResult result(nullptr);
        if (matchSelector(element, elementName, elementId, elementClasses,
                          selectorList, 0, result) == Match::SelectorMatches) {
            if (result.pseudoType != PseudoElementType::PseudoElementNone) {
                if (result.pseudoType == pseudoElementType) {
                    ret->setPseudoType(pseudoElementType);
                    authorRules.push_back(std::make_pair(rule, url));
                } else {
                    if (result.pseudoType ==
                        PseudoElementType::PseudoElementFirstLine) {
                        ret->m_seenPseudoElementFirstLine = true;
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementFirstLetter) {
                        ret->m_seenPseudoElementFirstLetter = true;
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementBefore) {
                        ret->m_seenPseudoElementBefore |=
                            rule->styleDeclaration()->hasCSSValuePair(
                                CSSStyleValuePair::KeyKind::Content);
                    } else if (result.pseudoType ==
                               PseudoElementType::PseudoElementAfter) {
                        ret->m_seenPseudoElementAfter |=
                            rule->styleDeclaration()->hasCSSValuePair(
                                CSSStyleValuePair::KeyKind::Content);
                    }
                }
            } else if (pseudoElementType ==
                       PseudoElementType::PseudoElementNone) {
                authorRules.push_back(std::make_pair(rule, url));
            }
        }

        if (result.seenCombinator) {
            ret->setStyleDamageSource(result.styleDamageFrom);
        } else {
            ret->setStyleDamageSource((StyleResolver::StyleDamageSource)(
                result.styleDamageFrom &
                ~StyleResolver::StyleDamageSource::StyleDamageFromDOMTree));
        }
        ret->setStyleDamageSourceNodeStateMap(
            result.styleDamageSourceNodeStateMap);
        ret->setStyleDamageSourceNodeStateDOMTreeMap(
            result.styleDamageSourceNodeStateDOMTreeMap);
    }
}

static bool comparingRules(const std::pair<StyleRule*, ResourceURL*>& r1,
                           const std::pair<StyleRule*, ResourceURL*>& r2)
{
    const auto& a = r1.first;
    const auto& b = r2.first;

    bool aUA = a->isUARule();
    bool aUB = b->isUARule();

    if (aUA && aUB) {
    } else if (aUA) {
        return true;
    } else if (aUB) {
        return false;
    }

    unsigned int specificityA = a->selectorList().specificity();
    unsigned int specificityB = b->selectorList().specificity();
    if (specificityA == specificityB) {
        return a->order() < b->order();
    }
    return specificityA < specificityB;
}

template <typename Iter, typename Func>
void sortVector(Iter begin, Iter end, Func matchingRule)
{
    std::stable_sort(begin, end, matchingRule);
}

void StyleResolver::matchAllRules(StyleResolveContext& ctx, Element* element,
                                  ComputedStyle* ret, ComputedStyle* parent,
                                  PseudoElementType pseudoElementType)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(ret != nullptr);
    STARFISH_ASSERT(parent != nullptr);

    AtomicString elementName = element->name().localNameAtomic();
    AtomicString elementId = element->atomicId();
    const GCAtomicTightVector<AtomicString>& elementClasses =
        element->classNames();

    const size_t matchedRulesInlineStorageSize = 32;
    MatchedStyleRules<matchedRulesInlineStorageSize> matchedRules;

    if (element->hasId()) {
        auto& rules = m_ruleSet->idRules();
        auto iter = rules.find(elementId);
        if (iter != rules.end()) {
            collectMatchingRulesFromAuthorSheet(
                ctx, iter.value().begin(), iter.value().end(),
                CSSSelector::Type::Id, element, elementName, elementId,
                elementClasses, matchedRules, ret, pseudoElementType);
        }
    }

    if (element->hasClass()) {
        auto& rules = m_ruleSet->classRules();
        size_t classLen = elementClasses.size();
        for (unsigned k = 0; k < classLen; k++) {
            auto iter = rules.find(elementClasses[k]);
            if (iter != rules.end()) {
                collectMatchingRulesFromAuthorSheet(
                    ctx, iter.value().begin(), iter.value().end(),
                    CSSSelector::Type::Class, element, elementName, elementId,
                    elementClasses, matchedRules, ret, pseudoElementType);
            }
        }
    }

    {
        auto& rules = m_ruleSet->tagRules();
        auto iter = rules.find(elementName);
        if (iter != rules.end()) {
            collectMatchingRulesFromAuthorSheet(
                ctx, iter.value().begin(), iter.value().end(),
                CSSSelector::Type::Tag, element, elementName, elementId,
                elementClasses, matchedRules, ret, pseudoElementType);
        }
    }

    {
        auto& rules = m_ruleSet->universalRules();
        collectMatchingRulesFromAuthorSheet(
            ctx, rules.begin(), rules.end(), CSSSelector::Type::UnKnown,
            element, elementName, elementId, elementClasses, matchedRules, ret,
            pseudoElementType);
    }

    auto begin = &matchedRules[0];
    auto end = matchedRules.data() + matchedRules.size();
    auto authorSheetBegin = end;

    sortVector(begin, end, comparingRules);

    {
        auto iter = begin;
        while (iter != end) {
            if (!iter->first->isUARule()) {
                authorSheetBegin = iter;
                break;
            }
            iter++;
        }
    }

    // Gather all css custom properties
    {
        auto iter = begin;
        while (iter != end) {
            const auto& propertiesList =
                iter->first->styleDeclaration()->cssCustomValues();
            if (propertiesList) {
                const auto& properties = propertiesList->values();
                auto list =
                    ret->rareComputedStyleData()->ensureCustomProperty();
                for (size_t i = 0; i < properties.size(); ++i) {
                    list->setProperty(properties[i].name(),
                                      properties[i].value());
                }
            }
            iter++;
        }

        if (pseudoElementType == PseudoElementNone &&
            element->inlineStyleWithoutCreation()) {
            auto propertiesList =
                element->inlineStyleWithoutCreation()->cssCustomValues();
            if (propertiesList) {
                const auto& properties = propertiesList->values();
                auto list =
                    ret->rareComputedStyleData()->ensureCustomProperty();
                for (size_t i = 0; i < properties.size(); ++i) {
                    list->setProperty(properties[i].name(),
                                      properties[i].value());
                }
            }
        }
    }

    {
        auto list = ret->customProperty();
        if (list) {
            m_cssCustomValues = list.value();
        }
    }

    // Apply ua-rules
    // We disallow ua !important rules due to performance now
    {
        auto iter = begin;
        while (iter != authorSheetBegin) {
            apply(element, iter->first->styleDeclaration()->m_cssValues,
                  iter->second, ret, parent, false);
            iter++;
        }
    }

    // Apply presentation attribute's style
    CSSStyleValuePairVectorHolder cssValues;
    if (element->isSVGElement()) {
        element->styleForPresentationAttribute(cssValues, cssCustomValues());
    } else {
        element->styleForPresentationAttribute(cssValues);
    }
    apply(element, cssValues.mutableData(), element->document()->documentURI(),
          ret, parent, false);

    // Apply non-important author-rules
    {
        auto iter = authorSheetBegin;
        while (iter != end) {
            apply(element, iter->first->styleDeclaration()->m_cssValues,
                  iter->second, ret, parent, false);
            iter++;
        }
    }

    // inline style
    if (pseudoElementType == PseudoElementNone &&
        element->inlineStyleWithoutCreation()) {
        apply(element, element->inlineStyleWithoutCreation()->m_cssValues,
              element->document()->baseURL(), ret, parent, false);
    }

    // Apply important author-rules
    {
        auto iter = authorSheetBegin;
        while (iter != end) {
            apply(element, iter->first->styleDeclaration()->m_cssValues,
                  iter->second, ret, parent, true);
            iter++;
        }
    }

    // inline style
    if (pseudoElementType == PseudoElementNone &&
        element->inlineStyleWithoutCreation()) {
        apply(element, element->inlineStyleWithoutCreation()->m_cssValues,
              element->document()->baseURL(), ret, parent, true);
    }
}

StyleResolver::Match StyleResolver::matchSelector(
    Element* element, AtomicString elementName, AtomicString elementId,
    const GCAtomicTightVector<AtomicString>& elementClasses,
    const CSSSelectorList& selectorList, unsigned idx, MatchResult& result,
    bool isQueryingSelector)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(idx < selectorList.size());

    CSSSelector* selector = selectorList[idx].m_selector;
    if (!checkOne(element, elementName, elementId, elementClasses, selector,
                  result, isQueryingSelector)) {
        return Match::SelectorFailsLocally;
    }

    auto relation = selectorList[idx].m_relation;
    if (selectorList[idx].m_relation == CSSSelectorListItem::None) {
        return Match::SelectorMatches;
    }

    Match match;
    if (relation == CSSSelectorListItem::RelationType::SubSelector) {
        match = matchSelector(element, elementName, elementId, elementClasses,
                              selectorList, ++idx, result, isQueryingSelector);
    } else {
        result.seenCombinator = true;
        match =
            matchForRelation(element, elementName, elementId, elementClasses,
                             selectorList, relation, ++idx, result);
    }
    return match;
}

StyleResolver::Match StyleResolver::matchForRelation(
    Element* element, AtomicString elementName, AtomicString elementId,
    const GCAtomicTightVector<AtomicString>& elementClasses,
    const CSSSelectorList& selectorList,
    CSSSelectorListItem::RelationType relation, unsigned idx,
    MatchResult& result)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(idx < selectorList.size());

    CSSSelector* selector = selectorList[idx].m_selector;
    STARFISH_ASSERT(selector != nullptr);

    std::function<Element*(Element*)> nextParentElement =
        [](Element* element) -> Element* { return element->parentElement(); };
    if (UNLIKELY(selector->isPseudoClassHostFamilySelector())) {
        nextParentElement = [](Element* element) -> Element* {
            if (element->isShadowRootHost()) {
                return nullptr;
            }
            return element->renderingParentElement();
        };
    }

    switch (relation) {
    case CSSSelectorListItem::RelationType::Descendant: {
        Element* parent = nextParentElement(element);
        while (parent) {
            AtomicString elementName = parent->name().localNameAtomic();
            AtomicString elementId = parent->atomicId();
            const GCAtomicTightVector<AtomicString>& elementClasses =
                parent->classNames();
            if (matchSelector(parent, elementName, elementId, elementClasses,
                              selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            }
            parent = nextParentElement(parent);
        }

        return Match::SelectorFailsCompletely;
    }
    case CSSSelectorListItem::RelationType::Child: {
        Element* parent = nextParentElement(element);
        if (parent) {
            AtomicString elementName = parent->name().localNameAtomic();
            AtomicString elementId = parent->atomicId();
            const GCAtomicTightVector<AtomicString>& elementClasses =
                parent->classNames();
            if (matchSelector(parent, elementName, elementId, elementClasses,
                              selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            }
            return Match::SelectorFailsCompletely;
        } else {
            return Match::SelectorFailsCompletely;
        }
    }
    case CSSSelectorListItem::RelationType::AdjacentSibling: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        Element* previousSibling = element->previousElementSibling();
        if (previousSibling) {
            AtomicString elementName =
                previousSibling->name().localNameAtomic();
            AtomicString elementId = previousSibling->atomicId();
            const GCAtomicTightVector<AtomicString>& elementClasses =
                previousSibling->classNames();
            if (matchSelector(previousSibling, elementName, elementId,
                              elementClasses, selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            } else {
                return Match::SelectorFailsCompletely;
            }
        } else {
            return Match::SelectorFailsCompletely;
        }
    }
    case CSSSelectorListItem::RelationType::GeneralSibling: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        Element* previousSibling = element->previousElementSibling();
        while (previousSibling) {
            AtomicString elementName =
                previousSibling->name().localNameAtomic();
            AtomicString elementId = previousSibling->atomicId();
            const GCAtomicTightVector<AtomicString>& elementClasses =
                previousSibling->classNames();
            if (matchSelector(previousSibling, elementName, elementId,
                              elementClasses, selectorList, idx,
                              result) == Match::SelectorMatches) {
                return Match::SelectorMatches;
            }
            previousSibling = previousSibling->previousElementSibling();
        }
        return Match::SelectorFailsCompletely;
    }
    default:
        return Match::SelectorFailsCompletely;
    }
}

bool StyleResolver::checkOne(
    Element* element, AtomicString elementName, AtomicString elementId,
    const GCAtomicTightVector<AtomicString>& elementClasses,
    CSSSelector* selector, MatchResult& result, bool isQueryingSelector)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(selector != nullptr);

    auto selectorType = selector->type();
    if (selectorType == CSSSelector::Type::Class) {
        result.styleDamageFrom =
            (StyleDamageSource)(result.styleDamageFrom | StyleDamageFromClass);
        auto txt = selector->selectorText();
        size_t len = elementClasses.size();
        for (unsigned i = 0; i < len; i++) {
            if (txt == elementClasses[i]) {
                return true;
            }
        }
        return false;
    } else if (selectorType == CSSSelector::Type::Tag) {
        return (elementName == selector->selectorText());
    } else if (selectorType == CSSSelector::Type::Id) {
        STARFISH_ASSERT(!selector->selectorText().isEmptyAtomicString());
        result.styleDamageFrom =
            (StyleDamageSource)(result.styleDamageFrom | StyleDamageFromID);
        return elementId == selector->selectorText();
    } else {
        switch (selectorType) {
        case CSSSelector::Type::Universal:
            return true;
        case CSSSelector::AttributeExact:   // Example: E[foo="bar"]
        case CSSSelector::AttributeSet:     // Example: E[foo]
        case CSSSelector::AttributeHyphen:  // Example: E[foo|="bar"]
        case CSSSelector::AttributeList:    // Example: E[foo~="bar"]
        case CSSSelector::AttributeContain: // css3: E[foo*="bar"]
        case CSSSelector::AttributeBegin:   // css3: E[foo^="bar"]
        case CSSSelector::AttributeEnd:     // css3: E[foo$="bar"]
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromAttribute);
            return anyAttributeMatches(element, selector->type(),
                                       selector->asCSSAttributeSelector(),
                                       result);
        case CSSSelector::Type::PseudoClass:
            return checkPseudoClass(element, selector->asCSSPseudoSelector(),
                                    result);
        case CSSSelector::Type::PseudoElement:
            // while the use of pseudo-elements in selectors of querySelector is
            // permitted, they will not match any elements in the document, and
            // thus
            // would not result in any elements being returned.
            return isQueryingSelector
                       ? false
                       : checkPseudoElement(
                             element, selector->asCSSPseudoSelector(), result);
        default:
            return false;
        }
    }
}

static bool isFirstChild(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    return element->parentElement() ? !element->previousElementSibling()
                                    : false;
}

static bool isLastChild(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    return element->parentElement() ? !element->nextElementSibling() : false;
}

static bool isFirstOfType(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    Node* sibling = element->previousSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->name().localName()->equalsIgnoreCase(
                element->name().localName())) {
            return false;
        }
        sibling = sibling->previousSibling();
    }

    return true;
}

static bool isLastOfType(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    Node* sibling = element->nextSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->name().localName()->equalsIgnoreCase(
                element->name().localName())) {
            return false;
        }
        sibling = sibling->nextSibling();
    }

    return true;
}

static bool isEmpty(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    Node* child = element->firstChild();
    while (child) {
        if (child->isElement()) {
            return false;
        } else if (child->isText()) {
            STARFISH_ASSERT(child->asText()->textContent().hasValue());
            if (child->textContent().getValue()->length() > 0) {
                return false;
            }
        }

        child = child->nextSibling();
    }

    return true;
}

static unsigned nthChildIndex(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    unsigned index = 1;

    Node* sibling = element->previousSibling();
    while (sibling) {
        if (sibling->isElement()) {
            ++index;
        }
        sibling = sibling->previousSibling();
    }

    return index;
}

static unsigned nthOfTypeIndex(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    unsigned index = 1;
    String* tag = element->tagName();

    Node* sibling = element->previousSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->tagName()->equals(tag)) {
            ++index;
        }
        sibling = sibling->previousSibling();
    }

    return index;
}

static unsigned nthLastChildIndex(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    unsigned index = 1;

    Node* sibling = element->nextSibling();
    while (sibling) {
        if (sibling->isElement()) {
            ++index;
        }
        sibling = sibling->nextSibling();
    }

    return index;
}

static unsigned nthLastOfTypeIndex(Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    int index = 1;
    String* tag = element->tagName();

    Node* sibling = element->nextSibling();
    while (sibling) {
        if (sibling->isElement() &&
            sibling->asElement()->tagName()->equals(tag)) {
            ++index;
        }
        sibling = sibling->nextSibling();
    }

    return index;
}

bool StyleResolver::checkPseudoClass(Element* element,
                                     CSSPseudoSelector* selector,
                                     MatchResult& result)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(selector != nullptr);

    switch (selector->pseudoType()) {
    case CSSSelector::PseudoType::PseudoHover:
        if (result.seenCombinator) {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementStateDOMTree);
            result.styleDamageSourceNodeStateDOMTreeMap =
                result.styleDamageSourceNodeStateDOMTreeMap |
                Node::NodeStateHovered;
        } else {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementState);
            result.styleDamageSourceNodeStateMap =
                result.styleDamageSourceNodeStateMap | Node::NodeStateHovered;
        }
        return element->state() & Node::NodeState::NodeStateHovered;
    case CSSSelector::PseudoType::PseudoActive:
        if (result.seenCombinator) {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementStateDOMTree);
            result.styleDamageSourceNodeStateDOMTreeMap =
                result.styleDamageSourceNodeStateDOMTreeMap |
                Node::NodeStateActive;
        } else {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementState);
            result.styleDamageSourceNodeStateMap =
                result.styleDamageSourceNodeStateMap | Node::NodeStateActive;
        }
        return element->state() & Node::NodeState::NodeStateActive;
    case CSSSelector::PseudoType::PseudoFocus:
        if (result.seenCombinator) {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementStateDOMTree);
            result.styleDamageSourceNodeStateDOMTreeMap =
                result.styleDamageSourceNodeStateDOMTreeMap |
                Node::NodeStateFocused;
        } else {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementState);
            result.styleDamageSourceNodeStateMap =
                result.styleDamageSourceNodeStateMap | Node::NodeStateFocused;
        }
        return element->state() & Node::NodeState::NodeStateFocused;
    case CSSSelector::PseudoType::PseudoTarget:
        if (result.seenCombinator) {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementStateDOMTree);
            result.styleDamageSourceNodeStateDOMTreeMap =
                result.styleDamageSourceNodeStateDOMTreeMap |
                Node::NodeStateTarget;
        } else {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementState);
            result.styleDamageSourceNodeStateMap =
                result.styleDamageSourceNodeStateMap | Node::NodeStateTarget;
        }
        return element->state() & Node::NodeState::NodeStateTarget;
    case CSSSelector::PseudoType::PseudoLink:
        if (result.seenCombinator) {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementStateDOMTree);
            result.styleDamageSourceNodeStateDOMTreeMap =
                result.styleDamageSourceNodeStateDOMTreeMap |
                Node::NodeStateLink;
        } else {
            result.styleDamageFrom = (StyleDamageSource)(
                result.styleDamageFrom | StyleDamageFromElementState);
            result.styleDamageSourceNodeStateMap =
                result.styleDamageSourceNodeStateMap | Node::NodeStateLink;
        }
        return element->state() & Node::NodeState::NodeStateLink;
    case CSSSelector::PseudoType::PseudoRoot:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        return element == element->document()->documentElement();
    case CSSSelector::PseudoType::PseudoScope: {
        Node* scope = result.scope ? result.scope.getValue()
                                   : element->document()->documentElement();
        return element == scope;
    }
    case CSSSelector::PseudoType::PseudoFirstChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return isFirstChild(element);
    case CSSSelector::PseudoType::PseudoLastChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return isLastChild(element);
    case CSSSelector::PseudoType::PseudoFirstOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() && isFirstOfType(element);
    case CSSSelector::PseudoType::PseudoLastOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() && isLastOfType(element);
    case CSSSelector::PseudoType::PseudoOnlyChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return isFirstChild(element) && isLastChild(element);
    case CSSSelector::PseudoType::PseudoOnlyOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() && isFirstOfType(element) &&
               isLastOfType(element);
    case CSSSelector::PseudoType::PseudoPlaceholderShown:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        return element->isHTMLElement() && element->isHTMLFormControl() &&
               element->asHTMLFormControl()->isPlaceholderVisible();
    case CSSSelector::PseudoType::PseudoEmpty:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return isEmpty(element);
    case CSSSelector::PseudoType::PseudoNthChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() &&
               selector->matchNth(nthChildIndex(element));
    case CSSSelector::PseudoType::PseudoNthOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() &&
               selector->matchNth(nthOfTypeIndex(element));
    case CSSSelector::PseudoType::PseudoNthLastChild:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() &&
               selector->matchNth(nthLastChildIndex(element));
    case CSSSelector::PseudoType::PseudoNthLastOfType:
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromDOMTree);
        result.seenCombinator = true;
        return element->parentElement() &&
               selector->matchNth(nthLastOfTypeIndex(element));
    case CSSSelector::PseudoType::PseudoDir: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        String* value = element->getDir();
        String* argument = selector->argument();
        if (value->equals(argument) ||
            (value->equals(String::emptyString) && argument->equals("ltr"))) {
            return true;
        }
        return false;
    }
    case CSSSelector::PseudoType::PseudoLang: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        String* value = element->getLaunguage();
        String* argument = selector->argument();

        if (value->equals(String::emptyString) ||
            !value->startsWith(argument, false)) {
            break;
        }
        if (value->length() != argument->length() &&
            value->charAt(argument->length()) != '-') {
            break;
        }

        return true;
    }
    case CSSSelector::PseudoType::PseudoNot: {
        STARFISH_ASSERT(selector->pseudoSelectorList().size() == 1);
        result.styleDamageFrom = StyleDamageFromAll;
        AtomicString elementName = element->name().localNameAtomic();
        AtomicString elementId = element->atomicId();
        const GCAtomicTightVector<AtomicString>& elementClasses =
            element->classNames();
        return !checkOne(element, elementName, elementId, elementClasses,
                         selector->pseudoSelectorList()[0].m_selector, result);
    }
    case CSSSelector::PseudoType::PseudoDefined: {
        if (element->isHTMLElement() && !element->isHTMLUnknownElement()) {
            return true;
        }
        return false;
    }
    case CSSSelector::PseudoType::PseudoEnabled: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        if (element->isHTMLAnchorElement()) {
            if (element->getAttribute(starfish()->staticStrings()->m_href)
                    .hasValue()) {
                return true;
            }
        } else if (element->isHTMLElement()) {
            if (!element->asHTMLElement()->disabled()) {
                return true;
            }
        }
        return false;
    }
    case CSSSelector::PseudoType::PseudoDisabled: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        if (element->isHTMLElement()) {
            if (element->asHTMLElement()->disabled()) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }
    case CSSSelector::PseudoType::PseudoChecked: {
        result.styleDamageFrom = (StyleDamageSource)(result.styleDamageFrom |
                                                     StyleDamageFromAttribute);
        if (element->isHTMLElement()) {
            if (element->isHTMLInputElement()) {
                if (element->asHTMLInputElement()->type()->equals("radio") ||
                    element->asHTMLInputElement()->type()->equals("checkbox")) {
                    if (element->asHTMLInputElement()->checked()) {
                        return true;
                    }
                }
            } else if (element->isHTMLOptionElement()) {
                if (element->asHTMLOptionElement()->selectedness()) {
                    return true;
                }
            }
        }
        return false;
    }
    case CSSSelector::PseudoType::PseudoHost: {
        return element->isShadowRootHost();
    }
    case CSSSelector::PseudoType::PseudoHostFunction: {
        if (!element->isShadowRootHost()) {
            return false;
        }

        STARFISH_ASSERT(selector->pseudoSelectorList().size() == 1);
        result.styleDamageFrom = StyleDamageFromAll;
        AtomicString elementName = element->name().localNameAtomic();
        AtomicString elementId = element->atomicId();
        const GCAtomicTightVector<AtomicString>& elementClasses =
            element->classNames();
        return checkOne(element, elementName, elementId, elementClasses,
                        selector->pseudoSelectorList()[0].m_selector, result);
    }
    default:
#ifdef STARFISH_ENABLE_TEST
    {
        // Code for reducing logging `STARFISH_UNIMPLEMENTED`
        static bool pseudoLogMap[CSSSelector::PseudoTotalCount];
        if (pseudoLogMap[selector->pseudoType()]) {
            return false;
        }
        pseudoLogMap[selector->pseudoType()] = true;
    }
#endif
        STARFISH_UNSUPPORTED("css pseudo-element: %d", selector->pseudoType());
        break;
    }
    return false;
}

bool StyleResolver::checkPseudoElement(Element* element,
                                       CSSPseudoSelector* selector,
                                       MatchResult& result)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(selector != nullptr);

    switch (selector->pseudoType()) {
    case CSSSelector::PseudoType::PseudoFirstLine:
        result.pseudoType = PseudoElementType::PseudoElementFirstLine;
        m_usesFirstLineRule = true;
        return true;
    case CSSSelector::PseudoType::PseudoFirstLetter:
        result.pseudoType = PseudoElementType::PseudoElementFirstLetter;
        return true;
    case CSSSelector::PseudoType::PseudoBefore:
        result.pseudoType = PseudoElementType::PseudoElementBefore;
        return true;
    case CSSSelector::PseudoType::PseudoAfter:
        result.pseudoType = PseudoElementType::PseudoElementAfter;
        return true;
    default:
        STARFISH_UNSUPPORTED("css pseudo-element: %d", selector->pseudoType());
        return false;
    }
}

// Find keyframes that are matched with name in rule set.
static StyleRuleKeyframes* findStyleRuleKeyframes(const StyleResolver& resolver,
                                                  String* animationName)
{
    STARFISH_ASSERT(animationName != nullptr);
    const GCVector<StyleRuleKeyframes*>& keyframes =
        resolver.ruleSet()->keyframes();
    size_t size = keyframes.size();
    for (size_t i = 0; i < size; i++) {
        if (keyframes[i]->keyframesName()->equals(animationName) == true) {
            return keyframes[i];
        }
    }
    return nullptr;
}

static AnimationKeyframe* findAnimationKeyframe(
    GCVector<AnimationKeyframe*>& keyframeList, double keyframeSelector)
{
    size_t size = keyframeList.size();
    for (size_t i = 0; i < size; i++) {
        if (keyframeList[i]->keyframeSelector() == keyframeSelector) {
            return keyframeList[i];
        }
    }

    AnimationKeyframe* keyframe = nullptr;
    if (size == 0) {
        keyframe = new AnimationKeyframe();
    } else {
        keyframe = new AnimationKeyframe(*keyframeList[size - 1], false);
    }
    keyframe->setKeyframeSelector(keyframeSelector);
    keyframeList.push_back(keyframe);

    return keyframe;
}

static bool isAnimationAffectingProperty(CSSStyleValuePair::KeyKind property)
{
    switch (property) {
    case CSSStyleValuePair::KeyKind::Animation:
    case CSSStyleValuePair::KeyKind::AnimationDelay:
    case CSSStyleValuePair::KeyKind::AnimationDirection:
    case CSSStyleValuePair::KeyKind::AnimationDuration:
    case CSSStyleValuePair::KeyKind::AnimationFillMode:
    case CSSStyleValuePair::KeyKind::AnimationIterationCount:
    case CSSStyleValuePair::KeyKind::AnimationName:
    case CSSStyleValuePair::KeyKind::AnimationPlayState:
    case CSSStyleValuePair::KeyKind::AnimationTimingFunction:
    case CSSStyleValuePair::KeyKind::Display:
    case CSSStyleValuePair::KeyKind::Transition:
    case CSSStyleValuePair::KeyKind::TransitionDelay:
    case CSSStyleValuePair::KeyKind::TransitionDuration:
    case CSSStyleValuePair::KeyKind::TransitionProperty:
    case CSSStyleValuePair::KeyKind::TransitionTimingFunction:
        return true;
    default:
        return false;
    }
}

static void setPropertyIfNeeds(
    GCVector<AnimationKeyframe*>& animationKeyframeList,
    const CSSStyleValuePair& property)
{
    // Register the cssValue to be animated in the current animationKeyframe. If
    // there is no cssValue corresponding to another item in
    // animationKeyframeList, fill it with the default value. That is, items in
    // animationKeyframeList have the same CSS property list.

    // keyframeList.back() is current frame in this context.
    auto lastAnimationKeyframe = animationKeyframeList.back();
    lastAnimationKeyframe->addProperty(property.keyKind(), property);

    for (auto keyframe : animationKeyframeList) {
        size_t idx = keyframe->keyKindIndex(property.keyKind());
        if (idx == SIZE_MAX) {
            // Syncs the property list of the items in the
            // animationKeyframeList.
            keyframe->addProperty(property.keyKind(), CSSStyleValuePair());
        }
    }
}

void computeWebAnimationKeyframes(const StyleResolver& resolver,
                                  Element* element, ComputedStyle* style,
                                  GCVector<StyleRuleBase*>& keyframes)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);
    STARFISH_ASSERT(style->animation() != nullptr);
    STARFISH_ASSERT(style->animation()->animationNameSize() > 0);

    StyleAnimationData* styleAnimationData = style->animation();
    size_t animationNameSize = styleAnimationData->animationNameSize();

    size_t index = animationNameSize - 1;
    String* name = styleAnimationData->animationName(index);
    if (name->isEmpty() || name->equalsIgnoreCase("none") == true) {
        return;
    }

    TimingFunction* timing = styleAnimationData->timingFunction(index);
    size_t styleKeyframeListSize = keyframes.size();

    GCVector<AnimationKeyframe*> keyframeList;

    for (size_t i = 0; i < styleKeyframeListSize; i++) {
        StyleRuleKeyframe* styleKeyframe = keyframes[i]->asStyleRuleKeyframe();
        GCAtomicVector<double> selectorList = styleKeyframe->selectorList();
        AnimationKeyframe* animationKeyframe =
            findAnimationKeyframe(keyframeList, selectorList[0]);
        TimingFunction* keyframeTiming = timing;

        const GCAtomicVector<CSSStyleValuePair>& cssValues =
            styleKeyframe->styleDeclaration()->cssValues();
        size_t cssValueSize = cssValues.size();
        for (size_t j = 0; j < cssValueSize; j++) {
            CSSStyleValuePair::KeyKind p = cssValues[j].keyKind();
            if (p == CSSStyleValuePair::KeyKind::AnimationTimingFunction) {
                CSSStyleValuePair::ValueKind valueKind =
                    cssValues[j].valueKind();
                if (valueKind == CSSStyleValuePair::ValueKind::Inherit &&
                    element != nullptr && element->parentElement() != nullptr &&
                    element->parentElement()->style() != nullptr &&
                    element->parentElement()->style()->animation() != nullptr) {
                    keyframeTiming = element->parentElement()
                                         ->style()
                                         ->animation()
                                         ->timingFunction(0);
                } else if (valueKind ==
                           CSSStyleValuePair::ValueKind::ValueListKind) {
                    CSSStyleValuePair& value = cssValues[j].multiValue()->at(0);
                    STARFISH_ASSERT(value.valueKind() ==
                                    CSSStyleValuePair::ValueKind::
                                        TimingFunctionPointerKind);
                    keyframeTiming = value.timingFunctionPointerValue();
                } else {
                    keyframeTiming = AnimationKeyframe::defaultTimingFunction();
                }
            } else if (isAnimationAffectingProperty(p) == false) {
                setPropertyIfNeeds(keyframeList, cssValues[j]);
            }
        }
        animationKeyframe->setTimingFunction(keyframeTiming);

        for (size_t k = 1; k < selectorList.size(); k++) {
            AnimationKeyframe* clone =
                new AnimationKeyframe(*animationKeyframe, true);
            clone->setKeyframeSelector(selectorList[k]);
            keyframeList.push_back(clone);
        }
    }

    std::stable_sort(keyframeList.begin(), keyframeList.end(),
                     [](AnimationKeyframe* a, AnimationKeyframe* b) {
                         STARFISH_ASSERT(a != nullptr);
                         STARFISH_ASSERT(b != nullptr);
                         return a->keyframeSelector() < b->keyframeSelector();
                     });

    // merge duplicate KEYFRAMEs if needs.
    size_t target_index = 0;
    for (size_t j = 1; j < keyframeList.size(); j++) {
        if (keyframeList[j]->keyframeSelector() !=
            keyframeList[target_index]->keyframeSelector()) {
            target_index++;
            keyframeList[target_index] = keyframeList[j];
        }
    }
    if (keyframeList.empty() == false) {
        keyframeList.resize(target_index + 1);
    }

    // add 0% and 100% KEYFRAMEs if absent.
    AnimationKeyframe* start = new AnimationKeyframe();
    start->setTimingFunction(timing);
    if (keyframeList.empty() == true) {
        keyframeList.push_back(start);
    } else if (keyframeList.front()->keyframeSelector() != 0.0) {
        for (auto keyKind : keyframeList.front()->keyKinds()) {
            start->addProperty(keyKind, CSSStyleValuePair());
        }
        keyframeList.insert(keyframeList.begin(), start);
    }
    if (keyframeList.back()->keyframeSelector() != 1.0) {
        AnimationKeyframe* end =
            new AnimationKeyframe(*keyframeList.back(), true);
        end->setKeyframeSelector(1.0);
        end->setTimingFunction(timing);
        keyframeList.push_back(end);
    }

    STARFISH_ASSERT(keyframeList.front()->keyframeSelector() == 0.0);
    STARFISH_ASSERT(keyframeList.back()->keyframeSelector() == 1.0);

    // apply duration into each KEYFRAME
    CSSTime duration = styleAnimationData->duration(index);
    if (duration.toTimeValue() > 0) {
        double preKeyframeName = 0.0;
        auto preKeyframe = keyframeList.front();
        for (auto curKeyframe : keyframeList) {
            if (curKeyframe->keyframeSelector() == 0) {
                continue;
            }
            if (curKeyframe->keyframeSelector() - preKeyframeName == 0) {
                preKeyframe->setDuration(CSSTime(0));
            } else {
                preKeyframe->setDuration(CSSTime(
                    duration.toTimeValue() *
                    (curKeyframe->keyframeSelector() - preKeyframeName)));
            }

            preKeyframeName = curKeyframe->keyframeSelector();
            preKeyframe = curKeyframe;
        }
    }

    styleAnimationData->animationKeyframes(index)
        .animationKeyframeList()
        .clear();
    styleAnimationData->animationKeyframes(index)
        .animationKeyframeList()
        .assign(keyframeList.begin(), keyframeList.end());
}

void computeCSSAnimationKeyframes(const StyleResolver& resolver,
                                  Element* element, ComputedStyle* style)
{
    STARFISH_ASSERT(element != nullptr);
    STARFISH_ASSERT(style != nullptr);

    StyleAnimationData* styleAnimationData = style->animation();
    if (styleAnimationData == nullptr) {
        return;
    }

    size_t animationNameSize = styleAnimationData->animationNameSize();
    for (size_t i = 0; i < animationNameSize; i++) {
        String* name = styleAnimationData->animationName(i);
        if (name->isEmpty() || name->equalsIgnoreCase("none") == true) {
            continue;
        }

        StyleRuleKeyframes* styleKeyframes =
            findStyleRuleKeyframes(resolver, name);
        if (styleKeyframes == nullptr) {
            continue;
        }

        // animation's timing function
        /// ex) animation: custom-animation 3s linear;
        /// tmingFunction linear is used for each keyframe if which hasn't own
        /// timing function.
        TimingFunction* timing = styleAnimationData->timingFunction(i);

        // Convert StyleRuleKeyframes to AnimationKeyframes.
        GCVector<AnimationKeyframe*> animationKeyframeList;
        for (StyleRuleKeyframe* styleRuleKeyframe :
             styleKeyframes->keyframeList()) {
            GCAtomicVector<double> selectorList =
                styleRuleKeyframe->selectorList();
            AnimationKeyframe* animationKeyframe =
                findAnimationKeyframe(animationKeyframeList, selectorList[0]);
            TimingFunction* keyframeTiming = timing;

            for (const CSSStyleValuePair& cssValue :
                 styleRuleKeyframe->styleDeclaration()->cssValues()) {
                CSSStyleValuePair::KeyKind p = cssValue.keyKind();
                if (cssValue.valueKind() ==
                    CSSStyleValuePair::ValueKind::VarFunctionValueKind) {
                    CSSStyleDeclaration* resolvedDeclaration =
                        const_cast<StyleResolver&>(resolver).resolveVarValue(
                            element, cssValue, cssValue.keyKind(),
                            cssValue.flagImportant());
                    for (const auto& resolvedCssValues :
                         resolvedDeclaration->cssValues()) {
                        setPropertyIfNeeds(animationKeyframeList,
                                           resolvedCssValues);
                    }
                } else if (p == CSSStyleValuePair::KeyKind::
                                    AnimationTimingFunction) {
                    CSSStyleValuePair::ValueKind valueKind =
                        cssValue.valueKind();
                    if (valueKind == CSSStyleValuePair::ValueKind::Inherit &&
                        element != nullptr &&
                        element->parentElement() != nullptr &&
                        element->parentElement()->style() != nullptr &&
                        element->parentElement()->style()->animation() !=
                            nullptr) {
                        keyframeTiming = element->parentElement()
                                             ->style()
                                             ->animation()
                                             ->timingFunction(0);
                    } else if (valueKind ==
                               CSSStyleValuePair::ValueKind::ValueListKind) {
                        CSSStyleValuePair& value = cssValue.multiValue()->at(0);
                        STARFISH_ASSERT(value.valueKind() ==
                                        CSSStyleValuePair::ValueKind::
                                            TimingFunctionPointerKind);
                        keyframeTiming = value.timingFunctionPointerValue();
                    } else {
                        keyframeTiming =
                            AnimationKeyframe::defaultTimingFunction();
                    }
                } else if (isAnimationAffectingProperty(p) == false) {
                    // Set css value to current frame and sync property list for
                    // all items in animationKeyframeList.
                    setPropertyIfNeeds(animationKeyframeList, cssValue);
                }
            }

            animationKeyframe->setTimingFunction(keyframeTiming);

            // Copy AnimationKeyframe to hanlde mulitple keyframe-selector.
            // The mulitple keyframe-selector is separated by commas.
            // For example, 10%, 20% { background-color: red; }
            for (size_t k = 1; k < selectorList.size(); k++) {
                AnimationKeyframe* clone =
                    new AnimationKeyframe(*animationKeyframe, true);
                clone->setKeyframeSelector(selectorList[k]);
                animationKeyframeList.push_back(clone);
            }
        }

        std::stable_sort(
            animationKeyframeList.begin(), animationKeyframeList.end(),
            [](AnimationKeyframe* a, AnimationKeyframe* b) {
                STARFISH_ASSERT(a != nullptr);
                STARFISH_ASSERT(b != nullptr);
                return a->keyframeSelector() < b->keyframeSelector();
            });

        // merge duplicate KEYFRAMEs if needs.
        size_t target_index = 0;
        for (size_t j = 1; j < animationKeyframeList.size(); j++) {
            if (animationKeyframeList[j]->keyframeSelector() !=
                animationKeyframeList[target_index]->keyframeSelector()) {
                target_index++;
                animationKeyframeList[target_index] = animationKeyframeList[j];
            }
        }
        if (animationKeyframeList.empty() == false) {
            animationKeyframeList.resize(target_index + 1);
        }

        // add 0% and 100% KEYFRAMEs if absent.
        AnimationKeyframe* dummyAnimationKeyframe = new AnimationKeyframe();
        dummyAnimationKeyframe->setTimingFunction(timing);
        for (auto& keyKind : animationKeyframeList.front()->keyKinds()) {
            // TODO: Consider using CSSStyleValuePair directly, which has only
            // key types and no values.
            //
            // Synchronize the keyKinds that explicitly appeared in the
            // AnimationKeyframeList.
            dummyAnimationKeyframe->addProperty(keyKind, CSSStyleValuePair());
        }

        if (animationKeyframeList.empty()) {
            AnimationKeyframe* emptyKeyframe = new AnimationKeyframe();
            emptyKeyframe->setTimingFunction(timing);
            animationKeyframeList.push_back(emptyKeyframe);
        } else if (animationKeyframeList.front()->keyframeSelector() != 0.0) {
            AnimationKeyframe* from =
                new AnimationKeyframe(*dummyAnimationKeyframe, true);
            animationKeyframeList.insert(animationKeyframeList.begin(), from);
        }

        if (animationKeyframeList.back()->keyframeSelector() != 1.0) {
            AnimationKeyframe* to = dummyAnimationKeyframe;
            to->setKeyframeSelector(1.0);
            animationKeyframeList.push_back(to);
        }

        STARFISH_ASSERT(animationKeyframeList.front()->keyframeSelector() ==
                        0.0);
        STARFISH_ASSERT(animationKeyframeList.back()->keyframeSelector() ==
                        1.0);

        // apply duration into each KEYFRAME
        CSSTime duration = styleAnimationData->duration(i);
        if (duration.toTimeValue() > 0) {
            double preKeyframeName = 0.0;
            auto preKeyframe = animationKeyframeList.front();
            for (auto curKeyframe : animationKeyframeList) {
                if (curKeyframe->keyframeSelector() == 0) {
                    continue;
                }
                if (curKeyframe->keyframeSelector() - preKeyframeName == 0) {
                    preKeyframe->setDuration(CSSTime(0));
                } else {
                    preKeyframe->setDuration(CSSTime(
                        duration.toTimeValue() *
                        (curKeyframe->keyframeSelector() - preKeyframeName)));
                }

                preKeyframeName = curKeyframe->keyframeSelector();
                preKeyframe = curKeyframe;
            }
        }

        // Add animationKeyframeList to AnimationKeyframes of StyleAnimationData
        // in computed style
        //
        // TOOD: Register AnimationKeyframes created from StyleAnimationData,
        // which represents css animation properties, in StyleAnimationData.
        // Because StyleAnimationData represents the animation property of a
        // computed style, it is newly created each time the style is
        // calculated for computed, but the old and new computed style ​​may
        // have same animation property value. There will be very little change,
        // probably while the animation is in progress. In particular, style
        // calculations that occur while animation progress is in progress will
        // rarely change animation property values. Therefore, the animation
        // keyframe list should not be newly created unless animation properties
        // or keyframes rules are changed.
        styleAnimationData->animationKeyframes(i)
            .animationKeyframeList()
            .clear();
        styleAnimationData->animationKeyframes(i)
            .animationKeyframeList()
            .assign(animationKeyframeList.begin(), animationKeyframeList.end());
    }
}

void computeTransition(Element* element, Optional<ComputedStyle*> fromStyle,
                       Optional<Frame*> oldFrame, ComputedStyle* toStyle,
                       ComputedStyleDamage& damage,
                       bool (&damagedKeys)[CSSStyleValuePair::KeyKindSize])
{
    AnimationExecutor* executor = element->document()->animationExecutor();
    AnimationExecutor::ExecutionContext context(
        executor, element, fromStyle, oldFrame, toStyle,
        element->document()->browsingContext()->styleResolveStartTick(), damage,
        damagedKeys);

    context.begin();

    // Check transition have to remove and fire events.
    executor->checkActiveTransitionsState(context);

    // Check new transition task.
    executor->addNewActiveTransitionIfNeeds(context);

    // Execute transition step.
    executor->executeActiveTransitionsStep(context);

    context.end();
}

void computeAnimation(StyleResolver& resolver, Element* element,
                      Optional<ComputedStyle*> fromStyle,
                      ComputedStyle* toStyle, ComputedStyleDamage& damage,
                      bool (&damagedKeys)[CSSStyleValuePair::KeyKindSize])
{
    StyleAnimationData* styleAnimationData = toStyle->animation();
    if (styleAnimationData != nullptr) {
        // Create AnimationKeyframes from keyframes style rule.
        computeCSSAnimationKeyframes(resolver, element, toStyle);
    }

    AnimationExecutor* executor = element->document()->animationExecutor();
    AnimationExecutor::ExecutionContext context(
        executor, element, fromStyle, nullptr, toStyle,
        element->document()->browsingContext()->styleResolveStartTick(), damage,
        damagedKeys);

    context.begin();

    // Check animation have to remove and fire events.
    executor->checkActiveAnimationsState(context);

    // Check new animation task.
    executor->addNewActiveAnimationsIfNeeds(context);

    // Execute transition step.
    executor->executeActiveAnimationsStep(context);

    context.end();
}

static ComputedStyleDamage DamageComputedStyleDamageForBeginAnimation(
    Element* element, ComputedStyle* style)
{
    if (!element->style() || !element->frame()) {
        // `!element->style()` usually means that the first style resolve for
        // that element.
        // `!element->frame()` usually means that element that was not
        // displayed will be displayed again at this time.

        if (style->animation()) {
            // In these cases the css animation should be able to begin.
            element->clearDidPrepareAnimation();
            return ComputedStyleDamage::ComputedStyleDamageAnimation;
        }
    }
    return ComputedStyleDamage::ComputedStyleDamageNone;
}

static ComputedStyleDamage applyStyleToElement(Element* element,
                                               ComputedStyle* style,
                                               StyleResolveContext& ctx)
{
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    // TODO use needsStyleRecalcOnlyForAnimation
    // bool needsStyleRecalcOnlyForAnimation =
    // element->needsStyleRecalcForAnimation();
    // bool canCareOnlyAnimation = needsStyleRecalcOnlyForAnimation &&
    // !inheritedStyleChanged;
    element->clearNeedsStyleRecalcForAnimation();

    if (style->display() == DisplayValue::NoneDisplayValue &&
        (!element->style() ||
         element->style()->display() == DisplayValue::NoneDisplayValue)) {
        element->setStyle(style, &ctx);
        element->clearNeedsStyleRecalc();
        return damage;
    }

    bool damagedKeys[CSSStyleValuePair::KeyKindSize] = {
        false,
    };

    if (!element->style()) {
        damage = (ComputedStyleDamage)(
            ComputedStyleDamage::ComputedStyleDamageInherited |
            ComputedStyleDamage::ComputedStyleDamageRebuildFrame);
    } else {
        if (!element->frame()) {
            damage = (ComputedStyleDamage)(
                ComputedStyleDamage::ComputedStyleDamageRebuildFrame);
        }
        damage = (ComputedStyleDamage)(
            damage | compareStyle(element->style(), style, damagedKeys,
                                  element->isSVGDescendantElement()));

        if (damagedKeys[CSSStyleValuePair::KeyKind::Animation] ||
            damagedKeys[CSSStyleValuePair::KeyKind::AnimationName]) {
            element->clearDidPrepareAnimation();
        }
    }
    damage = (ComputedStyleDamage)(
        damage | DamageComputedStyleDamageForBeginAnimation(element, style));

    ComputedStyle* oldStyle = element->style();
    Frame* oldFrame = element->frame();

    computeTransition(element, oldStyle, oldFrame, style, damage, damagedKeys);
#if defined(STARFISH_ENABLE_ANIMATION)
    computeAnimation(*ctx.m_styleResolver, element, oldStyle, style, damage,
                     damagedKeys);
#endif

    ctx.m_styleResolver->clearCssCustomValues();
    element->markDidPrepareAnimation();

    {
// #define STARFISH_ENABLE_PRINT_STYLE_DAMAGE
#if defined(STARFISH_ENABLE_PRINT_STYLE_DAMAGE)
        if (damage && element->style()) {
            for (size_t i = 0; i < CSSStyleValuePair::KeyKindSize; i++) {
                if (damagedKeys[i]) {
                    switch (i) {
#define ADD_CSS_KEYKIND(Name, name, cssname)                                \
    case CSSStyleValuePair::KeyKind::Name:                                  \
        STARFISH_LOG_INFO("element %p, #%s, .%s %s damaged", element,       \
                          element->id()->toUTF8NonGCString().data(),        \
                          element->className()->toUTF8NonGCString().data(), \
                          #Name "");                                        \
        break;
                        FOR_EACH_STYLE_ATTRIBUTE_TOTAL(ADD_CSS_KEYKIND)
#undef ADD_CSS_KEYKIND
                    default:
                        STARFISH_UNIMPLEMENTED();
                    }
                }
            }
        }
#endif
    }

    if (damage & ComputedStyleDamage::ComputedStyleDamageRebuildFrame) {
        if (style->display() != DisplayValue::NoneDisplayValue &&
            element->frame() == nullptr && element->renderingParentElement()) {
            // special path for Node::appendChild

            Element* e = element->renderingParentElement();
            while (e) {
                if (e->style() &&
                    (e->style()->hasBlockLikeDisplay() || e->isSVGElement())) {
                    break;
                }
                e = e->renderingParentElement();
            }

            if (e && e->frame() && !e->frame()->isFrameDocument()) {
                e->window()->browsingContext()->setNeedsFrameTreeBuild();
                if (e->isSVGChildElement()) {
                    element->markNeedsFrameTreeBuild();
                    while (e) {
                        e->markChildNeedsFrameTreeBuild();
                        e = e->renderingParentElement();
                    }
                } else {
                    FrameTreeBuilder::
                        needsFrameTreeBuildFromChildrenOfThisFrame(e->frame());
                }
            }
        } else {
            element->setNeedsFrameTreeBuild();
        }
    }

    if (damage & ComputedStyleDamage::ComputedStyleDamageLayout) {
        element->setNeedsLayout();
    }

    if (damage &
        ComputedStyleDamage::ComputedStyleDamageEstablishesStackingContext) {
        element->webView()->setNeedsEstablishesStackingContext();
    }

    if (damage & ComputedStyleDamage::
                     ComputedStyleDamageComputeStackingContextProperties) {
        element->webView()->setNeedsComputeStackingContextProperties();
    }

    if (damage & ComputedStyleDamage::ComputedStyleDamagePainting) {
        element->setNeedsPainting();
    }

    if (damage & ComputedStyleDamage::ComputedStyleDamageComposite) {
        element->setNeedsComposite();
    }
    element->setStyle(style, &ctx);
    element->clearNeedsStyleRecalc();

    return damage;
}

static ComputedStyleDamage resolveElementStyle(StyleResolveContext& ctx,
                                               Element* element,
                                               ComputedStyle* parentStyle,
                                               bool inheritedStyleChanged)
{
    StyleResolver* resolver = ctx.m_styleResolver;
    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;

    if (element->needsStyleRecalc() || inheritedStyleChanged) {
        ComputedStyle* style =
            resolver->resolveStyle(ctx, element, parentStyle);
        damage = applyStyleToElement(element, style, ctx);
    }

    return damage;
}

static void clearStyle(StyleResolveContext& ctx, Element* element)
{
    STARFISH_ASSERT(element != nullptr);

    element->clearDidPrepareAnimation();

    RenderingSiblingIterator iter(element->firstRenderingChild());
    while (true) {
        Optional<Node*> child = iter.next();
        if (!child) {
            break;
        }

        if (child->isElement() == true) {
            child->clearNeedsStyleRecalc();
            if (child->style() != nullptr) {
                ctx.pushIntoComputedStylePool(child->style());
                child->setStyle(nullptr);
                clearStyle(ctx, child->asElement());
            }
        } else {
            child->setStyle(nullptr);
        }
    }
}

static void markStyleDirtyOfSVGUseElement(SVGSVGElement* svgElement)
{
    for (auto e : svgElement->useElementsPair()) {
        if (e.second->needsStyleRecalc() || e.second->childNeedsStyleRecalc()) {
            e.first->markChildNeedsStyleRecalc();
        }
    }
}

static void resolveSVGUseElementStyle(SVGSVGElement* svgElement)
{
    for (auto e : svgElement->useElementsPair()) {
        if (e.first->childNeedsStyleRecalc()) {
            ShadowRoot* sr = e.first->internalShadowRoot().value();
            StyleResolveContext ctx(e.second);
            ComputedStyle* style = e.second->styleResolver().resolveStyle(
                ctx, e.second, e.first->style());
            sr->setStyle(
                new ComputedStyle(style)); // just set style here for inherit
            applyStyleToElement(sr->firstElementChild(), style, ctx);
            if (style->display() != NoneDisplayValue) {
                RenderingSiblingIterator iter(sr->firstRenderingChild());
                while (true) {
                    Optional<Node*> child = iter.next();
                    if (!child) {
                        break;
                    }

                    if (child->isElement()) {
                        e.second->styleResolver().resolveChildrenStyle(
                            ctx, child->asElement(), style, true);
                    }
                }
            }
            e.first->clearChildNeedsStyleRecalc();
        }
    }
}

void StyleResolver::resolveChildrenStyle(StyleResolveContext& parentContext,
                                         Node* parentElement,
                                         ComputedStyle* parentElementStyle,
                                         bool inheritedStyleChanged)
{
    ComputedStyle* childTextNodeStyle = nullptr;
    STARFISH_ASSERT(parentElementStyle->display() != NoneDisplayValue);
    ComputedStyleDamage childTextNodeComputedStyleDamage =
        ComputedStyleDamage::ComputedStyleDamageNone;

    StyleResolveContext* ctx;

    bool isSVGSVGElement = parentElement->isSVGSVGElement();
    if (UNLIKELY(isSVGSVGElement)) {
        parentElement->asSVGSVGElement()->connectUseElements();
        markStyleDirtyOfSVGUseElement(parentElement->asSVGSVGElement());
    }

    // if parentElement is host
    bool isParentElementShadowRootHost =
        parentElement->isElement() &&
        parentElement->asElement()->isShadowRootHost();
    if (UNLIKELY(isParentElementShadowRootHost)) {
        parentElement->asElement()->internalShadowRoot()->setStyle(
            parentElementStyle);
        if (parentElement->isSVGUseElement()) {
            // svg use element style will be resolved after
            return;
        } else {
            ctx = new (alloca(sizeof(StyleResolveContext)))
                StyleResolveContext(&parentElement->asElement()
                                         ->internalShadowRoot()
                                         .value()
                                         ->styleResolver(),
                                    parentContext);
        }
    } else {
        ctx = &parentContext;
        ctx->m_ancestorSelectorFilter->pushNode(parentElement);
    }

    RenderingSiblingIterator iter(parentElement->firstRenderingChild());
    while (true) {
        Optional<Node*> child = iter.next();
        if (!child) {
            break;
        }

        if (child->isElement()) {
            ComputedStyle* oldStyle = child->style();
            StyleResolveContext* originalContext = ctx;
            if (UNLIKELY(child->isSlotted())) {
                ctx = new (alloca(sizeof(StyleResolveContext)))
                    StyleResolveContext(child.value(),
                                        ctx->m_computedStylePool);
            }

            auto damage =
                resolveElementStyle(*ctx, child->asElement(),
                                    parentElementStyle, inheritedStyleChanged);

            if (damage != ComputedStyleDamage::ComputedStyleDamageNone &&
                oldStyle &&
                oldStyle->someNonInheritMemberExplicitlyInherited()) {
                child->markChildNeedsStyleRecalc();

                RenderingSiblingIterator iter(child->firstRenderingChild());
                while (true) {
                    Optional<Node*> grandChild = iter.next();
                    if (!grandChild) {
                        break;
                    }
                    if (grandChild->isElement()) {
                        grandChild->markNeedsStyleRecalc();
                    }
                }
            }

            child->m_gotInheritedStyleDirty =
                damage & ComputedStyleDamage::ComputedStyleDamageInherited;

            if (child->style()->display() == DisplayValue::NoneDisplayValue) {
                child->m_gotInheritedStyleDirty = false;
                child->clearNeedsStyleRecalc();
            } else if (inheritedStyleChanged ||
                       child->m_gotInheritedStyleDirty || !oldStyle ||
                       oldStyle->display() == NoneDisplayValue) {
                child->setChildNeedsStyleRecalc();
                child->m_gotInheritedStyleDirty = true;
            }

            if (child->style()->display() == NoneDisplayValue) {
                child->clearChildNeedsStyleRecalc();
                clearStyle(*ctx, child->asElement());
            }

            if (oldStyle && oldStyle != child->style()) {
                ctx->pushIntoComputedStylePool(oldStyle);
            }

            if (UNLIKELY(child->isSlotted())) {
                ctx->~StyleResolveContext();
            }

            ctx = originalContext;
        } else {
            if (inheritedStyleChanged || child->needsStyleRecalc()) {
                if (childTextNodeStyle == nullptr) {
                    childTextNodeStyle = new (ctx->allocateComputedStyle())
                        ComputedStyle(parentElementStyle);
                    childTextNodeStyle->loadResources(parentElement);
                    childTextNodeStyle->arrangeStyleValues(parentElementStyle,
                                                           child.value());

                    ComputedStyle* oldStyle = child->style();
                    if (oldStyle) {
                        bool damagedKeys
                            [CSSStyleValuePair::KeyKindSize]; // don't
                                                              // care
                        childTextNodeComputedStyleDamage = compareStyle(
                            childTextNodeStyle, oldStyle, damagedKeys,
                            child->isElement() &&
                                child->asElement()->isSVGDescendantElement());
                    }
                }

                child->setStyle(childTextNodeStyle);
                child->clearNeedsStyleRecalc();
                child->clearChildNeedsStyleRecalc();

                if (childTextNodeComputedStyleDamage &
                    ComputedStyleDamagePainting) {
                    child->setNeedsPainting();
                }

                if (!child->frame()) {
                    if (parentElement->frame() && parentElement->isElement()) {
                        Element* e = parentElement->asElement();
                        while (e) {
                            if (e->style()->hasBlockLikeDisplay()) {
                                break;
                            }
                            e = e->parentElement();
                        }
                        if (e && e->frame() && !e->frame()->isFrameDocument()) {
                            window()
                                ->browsingContext()
                                ->setNeedsFrameTreeBuild();
                            FrameTreeBuilder::
                                needsFrameTreeBuildFromChildrenOfThisFrame(
                                    e->frame());
                        }
                    }
                }
            }
        }
        STARFISH_ASSERT(!child->needsStyleRecalc());
    }

    iter = RenderingSiblingIterator(parentElement->firstRenderingChild());
    while (true) {
        Optional<Node*> child = iter.next();
        if (!child) {
            break;
        }

        if (child->isElement() && child->childNeedsStyleRecalc()) {
            resolveChildrenStyle(*ctx, child->asElement(), child->style(),
                                 inheritedStyleChanged ||
                                     child->m_gotInheritedStyleDirty);
        }
    }

    parentElement->clearChildNeedsStyleRecalc();

    if (UNLIKELY(isSVGSVGElement)) {
        resolveSVGUseElementStyle(parentElement->asSVGSVGElement());
    }

    if (UNLIKELY(isParentElementShadowRootHost)) {
        ctx->~StyleResolveContext();
    } else {
        ctx->m_ancestorSelectorFilter->popNode();
    }
}

void StyleResolver::resolveDOMStyle(Document* document, bool force)
{
    INSTALL_RECORDABLE_PROFILE_TIMER(ProfileKind::kStyle, "Resolve DOMStyle");
    StyleResolveContext ctx(document);
    resolveChildrenStyle(ctx, document, document->style(), force);
}

bool StyleResolver::tryAddSheet(Node* node, CSSStyleSheet* sheet)
{
    STARFISH_ASSERT(node != nullptr);
    STARFISH_ASSERT(sheet != nullptr);

    if (node->isHTMLElement()) {
        HTMLElement* htmlElement = node->asHTMLElement();
        CSSStyleSheet* nSheet = nullptr;
        if (htmlElement->isHTMLStyleElement()) {
            nSheet = htmlElement->asHTMLStyleElement()->generatedSheet();
        } else if (htmlElement->isHTMLLinkElement()) {
            nSheet = htmlElement->asHTMLLinkElement()->generatedSheet();
        }

        if (nSheet) {
            auto iter = std::find(m_sheets.begin(), m_sheets.end(), nSheet);
            STARFISH_ASSERT(iter != m_sheets.end());
            m_sheets.insert(iter, sheet);
            return true;
        }
    }
    return false;
}

bool StyleResolver::traverseAndTryAddSheet(Node* parent, CSSStyleSheet* sheet,
                                           bool& originFound)
{
    STARFISH_ASSERT(parent != nullptr);
    STARFISH_ASSERT(sheet != nullptr);

    RenderingSiblingIterator iter(parent->firstRenderingChild());
    while (true) {
        Optional<Node*> child = iter.next();
        if (!child) {
            break;
        }
        if (!originFound && child.value() == sheet->origin()) {
            originFound = true;
        } else if (originFound) {
            if (tryAddSheet(child.value(), sheet)) {
                return true;
            }
        } else {
            if (traverseAndTryAddSheet(child.value(), sheet, originFound)) {
                return true;
            }
        }
    }

    return false;
}

void StyleResolver::addSheet(CSSStyleSheet* sheet)
{
    m_needsRecalcRuleSet = true;
    document()->browsingContext()->setNeedsStyleSheetsRecalc();

    bool originFound = false;
    if (!traverseAndTryAddSheet(m_document, sheet, originFound)) {
        m_sheets.push_back(sheet);
    }
}

void StyleResolver::removeSheet(CSSStyleSheet* sheet)
{
    m_needsRecalcRuleSet = true;
    document()->browsingContext()->setNeedsStyleSheetsRecalc();

    auto iter = std::find(m_sheets.begin(), m_sheets.end(), sheet);
    STARFISH_ASSERT(iter != m_sheets.end());
    m_sheets.erase(iter);
}

void StyleResolver::removeAllRules()
{
    m_ruleSet->clear();
    m_ruleSetAttrFilter.clear();

    if (m_hasSimplePseudoClassHostSelector) {
        Traverse::traverseIncludingShadowDOM(document(), [](Node* node) {
            if (node->isShadowRoot()) {
                node->asShadowRoot()->styleResolver().setNeedsRecalcRuleSet();
            }
        });
    }
    m_hasSimplePseudoClassHostSelector = false;

    resetNextRuleSetOrder();
}

size_t StyleResolver::nextRuleSetOrder()
{
    return m_nextRuleSetOrder++;
}

void StyleResolver::resetNextRuleSetOrder()
{
    m_nextRuleSetOrder = 0;
}

class WebFontLoadChecker : public ResourceClient {
public:
    WebFontLoadChecker(Resource* res, String* wf)
        : ResourceClient(res)
        , m_familyName(wf)
    {
    }
    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        resource()
            ->loader()
            ->document()
            ->browsingContext()
            ->setWholeDocumentNeedsStyleRecalc();
        resource()->loader()->document()->fontSelector()->clearCache(
            m_familyName);
        resource()->loader()->document()->setNeedsFrameTreeBuildWithoutSelf();
        STARFISH_LOG_INFO("WebFont %s is failed to load..",
                          m_familyName->toUTF8NonGCString().data());
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        resource()
            ->loader()
            ->document()
            ->browsingContext()
            ->setWholeDocumentNeedsStyleRecalc();
        resource()->loader()->document()->fontSelector()->clearCache(
            m_familyName);
        // we needs to rebuild frame tree due to considering pseudo-elements
        resource()->loader()->document()->setNeedsFrameTreeBuildWithoutSelf();
        STARFISH_LOG_INFO("WebFont %s is downloaded",
                          m_familyName->toUTF8NonGCString().data());
        resource()->loader()->document()->updateCanvasWebFontState();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
    }

    String* m_familyName;
};

void StyleResolver::recalcRuleSetIfNeeds()
{
    if (m_needsRecalcRuleSet) {
        m_needsRecalcRuleSet = false;

        removeAllRules();

        m_viewportDependentMediaQueryResults.clear();
        m_deviceDependentMediaQueryResults.clear();

        size_t sheets = m_sheets.size();

        size_t offset = 0;
        std::vector<std::pair<CSSStyleDeclaration*, ResourceURL*>> webFonts;

        // We can use non gc vector
        // because CSSStyleSheets has string reference to each
        // CSSStyleDeclaration
        for (size_t i = 0; i < sheets; i++) {
            CSSStyleSheet* sheet = m_sheets[i];

            if (i > 0) { // i == 0 is UA-sheet
                sheet->parseSheetIfneeds();

                const MediaQueryEvaluator& evaluator = mediaQueryEvaluator();
                if (sheet->mediaQuerySet() &&
                    !sheet->matchesMediaQueries(
                        evaluator, sheet->mediaQuerySet(),
                        &m_viewportDependentMediaQueryResults,
                        &m_deviceDependentMediaQueryResults)) {
                    continue;
                }

                sheet->clearStyleRules();
                sheet->clearKeyframesRules();
                sheet->collectRulesFromImportedSheet(
                    sheet->importRules(), webFonts,
                    &m_viewportDependentMediaQueryResults,
                    &m_deviceDependentMediaQueryResults);
                sheet->collectStyleRules(sheet->childRules(), webFonts,
                                         sheet->url(),
                                         &m_viewportDependentMediaQueryResults,
                                         &m_deviceDependentMediaQueryResults);
            }

            size_t rules = sheet->styleRules().size();
            for (size_t j = 0; j < rules; j++) {
                StyleRule* rule = sheet->styleRules()[j].first;
                // If a styleRule has a simple pseudo class host, add a
                // styleRule to rule set of parent. if not, add a styleRule to
                // this rule set to support Combinators.
                if (UNLIKELY(rule->isSimplePseudoClassHostSelector() &&
                             &m_document->styleResolver() != this)) {
                    // `m_document->styleResolver() != this` means that
                    // this style resolver is for shadow-dom.
                    m_document->styleResolver().addToRuleSet(
                        sheet->styleRules()[j]);
                    m_document->styleResolver()
                        .m_hasSimplePseudoClassHostSelector = true;
                } else {
                    addToRuleSet(sheet->styleRules()[j]);
                }
            }
            offset += rules;

            size_t keyframes = sheet->keyframes().size();
            for (size_t k = 0; k < keyframes; k++) {
                addToKeyframesRule(sheet->keyframes()[k]);
            }
        }

#if !defined(PORT_CANVAS_BACKEND_MOCK)
        for (size_t i = 0; i < webFonts.size(); i++) {
            CSSStyleDeclaration* decl = webFonts[i].first;
            if (!decl->hasCSSValuePair(
                    CSSStyleValuePair::KeyKind::FontFamily) ||
                decl->getCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily)
                        .valueKind() !=
                    CSSStyleValuePair::ValueKind::KeywordValueKind ||
                !decl->hasCSSValuePair(CSSStyleValuePair::KeyKind::Src)) {
                continue;
            }
            String* fontFamily =
                decl->getCSSValuePair(CSSStyleValuePair::KeyKind::FontFamily)
                    .keywordValue();
            FontFaceSrcData* src =
                decl->getCSSValuePair(CSSStyleValuePair::KeyKind::Src)
                    .fontFaceSrcDataValue();
            bool isFontWeightSpecified =
                decl->hasCSSValuePair(CSSStyleValuePair::KeyKind::FontWeight);
            bool isFontStyleSpecified =
                decl->hasCSSValuePair(CSSStyleValuePair::KeyKind::FontStyle);
            FontStyleValue style = FontStyleValue::NormalFontStyleValue;
            char weight = FontWeightValue::NormalFontWeightValue;
            if (isFontWeightSpecified) {
                auto w = decl->getCSSValuePair(
                    CSSStyleValuePair::KeyKind::FontWeight);
                if (w.valueKind() !=
                    CSSStyleValuePair::ValueKind::FontWeightValueKind) {
                    isFontWeightSpecified = false;
                } else {
                    weight = w.fontWeightValue();
                }
            }

            switch (weight) {
            case OneHundredFontWeightValue:
                weight = 1;
                break;
            case TwoHundredsFontWeightValue:
                weight = 2;
                break;
            case ThreeHundredsFontWeightValue:
                weight = 3;
                break;
            case FourHundredsFontWeightValue:
            case NormalFontWeightValue:
                weight = 4;
                break;
            case FiveHundredsFontWeightValue:
                weight = 5;
                break;
            case SixHundredsFontWeightValue:
                weight = 6;
                break;
            case SevenHundredsFontWeightValue:
            case BoldFontWeightValue:
                weight = 7;
                break;
            case EightHundredsFontWeightValue:
                weight = 8;
                break;
            case NineHundredsFontWeightValue:
                weight = 9;
                break;
            default:
                break;
            }

            if (isFontStyleSpecified) {
                auto w = decl->getCSSValuePair(
                    CSSStyleValuePair::KeyKind::FontStyle);
                if (w.valueKind() !=
                    CSSStyleValuePair::ValueKind::FontStyleValueKind) {
                    isFontStyleSpecified = false;
                } else {
                    style = w.fontStyleValue();
                }
            }

            // finding suitable font
            std::vector<size_t> indexes;
            for (size_t k = 0; k < src->data().size(); k++) {
                indexes.push_back(k);
            }

            std::sort(indexes.begin(), indexes.end(),
                      [&](const size_t& a, const size_t& b) -> bool {
                          auto sa = src->data()[a];
                          auto sb = src->data()[b];

                          auto loadFromA = std::get<1>(sa);
                          auto loadFromB = std::get<1>(sb);

                          // a < b -> true;
                          if (loadFromA == FontFaceSrcData::Local &&
                              loadFromB == FontFaceSrcData::Local) {
                              return a < b;
                          } else if (loadFromA == FontFaceSrcData::Local &&
                                     loadFromB == FontFaceSrcData::URL) {
                              return false;
                          } else if (loadFromA == FontFaceSrcData::URL &&
                                     loadFromB == FontFaceSrcData::Local) {
                              return true;
                          } else {
                              auto formatA = std::get<2>(sa);
                              auto formatB = std::get<2>(sb);
                              return formatA > formatB;
                          }
                      });

            auto fontFaceData = src->data()[indexes[0]];

            if (std::get<1>(fontFaceData) != FontFaceSrcData::Local) {
                ResourceURL* fontURL = nullptr;
                FontResource* res = nullptr;
                if (webFonts[i].second && (webFonts[i].second)->isValid()) {
                    fontURL =
                        new ResourceURL(std::get<0>(fontFaceData),
                                        (webFonts[i].second)->urlString());
                } else {
                    fontURL =
                        new ResourceURL(std::get<0>(fontFaceData),
                                        document()->documentURI()->urlString());
                }

                for (size_t i = 0; i < document()->m_loadedWebFontList.size();
                     i++) {
                    if (fontURL->urlString()->equals(
                            document()
                                ->m_loadedWebFontList[i]
                                ->url()
                                ->urlString())) {
                        res = document()->m_loadedWebFontList[i];
                        break;
                    }
                }

                if (res == nullptr) {
                    res = document()->resourceLoader().fetchFont(fontURL);
                    res->addResourceClient(
                        new WebFontLoadChecker(res, fontFamily));

                    document()->m_loadedWebFontList.push_back(res);
                }

                WebFont webFont(isFontStyleSpecified, isFontWeightSpecified,
                                fontFamily, style, weight, res);

                bool found = false;
                for (size_t i = 0; i < document()->m_webFontList.size(); i++) {
                    if (document()->m_webFontList[i] == webFont) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    document()->m_webFontList.push_back(webFont);
                }

                if (webView()->needsDownloadWebFontsEarly() &&
                    !webFont.fontResource()->isRequested()) {
                    RequestData* reqData = new RequestData();
                    reqData->m_url = webFont.fontResource()->url();
                    reqData->m_referrer =
                        new ReferrerURL(document()->documentURI());
                    reqData->m_destination = RequestDestination::Font;
                    reqData->m_syncLevel =
                        RequestSyncLevel::SyncIfAlreadyLoaded;
                    webFont.fontResource()->request(reqData, true);
                }
            } else {
                WebFont webFont(isFontStyleSpecified, isFontWeightSpecified,
                                fontFamily, style, weight,
                                std::get<0>(fontFaceData));

                bool found = false;
                for (size_t i = 0; i < document()->m_webFontList.size(); i++) {
                    if (document()->m_webFontList[i] == webFont) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    document()->m_webFontList.push_back(webFont);
                }
            }
        }
#endif
    }
}

static void extractValuesforSelector(const CSSSelector* selector,
                                     AtomicString& id, AtomicString& className,
                                     AtomicString& tagName)
{
    STARFISH_ASSERT(selector != nullptr);

    switch (selector->type()) {
    case CSSSelector::Id:
        id = selector->selectorText();
        break;
    case CSSSelector::Class:
        className = selector->selectorText();
        break;
    case CSSSelector::Tag:
        tagName = selector->selectorText();
        break;
    default:
        break;
    }
}

void StyleResolver::addToRuleSet(std::pair<StyleRule*, ResourceURL*> rule)
{
    rule.first->initFlagsRelatedWithSelectorList();
    auto selectorList = rule.first->selectorList();

    AtomicString id;
    AtomicString className;
    AtomicString tagName;

    size_t size = selectorList.size();

    auto relation = selectorList[0].m_relation;
    extractValuesforSelector(selectorList[0].m_selector, id, className,
                             tagName);

    for (size_t i = 0; i < size && relation == CSSSelectorListItem::SubSelector;
         i++) {
        relation = selectorList[i].m_relation;
        extractValuesforSelector(selectorList[i].m_selector, id, className,
                                 tagName);
    }

    for (size_t i = 0; i < size; i++) {
        CSSSelector* selector = selectorList[i].m_selector;
        CSSSelector* currentSelector = selector;
        size_t subSelectorIndex = 0;
        size_t subSelectorSize =
            UNLIKELY(selector->type() == CSSSelector::Type::PseudoClass)
                ? selector->asCSSPseudoSelector()->pseudoSelectorList().size()
                : 0;
        while (currentSelector) {
            if (currentSelector->isAttributeSelector()) {
                if (!mayHaveAttrSelectorWithName(
                        currentSelector->asCSSAttributeSelector()
                            ->attribute()
                            .localNameAtomic())) {
                    m_ruleSetAttrFilter.push_back(
                        currentSelector->asCSSAttributeSelector()
                            ->attribute()
                            .localNameAtomic());
                }
            }

            if (UNLIKELY(subSelectorIndex < subSelectorSize)) {
                // For functional pseudo class selector such as :not(), :host()
                // can have compound selector
                currentSelector = selector->asCSSPseudoSelector()
                                      ->pseudoSelectorList()[subSelectorIndex++]
                                      .m_selector;
            } else {
                currentSelector = nullptr;
            }
        }
    }

    size_t order = nextRuleSetOrder();
    rule.first->setOrder(order);

    if (!id.isEmptyAtomicString()) {
        m_ruleSet->idRules().insert(std::make_pair(id, rule));
        return;
    }
    if (!className.isEmptyAtomicString()) {
        m_ruleSet->classRules().insert(std::make_pair(className, rule));
        return;
    }
    if (!tagName.isEmptyAtomicString()) {
        m_ruleSet->tagRules().insert(std::make_pair(tagName, rule));
        return;
    }
    m_ruleSet->universalRules().push_back(rule);
}

void StyleResolver::addToKeyframesRule(StyleRuleKeyframes* rule)
{
    STARFISH_ASSERT(rule != nullptr);
    STARFISH_ASSERT(rule->isKeyframesRule() == true);
    m_ruleSet->keyframes().push_back(rule);
}

const MediaQueryEvaluator& StyleResolver::mediaQueryEvaluator()
{
    if (m_mediaQueryEvaluator == nullptr) {
        m_mediaQueryEvaluator = new MediaQueryEvaluator(
            String::fromUTF8("screen"), new MediaValues(document()->frame()));
    }
    return *m_mediaQueryEvaluator;
}

bool StyleResolver::mediaQueryAffectedByViewportChange()
{
    auto evaluator = mediaQueryEvaluator();
    auto results = viewportDependentMediaQueryResults();
    for (size_t i = 0; i < results.size(); i++) {
        if (evaluator.eval(results[i]->expression()) != results[i]->result()) {
            return true;
        }
    }
    return false;
}

bool StyleResolver::mediaQueryAffectedByDeviceChange()
{
    auto evaluator = mediaQueryEvaluator();
    auto results = deviceDependentMediaQueryResults();
    for (size_t i = 0; i < results.size(); i++) {
        if (evaluator.eval(results[i]->expression()) != results[i]->result()) {
            return true;
        }
    }
    return false;
}

bool CSSStyleValuePair::updateValueUnitColor(const CSSTokenValue& token)
{
    if (CSSPropertyParser::parseNonNamedColor(token, this)) {
        return true;
    } else {
        return CSSPropertyParser::parseNamedColor(token, this);
    }
}

bool CSSStyleValuePair::updateValueUnitBorderColor(const CSSTokenValue& token)
{
    return updateValueUnitColor(token);
}

bool CSSStyleValuePair::updateValueUnitMargin(const CSSTokenValue& value)
{
    return updateValueUnitLengthOrCalc(value,
                                       CSSPropertyParser::AllowNegative |
                                           CSSPropertyParser::AllowPercent |
                                           CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueUnitInset(const CSSTokenValue& value)
{
    return updateValueUnitLengthOrCalc(value,
                                       CSSPropertyParser::AllowNegative |
                                           CSSPropertyParser::AllowPercent |
                                           CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueUnitPadding(const CSSTokenValue& value)
{
    return updateValueUnitLengthOrCalc(CSSTokenValue(value),
                                       CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueColor(Document* document,
                                         const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueBackgroundColor(Document* document,
                                                   const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueColor(document, tokens);
}

bool CSSStyleValuePair::updateValueCaretColor(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ColorValueKind;
    if (value.equals("auto")) {
        CSSTokenValue token("currentcolor");
        return CSSPropertyParser::parseNamedColor(token, this);
    } else if (value.equals("transparent")) {
        m_value.m_color = Unit::Color(0, 0, 0, 0);
        return true;
    } else {
        return updateValueColor(document, tokens);
    }

    return false;
}

#define UPDATE_VALUE_BORDER_COLOR(POS, ...)                \
    bool CSSStyleValuePair::updateValueBorder##POS##Color( \
        Document* document, const CSSTokenVector& tokens)  \
    {                                                      \
        return updateValueColor(document, tokens);         \
    }
GEN_FOURSIDE(UPDATE_VALUE_BORDER_COLOR)
#undef UPDATE_VALUE_BORDER_COLOR

bool CSSStyleValuePair::updateValueBorderBlockStartColor(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueColor(document, tokens);
}

bool CSSStyleValuePair::updateValueBorderBlockEndColor(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueColor(document, tokens);
}

bool CSSStyleValuePair::updateValueBorderInlineStartColor(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueColor(document, tokens);
}

bool CSSStyleValuePair::updateValueBorderInlineEndColor(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueColor(document, tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderStyle(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BorderStyleValueKind;
    if (value.equals("none")) {
        m_value.m_borderStyle = BorderStyleValue::NoneBorderStyleValue;
    } else if (value.equals("hidden")) {
        m_value.m_borderStyle = BorderStyleValue::HiddenBorderStyleValue;
    } else if (value.equals("solid") || value.equals("auto")) {
        m_value.m_borderStyle = BorderStyleValue::SolidBorderStyleValue;
    } else if (value.equals("dashed")) {
        m_value.m_borderStyle = BorderStyleValue::DashedBorderStyleValue;
    } else if (value.equals("inset")) {
        m_value.m_borderStyle = BorderStyleValue::InsetBorderStyleValue;
    } else if (value.equals("outset")) {
        m_value.m_borderStyle = BorderStyleValue::OutsetBorderStyleValue;
    } else if (value.equals("dotted")) {
        m_value.m_borderStyle = BorderStyleValue::DottedBorderStyleValue;
    } else if (value.equals("double")) {
        m_value.m_borderStyle = BorderStyleValue::DoubleBorderStyleValue;
    } else if (value.equals("groove")) {
        m_value.m_borderStyle = BorderStyleValue::GrooveBorderStyleValue;
    } else if (value.equals("ridge")) {
        m_value.m_borderStyle = BorderStyleValue::RidgeBorderStyleValue;
    } else {
        return false;
    }
    return true;
}

#define UPDATE_VALUE_BORDER_STYLE(POS, ...)                \
    bool CSSStyleValuePair::updateValueBorder##POS##Style( \
        Document* document, const CSSTokenVector& tokens)  \
    {                                                      \
        if (tokens.size() != 1) {                          \
            return false;                                  \
        }                                                  \
        return updateValueUnitBorderStyle(tokens[0]);      \
    }
GEN_FOURSIDE(UPDATE_VALUE_BORDER_STYLE)
#undef UPDATE_VALUE_BORDER_STYLE

bool CSSStyleValuePair::updateValueBorderBlockStartStyle(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderBlockEndStyle(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderInlineStartStyle(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderInlineEndStyle(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderRadius(const CSSTokenVector& tokens)
{
    size_t len = tokens.size();
    if (len < 1 || len > 2) {
        return false;
    }
    ValueList* list = new ValueList(Separator::SpaceSeparator);

    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& t = tokens[i];
        const char* value = t.data();
        CSSStyleValuePair pair;
        if (compareCString("inherit", value)) {
            pair.setValueKind(CSSStyleValuePair::ValueKind::Inherit);
        } else if (compareCString("initial", value)) {
            pair.setValueKind(CSSStyleValuePair::ValueKind::Initial);
        } else {
            auto ret = pair.updateValueUnitLengthOrCalc(
                t, CSSPropertyParser::AllowPercent);
            if (!ret) {
                return false;
            }
        }
        list->push_back(pair);
    }

    setValueList(list);
    return true;
}

bool CSSStyleValuePair::updateValueBorderTopLeftRadius(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBorderRadius(tokens);
}

bool CSSStyleValuePair::updateValueBorderTopRightRadius(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBorderRadius(tokens);
}

bool CSSStyleValuePair::updateValueBorderBottomLeftRadius(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBorderRadius(tokens);
}

bool CSSStyleValuePair::updateValueBorderBottomRightRadius(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBorderRadius(tokens);
}

bool CSSStyleValuePair::updateValueDirection(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::DirectionValueKind;
    if (value.equals("ltr")) {
        m_value.m_direction = DirectionValue::LtrDirectionValue;
    } else if (value.equals("rtl")) {
        m_value.m_direction = DirectionValue::RtlDirectionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWhiteSpace(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::WhiteSpaceValueKind;
    if (value.equals("normal")) {
        m_value.m_whiteSpace = WhiteSpaceValue::NormalWhiteSpaceValue;
    } else if (value.equals("nowrap")) {
        m_value.m_whiteSpace = WhiteSpaceValue::NoWrapWhiteSpaceValue;
    } else if (value.equals("pre")) {
        m_value.m_whiteSpace = WhiteSpaceValue::PreWhiteSpaceValue;
    } else if (value.equals("pre-wrap")) {
        m_value.m_whiteSpace = WhiteSpaceValue::PreWrapWhiteSpaceValue;
    } else if (value.equals("pre-line")) {
        m_value.m_whiteSpace = WhiteSpaceValue::PreLineWhiteSpaceValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueObjectFit(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ObjectFitValueKind;
    if (value.equals("fill")) {
        m_value.m_objectFit = ObjectFitValue::FillObjectFitValue;
    } else if (value.equals("contain")) {
        m_value.m_objectFit = ObjectFitValue::ContainObjectFitValue;
    } else if (value.equals("cover")) {
        m_value.m_objectFit = ObjectFitValue::CoverObjectFitValue;
    } else if (value.equals("none")) {
        m_value.m_objectFit = ObjectFitValue::NoneObjectFitValue;
    } else if (value.equals("scale-down")) {
        m_value.m_objectFit = ObjectFitValue::ScaledownObjectFitValue;
    } else {
        return false;
    }
    return true;
}

static bool isPositionValue(const CSSTokenVector& tokens, unsigned int idx,
                            bool& isXAxis, bool& isYAxis,
                            CSSStyleValuePair& result)
{
    if (tokens.size() <= idx) {
        return false;
    }
    const CSSTokenValue& value = tokens[idx];
    result.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
    if (value.equals("left")) {
        if (isXAxis) {
            return false;
        }
        result.setValue(SideValue::LeftSideValue);
        isXAxis = true;
    } else if (value.equals("right")) {
        if (isXAxis) {
            return false;
        }
        result.setValue(SideValue::RightSideValue);
        isXAxis = true;
    } else if (value.equals("center")) {
        result.setValue(SideValue::CenterSideValue);
    } else if (value.equals("top")) {
        if (isYAxis) {
            return false;
        }
        result.setValue(SideValue::TopSideValue);
        isYAxis = true;
    } else if (value.equals("bottom")) {
        if (isYAxis) {
            return false;
        }
        result.setValue(SideValue::BottomSideValue);
        isYAxis = true;
    } else {
        uint8_t option =
            CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowNegative;
        return result.updateValueUnitLengthOrCalc(value, option);
    }
    return true;
}

static bool isSideTokenValue(const CSSTokenValue& value)
{
    return value.equals("left") || value.equals("right") ||
           value.equals("center") || value.equals("top") ||
           value.equals("bottom");
}

static bool isVerticalValue(const CSSStyleValuePair& value)
{
    if (!value.isSideValueKind()) {
        return false;
    }
    return value.sideValue() == SideValue::TopSideValue ||
           value.sideValue() == SideValue::BottomSideValue;
}

static bool isCenterValue(const CSSStyleValuePair& value)
{
    if (!value.isSideValueKind()) {
        return false;
    }
    return value.sideValue() == SideValue::CenterSideValue;
}

static bool isHorizontalValue(const CSSStyleValuePair& value)
{
    if (!value.isSideValueKind()) {
        return false;
    }
    return value.sideValue() == SideValue::LeftSideValue ||
           value.sideValue() == SideValue::RightSideValue;
}

static bool updatePositionValue(const CSSStyleValuePair& value,
                                CSSStyleValuePair& xPair,
                                CSSStyleValuePair& yPair)
{
    bool isYValue = isVerticalValue(value);
    xPair = value;
    yPair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
    yPair.setValue(SideValue::CenterSideValue);
    if (isYValue) {
        std::swap(xPair, yPair);
    }
    return true;
}

static bool updatePositionValue(const CSSStyleValuePair& value1,
                                const CSSStyleValuePair& value2,
                                CSSStyleValuePair& xPair,
                                CSSStyleValuePair& yPair)
{
    bool isXY = isHorizontalValue(value1) || isVerticalValue(value2) ||
                !value1.isSideValueKind() || !value2.isSideValueKind();
    bool isYX = isVerticalValue(value1) || isHorizontalValue(value2);

    if (isXY && isYX) {
        return false;
    }
    xPair = value1;
    yPair = value2;
    if (isYX) {
        std::swap(xPair, yPair);
    }
    return true;
}

static bool updatePositionValue(const GCVector<CSSStyleValuePair>& values,
                                CSSStyleValuePair& xPair,
                                CSSStyleValuePair& yPair)
{
    CSSStyleValuePair center;
    for (unsigned int i = 0; i < values.size(); i++) {
        CSSStyleValuePair current = values[i];

        if (current.valueKind() == CSSStyleValuePair::ValueKind::None) {
            break;
        }

        if (!current.isSideValueKind()) {
            return false;
        }

        if (isCenterValue(current)) {
            if (center.isSideValueKind()) {
                return false;
            }
            center = current;
            continue;
        }

        CSSStyleValuePair result;
        if (i + 1 < values.size() &&
            values[i + 1].valueKind() != CSSStyleValuePair::ValueKind::None &&
            !values[i + 1].isSideValueKind()) {
            CSSStyleValuePair second = values[++i];
            ValuePair* pair = new ValuePair(current, second);
            result.setValuePair(pair);
        } else {
            result = current;
        }

        if (isHorizontalValue(current)) {
            if (xPair.valueKind() != CSSStyleValuePair::ValueKind::None) {
                return false;
            }
            xPair = result;
        } else {
            STARFISH_ASSERT(isVerticalValue(current));
            if (yPair.valueKind() != CSSStyleValuePair::ValueKind::None) {
                return false;
            }
            yPair = result;
        }
    }

    if (center.isSideValueKind()) {
        if (xPair.valueKind() != CSSStyleValuePair::ValueKind::None &&
            yPair.valueKind() != CSSStyleValuePair::ValueKind::None) {
            return false;
        }
        if (xPair.valueKind() == CSSStyleValuePair::ValueKind::None) {
            xPair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
            xPair.setValue(SideValue::CenterSideValue);
        } else {
            yPair.setValueKind(CSSStyleValuePair::ValueKind::SideValueKind);
            yPair.setValue(SideValue::CenterSideValue);
        }
    }

    return true;
}

bool CSSStyleValuePair::updateValueObjectPosition(Document* document,
                                                  const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    CSSStyleValuePair xPair;
    CSSStyleValuePair yPair;

    if (updateValueObjectPosition(tokens, xPair, yPair)) {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
        m_value.m_multiValue = new ValueList(Separator::SpaceSeparator);
        m_value.m_multiValue->push_back(xPair);
        m_value.m_multiValue->push_back(yPair);
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueObjectPosition(const CSSTokenVector& tokens,
                                                  CSSStyleValuePair& xPair,
                                                  CSSStyleValuePair& yPair)
{
    if (tokens.size() > 4) {
        return false;
    }

    bool isXAxis = false;
    bool isYAxis = false;

    CSSStyleValuePair value1;
    bool ret = isPositionValue(tokens, 0, isXAxis, isYAxis, value1);
    if (!ret) {
        return false;
    }

    CSSStyleValuePair value2;
    ret = isPositionValue(tokens, 1, isXAxis, isYAxis, value2);
    if (!ret) {
        return updatePositionValue(value1, xPair, yPair);
    }

    CSSStyleValuePair value3;
    ret = false;
    if (value1.isSideValueKind() &&
        ((value2.isSideValueKind()) != isSideTokenValue(tokens[2])) &&
        (!isCenterValue(value2.isSideValueKind() ? value2 : value1))) {
        ret = isPositionValue(tokens, 2, isXAxis, isYAxis, value3);
    }

    if (!ret) {
        if (isYAxis && !value2.isSideValueKind()) {
            return updatePositionValue(value1, xPair, yPair);
        }
        return updatePositionValue(value1, value2, xPair, yPair);
    }

    CSSStyleValuePair value4;
    ret = false;
    if (value3.isSideValueKind() && !isCenterValue(value3) &&
        !isSideTokenValue(tokens[3])) {
        ret = isPositionValue(tokens, 3, isXAxis, isYAxis, value4);
    }

    if (!ret) {
        if (tokens.size() < 3) {
            if (isYAxis && !value2.isSideValueKind()) {
                return updatePositionValue(value1, xPair, yPair);
            }
            return updatePositionValue(value1, value2, xPair, yPair);
        }
    }

    CSSStyleValuePair value5;
    GCVector<CSSStyleValuePair> values;
    values.push_back(value1);
    values.push_back(value2);
    values.push_back(value3);
    values.push_back(value4);

    return updatePositionValue(values, xPair, yPair);
}

bool CSSStyleValuePair::updateValueWordSpacing(Document* document,
                                               const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitWordSpacing(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitWordSpacing(const CSSTokenValue& value)
{
    // <normal> | length | initial | inherit
    if (value.equals("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowNegative);
    }
}

bool CSSStyleValuePair::updateValueUnitImageValue(const CSSTokenValue& value)
{
    // https://drafts.csswg.org/css-images-3/#typedef-image
    // <<image> = <url> | <gradient>
    return updateValueUnitUrlOrNone(value) || updateValueUnitGradient(value);
}

bool CSSStyleValuePair::updateValueLetterSpacing(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    // <normal> | length | initial | inherit
    const CSSTokenValue& value = tokens[0];
    if (value.equals("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowNegative);
    }
}

bool CSSStyleValuePair::updateValueDisplay(Document* document,
                                           const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::DisplayValueKind;
    if (value.equals("block")) {
        m_value.m_display = DisplayValue::BlockDisplayValue;
    } else if (value.equals("inline")) {
        m_value.m_display = DisplayValue::InlineDisplayValue;
    } else if (value.equals("inline-list-item")) {
        m_value.m_display = DisplayValue::InlineListItemDisplayValue;
    } else if (value.equals("list-item")) {
        m_value.m_display = DisplayValue::ListItemDisplayValue;
    } else if (value.equals("inline-block")) {
        m_value.m_display = DisplayValue::InlineBlockDisplayValue;
    } else if (value.equals("table")) {
        m_value.m_display = DisplayValue::TableDisplayValue;
    } else if (value.equals("inline-table")) {
        m_value.m_display = DisplayValue::InlineTableDisplayValue;
    } else if (value.equals("table-row-group")) {
        m_value.m_display = DisplayValue::TableRowGroupDisplayValue;
    } else if (value.equals("table-header-group")) {
        m_value.m_display = DisplayValue::TableHeaderGroupDisplayValue;
    } else if (value.equals("table-footer-group")) {
        m_value.m_display = DisplayValue::TableFooterGroupDisplayValue;
    } else if (value.equals("table-row")) {
        m_value.m_display = DisplayValue::TableRowDisplayValue;
    } else if (value.equals("table-column-group")) {
        m_value.m_display = DisplayValue::TableColumnGroupDisplayValue;
    } else if (value.equals("table-column")) {
        m_value.m_display = DisplayValue::TableColumnDisplayValue;
    } else if (value.equals("table-cell")) {
        m_value.m_display = DisplayValue::TableCellDisplayValue;
    } else if (value.equals("table-caption")) {
        m_value.m_display = DisplayValue::TableCaptionDisplayValue;
    } else if (value.equals("flex")) {
        m_value.m_display = DisplayValue::FlexDisplayValue;
    } else if (value.equals("inline-flex")) {
        m_value.m_display = DisplayValue::InlineFlexDisplayValue;
#if defined(STARFISH_ENABLE_CSS_WEBKIT_FLEX_PREFIX)
    } else if (value.equals("-webkit-flex")) {
        m_value.m_display = DisplayValue::FlexDisplayValue;
    } else if (value.equals("-webkit-inline-flex")) {
        m_value.m_display = DisplayValue::InlineFlexDisplayValue;
#endif
#if defined(STARFISH_ENABLE_CSS_WEBKIT_BOX_PREFIX)
    } else if (value.equals("-webkit-box")) {
        m_value.m_display = DisplayValue::BoxDisplayValue;
    } else if (value.equals("-webkit-inline-box")) {
        m_value.m_display = DisplayValue::InlineBoxDisplayValue;
#endif
    } else if (value.equals("grid")) {
        m_value.m_display = DisplayValue::GridDisplayValue;
    } else if (value.equals("inline-grid")) {
        m_value.m_display = DisplayValue::InlineGridDisplayValue;
    } else if (value.equals("none")) {
        m_value.m_display = DisplayValue::NoneDisplayValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueAll(Document* document,
                                       const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    // initial | inherit | unset | revert
    if (tokens.size() != 1) {
        return false;
    }

    // TODO: 'revert' value is added in CSS Level 4
    // Currently, this property value is not supported by Chrome, so we will
    // support this value in the future.

    return false;
}

bool CSSStyleValuePair::updateValuePointerEvents(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::PointerEventsValueKind;
    if (value.equals("none")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsNoneValue;
    } else if (value.equals("auto")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsAutoValue;
    } else if (value.equals("visiblepainted")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsVisiblePaintedValue;
    } else if (value.equals("visiblefill")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsVisibleFillValue;
    } else if (value.equals("visiblestroke")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsVisibleStrokeValue;
    } else if (value.equals("visible")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsVisibleValue;
    } else if (value.equals("painted")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsPaintedValue;
    } else if (value.equals("fill")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsFillValue;
    } else if (value.equals("stroke")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsStrokeValue;
    } else if (value.equals("all")) {
        m_value.m_pointerEventsValue =
            PointerEventsValue::PointerEventsAllValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueMixBlendMode(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BlendModeValueKind;
    for (int i = 0; i < CanvasBlend::sizeOfBlendModeNames; ++i) {
        if (value.equals(CanvasBlend::blendModeNames[i])) {
            auto bm = static_cast<BlendMode>(i);
            m_value.m_blendMode = bm;
            return true;
        }
    }
    return false;
}

bool CSSStyleValuePair::updateValueFloat(Document* document,
                                         const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::FloatValueKind;
    if (value.equals("none")) {
        m_value.m_float = FloatValue::NoneFloatValue;
    } else if (value.equals("left")) {
        m_value.m_float = FloatValue::LeftFloatValue;
    } else if (value.equals("right")) {
        m_value.m_float = FloatValue::RightFloatValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueClear(Document* document,
                                         const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ClearValueKind;
    if (value.equals("none")) {
        m_value.m_clear = ClearValue::NoneClearValue;
    } else if (value.equals("left")) {
        m_value.m_clear = ClearValue::LeftClearValue;
    } else if (value.equals("right")) {
        m_value.m_clear = ClearValue::RightClearValue;
    } else if (value.equals("both")) {
        m_value.m_clear = ClearValue::BothClearValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFontStyle(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitFontStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueFontKerning(Document* document,
                                               const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::FontKerningValueKind;
    if (value.equals("auto")) {
        m_value.m_fontKerning = FontKerningValue::FontKerningAutoValue;
    } else if (value.equals("normal")) {
        m_value.m_fontKerning = FontKerningValue::FontKerningNormalValue;
    } else if (value.equals("none")) {
        m_value.m_fontKerning = FontKerningValue::FontKerningNoneValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitFontStyle(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FontStyleValueKind;
    if (value.equals("normal")) {
        m_value.m_fontStyle = FontStyleValue::NormalFontStyleValue;
    } else if (value.equals("italic")) {
        m_value.m_fontStyle = FontStyleValue::ItalicFontStyleValue;
    } else if (value.equals("oblique")) {
        m_value.m_fontStyle = FontStyleValue::ObliqueFontStyleValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitRepeatStyle(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::RepeatStyleValueKind;
    if (value.equals("no-repeat")) {
        m_value.m_repeatStyle = RepeatStyleValue::NoRepeatRepeatValue;
    } else if (value.equals("repeat")) {
        m_value.m_repeatStyle = RepeatStyleValue::RepeatRepeatValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundRepeatX(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitRepeatStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueBackgroundRepeatY(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitRepeatStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitUrlOrNone(const CSSTokenValue& value)
{
    if (value.equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
    } else {
        return CSSPropertyParser::parseUrl(value.data(), this);
    }

    return true;
}

bool CSSStyleValuePair::updateValueUnitGradient(const CSSTokenValue& value)
{
    auto ss = value.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());
    parser.consumeString(CSSPropertyParser::AllowNegative);

    const CSSTokenValue& type = parser.parsedString();
    CSSGradientValue* gradient;
    if (type == "linear-gradient" && parser.consumeIfNext('(')) {
        // linear-gradient() = linear-gradient(
        //   [ <angle> | to <side-or-corner> ]?
        //   <color-stop-list>
        // )

        CSSLinearGradientValue* linearGradientValue =
            new CSSLinearGradientValue();

        CSSAngle angle;
        CSSStyleValuePair s;
        parser.consumeWhitespaces();
        if (*(parser.curPos()) == 't' || *(parser.curPos()) == '-' ||
            isDigit(*(parser.curPos()))) {
            if (!parser.consumeString(CSSPropertyParser::AllowNegative |
                                      CSSPropertyParser::AllowDot)) {
                return false;
            }
            const CSSTokenValue& str = parser.parsedString();
            if (str == "to") {
                CSSStyleValuePair leftOrRight;
                CSSStyleValuePair topOrBottom;
                uint8_t sideOrConer = 0;
                while (parser.consumeWhitespaces() &&
                       *(parser.curPos()) != ',') {
                    parser.consumeString(0);
                    const CSSTokenValue& ps = parser.parsedString();
                    if (topOrBottom.valueKind() !=
                            CSSStyleValuePair::ValueKind::SideValueKind &&
                        ps == "top") {
                        topOrBottom.setValueKind(
                            CSSStyleValuePair::ValueKind::SideValueKind);
                        topOrBottom.setValue(SideValue::TopSideValue);
                    } else if (leftOrRight.valueKind() !=
                                   CSSStyleValuePair::ValueKind::
                                       SideValueKind &&
                               ps == "right") {
                        leftOrRight.setValueKind(
                            CSSStyleValuePair::ValueKind::SideValueKind);
                        leftOrRight.setValue(SideValue::RightSideValue);
                    } else if (topOrBottom.valueKind() !=
                                   CSSStyleValuePair::ValueKind::
                                       SideValueKind &&
                               ps == "bottom") {
                        topOrBottom.setValueKind(
                            CSSStyleValuePair::ValueKind::SideValueKind);
                        topOrBottom.setValue(SideValue::BottomSideValue);
                    } else if (leftOrRight.valueKind() !=
                                   CSSStyleValuePair::ValueKind::
                                       SideValueKind &&
                               ps == "left") {
                        leftOrRight.setValueKind(
                            CSSStyleValuePair::ValueKind::SideValueKind);
                        leftOrRight.setValue(SideValue::LeftSideValue);
                    } else {
                        return false;
                    }
                }

                if (leftOrRight.valueKind() ==
                    CSSStyleValuePair::ValueKind::SideValueKind) {
                    linearGradientValue->setLeftOrRight(leftOrRight);
                }
                if (topOrBottom.valueKind() ==
                    CSSStyleValuePair::ValueKind::SideValueKind) {
                    linearGradientValue->setTopOrBottom(topOrBottom);
                }
                parser.consumeIfNext(',');
            } else if (parser.parseAngle(str.c_str(),
                                         CSSPropertyParser::AllowNegative |
                                             CSSPropertyParser::AllowDot,
                                         &s)) {
                angle = s.angleValue();
                linearGradientValue->setAngle(angle);
                parser.consumeIfNext(',');
            }
        }
        gradient = linearGradientValue;
    } else if (type == "radial-gradient" && parser.consumeIfNext('(')) {
        // <radial-gradient> = radial-gradient(
        //   [ [ <shape> || <size> ] [ at <position> ]? , |
        //     at <position>,
        //   ]?
        //   <color-stop> [ , <color-stop> ]+
        // )
        CSSRadialGradientValue* radialGradientValue =
            new CSSRadialGradientValue();
        parser.consumeWhitespaces();
        if (isDigit(*(parser.curPos())) || *(parser.curPos()) == 'a' ||
            *(parser.curPos()) == 'c' || *(parser.curPos()) == 'f' ||
            *(parser.curPos()) == 'e') {
            RadialGradientShape shape = RadialGradientShape::None;
            CSSRadialGradientSize size;
            CSSStyleValuePair firstRadius;
            CSSStyleValuePair secondRadius;
            CSSStyleValuePair positionX;
            CSSStyleValuePair positionY;
            CSSTokenVector tokens;
            bool inPositionStr = false;
            while (parser.consumeWhitespaces() && *(parser.curPos()) != ',') {
                if (!parser.consumeString(CSSPropertyParser::AllowNegative |
                                          CSSPropertyParser::AllowPercent |
                                          CSSPropertyParser::AllowDot)) {
                    return false;
                }
                const CSSTokenValue& value = parser.parsedString();
                CSSStyleValuePair temp;
                if (!inPositionStr && value == "circle") {
                    if (shape != RadialGradientShape::None) {
                        return false;
                    }
                    shape = RadialGradientShape::Circle;
                } else if (!inPositionStr && value == "ellipse") {
                    if (shape != RadialGradientShape::None) {
                        return false;
                    }
                    shape = RadialGradientShape::Elipse;
                } else if (!inPositionStr &&
                           (value[0] == 'c' || value[0] == 'f')) {
                    if (size.hasValue()) {
                        return false;
                    }
                    if (value == "closest-side") {
                        size.setKeyword(RadialGradientSizeKeyword::ClosetSide);
                    } else if (value == "farthest-side") {
                        size.setKeyword(
                            RadialGradientSizeKeyword::FarthestSide);
                    } else if (value == "closest-corner") {
                        size.setKeyword(
                            RadialGradientSizeKeyword::ClosetCorner);
                    } else if (value == "farthest-corner") {
                        size.setKeyword(
                            RadialGradientSizeKeyword::FarthestCorner);
                    } else {
                        return false;
                    }
                } else if (!inPositionStr &&
                           temp.updateValueUnitLength(
                               value, CSSPropertyParser::AllowPercent)) {
                    if (size.hasKeyword() || size.hasSecondRadius()) {
                        return false;
                    }
                    if (!size.hasFirstRadius()) {
                        size.setFirstRadius(temp);
                    } else {
                        size.setSecondRadius(temp);
                    }
                } else if (inPositionStr || value == "at") {
                    if (!inPositionStr) {
                        inPositionStr = true;
                    } else {
                        tokens.push_back(value);
                    }
                } else {
                    return false;
                }
            }

            if ((shape == RadialGradientShape::Circle &&
                 size.hasSecondRadius()) ||
                (tokens.size() > 4)) {
                return false;
            }

            if (shape != RadialGradientShape::None) {
                radialGradientValue->setShape(shape);
            }
            if (size.hasValue()) {
                radialGradientValue->setSize(size);
            }
            if (inPositionStr) {
                if (CSSStyleDeclaration::parseUnitPositionShorthand(
                        tokens, CSSStyleValuePair::BackgroundPosition,
                        &positionX, &positionY, false)) {
                    radialGradientValue->setPositionX(positionX);
                    radialGradientValue->setPositionY(positionY);
                } else {
                    return false;
                }
            }
        }
        gradient = radialGradientValue;
    } else {
        return false;
    }

    // Parse color-stops
    while (parser.consumeWhitespaces() && *(parser.curPos()) != ')') {
        CSSColorStop* cs = new CSSColorStop();

        while (parser.consumeWhitespaces() && *(parser.curPos()) != ',' &&
               *(parser.curPos()) != ')') {
            CSSStyleValuePair color;
            CSSStyleValuePair length;
            parser.consumeString(CSSPropertyParser::AllowSharp);
            String* ps = parser.parsedStringToGCString();
            if (*parser.curPos() == '(') {
                parser.consumeParenthesis();
                ps = ps->concat(parser.parsedStringToGCString());
            }

            CSSTokenValue value(ps->toUTF8NonGCString().data());
            if (!color.updateValueUnitColor(value)) {
                return false;
            }

            cs->setColor(color);
            uint32_t option = CSSPropertyParser::AllowNegative |
                              CSSPropertyParser::AllowPercent |
                              CSSPropertyParser::AllowDot;
            parser.consumeWhitespaces();
            if (*(parser.curPos()) != ',' && *(parser.curPos()) != ')') {
                parser.consumeString(option);
                ps = parser.parsedStringToGCString();
                if (ps->equals("calc")) {
                    String* calcStr = String::createASCIIString("calc(");
                    parser.consumeParenthesisBlock();
                    value =
                        CSSTokenValue("calc(" + parser.parsedString() + ")");
                    if (!length.updateValueUnitLengthOrCalc(value, option)) {
                        return false;
                    }
                } else {
                    value = CSSTokenValue(ps->toUTF8NonGCString().data());
                    if (!length.updateValueUnitLength(value, option)) {
                        return false;
                    }
                }
                cs->setOffset(length);
            }
            gradient->cssColorStopList().push_back(cs);
        }
        parser.consumeIfNext(',');
    }
    if (gradient->cssColorStopList().size() < 2) {
        return false;
    }

    m_value.m_gradientValue = gradient;
    m_valueKind = CSSStyleValuePair::ValueKind::GradientValueKind;
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundImage(const CSSTokenVector& tokens,
                                                   bool allowComma)
{
    bool shouldBeComma = false;
    ValueList* values = new ValueList(Separator::CommaSeparator);
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || !shouldBeComma) {
                return false;
            }
            shouldBeComma = false;
            continue;
        }
        CSSStyleValuePair ret;
        if (shouldBeComma || !ret.updateValueUnitImageValue(value)) {
            return false;
        }
        shouldBeComma = true;
        values->push_back(ret);
    }
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = values;
    return shouldBeComma;
}

bool CSSStyleValuePair::updateValueBackgroundImage(Document* document,
                                                   const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBackgroundImage(tokens, true);
}

bool CSSStyleValuePair::updateValueCursor(Document* document,
                                          const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);
    STARFISH_UNSUPPORTED("css property: cursor");
    return true;
}

static bool parseCounter(Document* document, const CSSTokenValue& s,
                         CSSStyleValuePair* pair)
{
    STARFISH_ASSERT(pair != nullptr);

    auto ss = s.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());
    parser.consumeString(0);
    if (!(parser.parsedString() == "counter")) {
        return false;
    }
    if (!parser.consumeIfNext('(')) {
        return false;
    }
    // Argument: name
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* name = parser.parsedStringToGCString();
    if (!CSSPropertyParser::stringIsIdent(name)) {
        return false;
    }
    AtomicString aname =
        AtomicString::createAtomicString(document->starfish(), name);
    parser.consumeWhitespaces();
    if (parser.consumeIfNext(')') && parser.isEnd()) {
        pair->setCounterFunctionValue(new CSSCounterFunction(aname));
        return true;
    }
    // Argument: style
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* style = parser.parsedStringToGCString();
    if (!CSSPropertyParser::stringIsIdent(style)) {
        return false;
    }
    AtomicString astyle =
        AtomicString::createAtomicString(document->starfish(), style);
    parser.consumeWhitespaces();
    if (!parser.consumeIfNext(')') || !parser.isEnd()) {
        return false;
    }
    auto value = new CSSCounterFunction(aname);
    value->setStyle(astyle);
    pair->setCounterFunctionValue(value);
    return true;
}

static bool parseCounters(Document* document, const CSSTokenValue& s,
                          CSSStyleValuePair* pair)
{
    STARFISH_ASSERT(pair != nullptr);

    auto ss = s.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());
    parser.consumeString(0);
    if (!(parser.parsedString() == "counters")) {
        return false;
    }
    if (!parser.consumeIfNext('(')) {
        return false;
    }
    // Argument: name
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* name = parser.parsedStringToGCString();
    if (!CSSPropertyParser::stringIsIdent(name)) {
        return false;
    }
    AtomicString aname =
        AtomicString::createAtomicString(document->starfish(), name);
    parser.consumeWhitespaces();
    // Argument: separator
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    if (!parser.consumeContentString()) {
        return false;
    }
    String* separator = parser.parsedStringToGCString();
    parser.consumeWhitespaces();
    if (parser.consumeIfNext(')') && parser.isEnd()) {
        auto value = new CSSCounterFunction(aname);
        value->setSeparator(separator);
        pair->setCounterFunctionValue(value);
        return true;
    }
    // Argument: style
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    parser.consumeWhitespaces();
    parser.consumeString(CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowUnderline);
    String* style = parser.parsedStringToGCString();
    if (!CSSPropertyParser::stringIsIdent(style)) {
        return false;
    }
    AtomicString astyle =
        AtomicString::createAtomicString(document->starfish(), style);
    parser.consumeWhitespaces();
    if (!parser.consumeIfNext(')') || !parser.isEnd()) {
        return false;
    }
    auto value = new CSSCounterFunction(aname);
    value->setSeparator(separator);
    value->setStyle(astyle);
    pair->setCounterFunctionValue(value);
    return true;
}

bool CSSStyleValuePair::updateValueContent(Document* document,
                                           const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    ValueList* values = new ValueList(Separator::SpaceSeparator);
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        CSSStyleValuePair ret;
        if (!ret.updateValueUnitUrlOrNone(value)) {
            CSSPropertyParser parser((char*)tokens[i].data());
            if (value.equals("normal")) {
                ret.m_valueKind = CSSStyleValuePair::ValueKind::Normal;
            } else if (value.equals("open-quote")) {
                ret.setQuoteValue(QuoteValue::OpenQuoteValue);
            } else if (value.equals("close-quote")) {
                ret.setQuoteValue(QuoteValue::CloseQuoteValue);
            } else if (value.equals("no-open-quote")) {
                ret.setQuoteValue(QuoteValue::NoOpenQuoteValue);
            } else if (value.equals("no-close-quote")) {
                ret.setQuoteValue(QuoteValue::NoCloseQuoteValue);
            } else if (parser.parseContentString(
                           value.data(), value.length(),
                           &(ret.m_value.m_stringValue))) {
                ret.m_valueKind = CSSStyleValuePair::ValueKind::StringValueKind;
            } else if (parser.parseAttr(value.data(), value.length(),
                                        &(ret.m_value.m_stringValue))) {
                ret.m_valueKind = CSSStyleValuePair::ValueKind::Attr;
            } else if (!parseCounter(document, value, &ret) &&
                       !parseCounters(document, value, &ret)) {
                // TODO: Consider various value types of the 'content' property.
                // https://www.w3.org/TR/CSS2/generate.html#content
                STARFISH_UNIMPLEMENTED();
                return false;
            }
        }
        values->push_back(ret);
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = values;
    return true;
}

bool CSSStyleValuePair::updateValueBorderImageRepeat(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueUnitBorderImageRepeat(tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderImageRepeat(
    const CSSTokenVector& tokens)
{
    size_t len = tokens.size();
    if (len < 1 || len > 2) {
        return false;
    }
    ValueList* values = new ValueList(Separator::SpaceSeparator);
    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        CSSStyleValuePair pair;
        if (value.equals("stretch")) {
            pair.setBorderImageRepeatValue(
                BorderImageRepeatValue::StretchValue);
        } else if (value.equals("repeat")) {
            pair.setBorderImageRepeatValue(BorderImageRepeatValue::RepeatValue);
        } else if (value.equals("round")) {
            pair.setBorderImageRepeatValue(BorderImageRepeatValue::RoundValue);
        } else if (value.equals("space")) {
            pair.setBorderImageRepeatValue(BorderImageRepeatValue::SpaceValue);
        } else {
            STARFISH_UNSUPPORTED("css property: border-image-repeat with %s",
                                 value.data());
            return false;
        }
        values->push_back(pair);
    }

    setValueList(values);

    return true;
}

bool CSSStyleValuePair::updateValueBorderImageSource(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitBorderImageSource(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitBorderImageSource(
    const CSSTokenValue& value)
{
    if (updateValueUnitUrlOrNone(value) || updateValueUnitGradient(value)) {
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueUnitBorderWidth(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
    if (value.equals("thick")) {
        m_value.m_borderWidth = BorderWidthValue::ThickBorderWidthValue;
    } else if (value.equals("thin")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
        m_value.m_borderWidth = BorderWidthValue::ThinBorderWidthValue;
    } else if (value.equals("medium")) {
        m_valueKind = CSSStyleValuePair::ValueKind::BorderWidthValueKind;
        m_value.m_borderWidth = BorderWidthValue::MediumBorderWidthValue;
    } else {
        return CSSPropertyParser::parseLength(value.c_str(), 0, this);
    }
    return true;
}

#define UPDATE_VALUE_BORDER_WIDTH(POS, ...)                \
    bool CSSStyleValuePair::updateValueBorder##POS##Width( \
        Document* document, const CSSTokenVector& tokens)  \
    {                                                      \
        if (tokens.size() != 1) {                          \
            return false;                                  \
        }                                                  \
        return updateValueUnitBorderWidth(tokens[0]);      \
    }
GEN_FOURSIDE(UPDATE_VALUE_BORDER_WIDTH)
#undef UPDATE_VALUE_BORDER_WIDTH

bool CSSStyleValuePair::updateValueBorderBlockStartWidth(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderWidth(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderBlockEndWidth(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderWidth(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderInlineStartWidth(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderWidth(tokens[0]);
}

bool CSSStyleValuePair::updateValueBorderInlineEndWidth(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderWidth(tokens[0]);
}

bool CSSStyleValuePair::updateValueNumber(const CSSTokenVector& tokens,
                                          uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitNumber(tokens[0], option);
}

bool CSSStyleValuePair::updateValueUnitNumber(const CSSTokenValue& token,
                                              uint8_t option)
{
    float f;
    m_valueKind = CSSStyleValuePair::ValueKind::Number;
    if (CSSPropertyParser::parseNumber(token.data(), token.length(), option,
                                       &f)) {
        m_value.m_floatValue = f;
        return true;
    }
    return false;
}

static bool parseCalc(CSSPropertyParser& parser, CalcData* data,
                      bool isLenParser, bool isAngleParser, bool isTimeParser,
                      bool isLineheightParser, uint8_t parserOption)
{
    STARFISH_ASSERT(data != nullptr);

    bool isPlus = true;
    size_t currentArgc = 1;
    // start default arguments start position
    data->addArgumentsStartPostion(0);
    while (true) {
        CalcTerm* term = new CalcTerm();
        bool isMul = false;
        bool unitParsed = false;
        // TODO : The type checking of unit needs to be reconsidered.
        // https://www.w3.org/TR/css-values-3/#calc-type-checking
        while (!parser.isEnd()) {
            parser.consumeWhitespaces();

            CSSStyleValuePair ret;
            char* pos = parser.curPos();
            parser.consumeString(CSSPropertyParser::AllowDot |
                                 CSSPropertyParser::AllowPlus |
                                 CSSPropertyParser::AllowNegative |
                                 CSSPropertyParser::AllowPercent);

            auto str = parser.parsedString();
            CalcValue val;
            if (parser.consumeIfNext('(')) {
                CalcData* newData = new CalcData(str);
                if (parseCalc(parser, newData, isLenParser, isAngleParser,
                              isTimeParser, isLineheightParser, parserOption)) {
                    if (!isPlus) {
                        newData->setSign(false);
                        isPlus = true;
                    }
                    val.setType(CalcValueType::ValueKind::kCalcData);
                    val.setValue(newData);

                } else {
                    return false;
                }
            } else if (isLenParser &&
                       ret.updateValueUnitLength(CSSTokenValue(str),
                                                 parserOption) &&
                       str != "0") {
                if (unitParsed) {
                    return false;
                }
                unitParsed = true;
                if (isPlus) {
                    if (ret.valueKind() ==
                        CSSStyleValuePair::ValueKind::Percentage) {
                        val.setType(CalcValueType::ValueKind::kPercentage);
                        val.setValue(ret.percentageValue());
                    } else {
                        val.setType(CalcValueType::ValueKind::kLength);
                        val.setValue(ret.cssLengthValue());
                    }
                } else {
                    if (ret.valueKind() ==
                        CSSStyleValuePair::ValueKind::Percentage) {
                        val.setType(CalcValueType::ValueKind::kPercentage);
                        val.setValue(-1 * ret.percentageValue());
                    } else {
                        val.setType(CalcValueType::ValueKind::kLength);
                        val.setValue(-1 * ret.cssLengthValue());
                    }
                    isPlus = true;
                }
            } else if (isTimeParser && ret.updateValueUnitTime(
                                           CSSTokenValue(str), parserOption)) {
                if (unitParsed) {
                    return false;
                }
                unitParsed = true;
                val.setType(CalcValueType::ValueKind::kTime);
                if (isPlus) {
                    val.setValue(ret.timeValue());
                } else {
                    val.setValue(-1 * ret.timeValue());
                    isPlus = true;
                }
            } else if (isAngleParser && ret.updateValueUnitAngle(
                                            CSSTokenValue(str), parserOption)) {
                if (unitParsed) {
                    return false;
                }
                unitParsed = true;
                val.setType(CalcValueType::ValueKind::kAngle);
                if (isPlus) {
                    val.setValue(ret.angleValue());
                } else {
                    val.setValue(-1 * ret.angleValue());
                    isPlus = true;
                }
            } else if (isLineheightParser &&
                       ret.updateValueUnitLineHeight(CSSTokenValue(str))) {
                if (unitParsed) {
                    parser.swap(pos);
                    if (parser.consumeNumber()) {
                        float num = parser.parsedNumber();
                        val.setType(CalcValueType::ValueKind::kNumber);
                        if (isPlus) {
                            val.setValue(num);
                        } else {
                            val.setValue(-1 * num);
                            isPlus = true;
                        }
                    } else {
                        return false;
                    }
                }

                if (isPlus) {
                    if (ret.valueKind() ==
                        CSSStyleValuePair::ValueKind::Percentage) {
                        val.setType(CalcValueType::ValueKind::kPercentage);
                        val.setValue(ret.percentageValue());
                        unitParsed = true;
                    } else if (ret.valueKind() ==
                               CSSStyleValuePair::ValueKind::Number) {
                        val.setType(CalcValueType::ValueKind::kNumber);
                        val.setValue(ret.numberValue());
                    } else {
                        val.setType(CalcValueType::ValueKind::kLength);
                        val.setValue(ret.cssLengthValue());
                        unitParsed = true;
                    }
                } else {
                    if (ret.valueKind() ==
                        CSSStyleValuePair::ValueKind::Percentage) {
                        val.setType(CalcValueType::ValueKind::kPercentage);
                        val.setValue(-1 * ret.percentageValue());
                        unitParsed = true;
                    } else if (ret.valueKind() ==
                               CSSStyleValuePair::ValueKind::Number) {
                        val.setType(CalcValueType::ValueKind::kNumber);
                        val.setValue(-1 * ret.numberValue());
                    } else {
                        val.setType(CalcValueType::ValueKind::kLength);
                        val.setValue(-1 * ret.cssLengthValue());
                        unitParsed = true;
                    }
                    isPlus = true;
                }
            } else {
                parser.swap(pos);
                if (parser.consumeNumber()) {
                    float num = parser.parsedNumber();
                    val.setType(CalcValueType::ValueKind::kNumber);
                    if (isPlus) {
                        val.setValue(num);
                    } else {
                        val.setValue(-1 * num);
                        isPlus = true;
                    }
                } else {
                    return false;
                }
            }

            if (term->hasValue()) {
                term->appendValue(isMul, val);
            } else {
                term->appendValue(val);
            }

            parser.consumeWhitespaces();

            if (parser.consumeIfNext('*')) {
                isMul = true;
            } else if (parser.consumeIfNext('/')) {
                isMul = false;
            } else {
                break;
            }
        }

        data->appendTerm(term);

        parser.consumeWhitespaces();

        if (parser.consumeIfNext('+')) {
            isPlus = true;
        } else if (parser.consumeIfNext('-')) {
            isPlus = false;
        } else if (data->type() != CalcData::Type::kCalc &&
                   parser.consumeIfNext(',')) {
            isPlus = true;
            currentArgc++;
            if (currentArgc > data->requiredArguemntsCount()) {
                return false;
            }
            data->addArgumentsStartPostion(data->terms().size());
        } else if (parser.consumeIfNext(')')) {
            break;
        }

        if (parser.isEnd()) {
            return false;
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitCalc(const CSSTokenValue& token,
                                            uint8_t calcParserOption,
                                            uint8_t parserOption)
{
    bool isLenParser =
        (calcParserOption == CSSStyleValuePair::CalcParserOption::LengthParser);
    bool isAngleParser =
        (calcParserOption == CSSStyleValuePair::CalcParserOption::AngleParser);
    bool isTimeParser =
        (calcParserOption == CSSStyleValuePair::CalcParserOption::TimeParser);
    bool isLineheightParser =
        (calcParserOption ==
         CSSStyleValuePair::CalcParserOption::LineheightParser);

    CSSPropertyParser parser((char*)token.data());

    parser.consumeString(0);
    const char* calcHeader = "calc";
    CSSTokenValue name = parser.parsedString();
    if (name == calcHeader || name == "max" || name == "min" ||
        name == "clamp") {
        if (!parser.consumeIfNext('(')) {
            return false;
        }

        CalcData* data = new CalcData(name);
        bool result = parseCalc(parser, data, isLenParser, isAngleParser,
                                isTimeParser, isLineheightParser, parserOption);
        if (result && parser.isEnd()) {
            m_valueKind = CSSStyleValuePair::ValueKind::CalcValueKind;
            m_value = data;
            return true;
        }

        CSSTokenVector tokens;
        // The css token will be parsed except for "calc(" and ")".
        size_t calcHeaderSize = strlen(calcHeader) + 1;
        CSSStyleDeclaration::tokenizeCSSValue(
            tokens, token.c_str() + calcHeaderSize,
            token.size() - (calcHeaderSize + 1));
        if (updateValueVarReferences(tokens)) {
            m_valueKind = CSSStyleValuePair::ValueKind::VarFunctionValueKind;
            m_temporaryValueKind = CSSStyleValuePair::ValueKind::CalcValueKind;
            m_value = String::fromUTF8(token.c_str(), token.size());
            return true;
        }
    }

    return false;
}

bool CSSStyleValuePair::updateValueUnitLength(const CSSTokenValue& token,
                                              uint8_t option)
{
    bool allowAuto = (option & CSSPropertyParser::ParserOption::AllowAuto);
    bool allowNone = (option & CSSPropertyParser::ParserOption::AllowNone);

    if (allowAuto) {
        if (token.equals("auto")) {
            m_valueKind = CSSStyleValuePair::ValueKind::Auto;
            return true;
        }
    }

    if (allowNone) {
        if (token.equals("none")) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
            return true;
        }
    }

    return CSSPropertyParser::parseLength(token.data(), option, this);
}

bool CSSStyleValuePair::updateValueUnitLengthOrCalc(const CSSTokenValue& token,
                                                    uint8_t option)
{
    if (updateValueUnitLength(token, option)) {
        return true;
    } else {
        option = option & ~CSSPropertyParser::ParserOption::AllowAuto;
        option = option & ~CSSPropertyParser::ParserOption::AllowNone;
        option = option & ~CSSPropertyParser::ParserOption::AllowWithoutUnit;

        return updateValueUnitCalc(
            token, CSSStyleValuePair::CalcParserOption::LengthParser, option);
    }
}

bool CSSStyleValuePair::updateValueLength(const CSSTokenVector& tokens,
                                          uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLengthOrCalc(tokens[0], option);
}

bool CSSStyleValuePair::updateValueUnitTime(const CSSTokenValue& token,
                                            uint8_t option)
{
    return CSSPropertyParser::parseTime(token.data(), option, this);
}

bool CSSStyleValuePair::updateValueUnitTimeOrCalc(const CSSTokenValue& token,
                                                  uint8_t option)
{
    if (updateValueUnitTime(token, option)) {
        return true;
    } else {
        return updateValueUnitCalc(
            token, CSSStyleValuePair::CalcParserOption::TimeParser, option);
    }
}

bool CSSStyleValuePair::updateValueTime(const CSSTokenVector& tokens,
                                        uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitTimeOrCalc(tokens[0], option);
}

bool CSSStyleValuePair::updateValueUnitAngle(const CSSTokenValue& token,
                                             uint8_t option)
{
    return CSSPropertyParser::parseAngle(token.data(), option, this);
}

bool CSSStyleValuePair::updateValueUnitAngleOrCalc(const CSSTokenValue& token,
                                                   uint8_t option)
{
    if (updateValueUnitAngle(token, option)) {
        return true;
    } else {
        return updateValueUnitCalc(
            token, CSSStyleValuePair::CalcParserOption::AngleParser, option);
    }
}

bool CSSStyleValuePair::updateValueAngle(const CSSTokenVector& tokens,
                                         uint8_t option)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAngleOrCalc(tokens[0], option);
}

bool CSSStyleValuePair::updateValueBorderImageOutset(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueUnitBorderImageOutset(tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderImageOutset(
    const CSSTokenVector& tokens)
{
    // [ <length> | <number>]{1,4}
    size_t size = tokens.size();
    if (size < 1 || size > 4) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (CSSPropertyParser::parseNumber(value.data(), value.length(), 0,
                                           &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(value, 0)) {
                return false;
            }
            values->push_back(ret);
        }
    }
    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueBorderImageWidth(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueUnitBorderImageWidth(tokens);
}
bool CSSStyleValuePair::updateValueUnitBorderImageWidth(
    const CSSTokenVector& tokens)
{
    // [ <length-percentage> | <number> | auto ]{1,4}
    size_t size = tokens.size();
    if (size < 1 || size > 4) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (CSSPropertyParser::parseNumber(value.data(), value.length(), 0,
                                           &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(
                    value, CSSPropertyParser::AllowPercent |
                               CSSPropertyParser::AllowAuto)) {
                return false;
            }
            values->push_back(ret);
        }
    }
    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueUnitPositionX(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
    if (value.equals("left")) {
        m_value.m_side = SideValue::LeftSideValue;
    } else if (value.equals("right")) {
        m_value.m_side = SideValue::RightSideValue;
    } else if (value.equals("center")) {
        m_value.m_side = SideValue::CenterSideValue;
    } else if (updateValueUnitLengthOrCalc(
                   value, CSSPropertyParser::AllowNegative |
                              CSSPropertyParser::AllowPercent)) {
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitPositionY(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::SideValueKind;
    if (value.equals("top")) {
        m_value.m_side = SideValue::TopSideValue;
    } else if (value.equals("bottom")) {
        m_value.m_side = SideValue::BottomSideValue;
    } else if (value.equals("center")) {
        m_value.m_side = SideValue::CenterSideValue;
    } else if (updateValueUnitLengthOrCalc(
                   value, CSSPropertyParser::AllowNegative |
                              CSSPropertyParser::AllowPercent)) {
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundPositionX(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitPositionX(tokens[0]);
}

bool CSSStyleValuePair::updateValueBackgroundPositionY(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitPositionY(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitBackgroundAttachment(
    const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BackgroundAttachmentValueKind;
    if (value.equals("scroll")) {
        m_value.m_backgroundAttachment =
            BackgroundAttachmentValue::ScrollBackgroundAttachmentValue;
    } else if (value.equals("fixed")) {
        m_value.m_backgroundAttachment =
            BackgroundAttachmentValue::FixedBackgroundAttachmentValue;
    } else if (value.equals("local")) {
        m_value.m_backgroundAttachment =
            BackgroundAttachmentValue::LocalBackgroundAttachmentValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundAttachment(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBackgroundAttachment(tokens, true);
}

bool CSSStyleValuePair::updateValueBackgroundAttachment(
    const CSSTokenVector& tokens, bool allowComma)
{
    size_t len = 0;
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    setValueList(new ValueList(Separator::CommaSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1)
                return false;
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        CSSStyleValuePair ret;
        if (len == 1) {
            if (!ret.updateValueUnitBackgroundAttachment(tokens[i - 1])) {
                return false;
            }
        } else {
            return false;
        }
        len = 0;
        multiValue()->push_back(ret);
    }

    return true;
}

bool CSSStyleValuePair::updateValueUnitBox(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::BoxValueKind;
    if (value.equals("border-box")) {
        m_value.m_box = BoxValue::BorderBoxBoxValue;
    } else if (value.equals("padding-box")) {
        m_value.m_box = BoxValue::PaddingBoxBoxValue;
    } else if (value.equals("content-box")) {
        m_value.m_box = BoxValue::ContentBoxBoxValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBackgroundClip(Document* document,
                                                  const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBox(tokens, true);
}

bool CSSStyleValuePair::updateValueBackgroundOrigin(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBox(tokens, true);
}

bool CSSStyleValuePair::updateValueBox(const CSSTokenVector& tokens,
                                       bool allowComma)
{
    size_t len = 0;
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    setValueList(new ValueList(Separator::CommaSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1)
                return false;
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        CSSStyleValuePair ret;
        if (len == 1) {
            if (!ret.updateValueUnitBox(tokens[i - 1])) {
                return false;
            }
        } else {
            return false;
        }
        len = 0;
        multiValue()->push_back(ret);
    }

    return true;
}

bool CSSStyleValuePair::updateValueBackgroundSize(Document* document,
                                                  const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueBackgroundSize(tokens, true);
}

bool CSSStyleValuePair::updateValueBackgroundSize(const CSSTokenVector& tokens,
                                                  bool allowComma)
{
    // [<bg-size> = [ <length-percentage [0,∞]> | auto ]{1,2} | cover | contain.
    size_t len = 0;
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    setValueList(new ValueList(Separator::CommaSeparator));

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || i == 0 || i == tokens.size() - 1)
                return false;
        } else if (i == tokens.size() - 1) {
            len++;
            i++;
        } else {
            len++;
            continue;
        }

        uint8_t option =
            CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowAuto;
        CSSStyleValuePair ret;
        if (len == 1) {
            if (value.equals("cover")) {
                ret.setBackgroundSizeValue(
                    BackgroundSizeValue::CoverBackgroundSizeValue);
            } else if (value.equals("contain")) {
                ret.setBackgroundSizeValue(
                    BackgroundSizeValue::ContainBackgroundSizeValue);
            } else {
                ret.setValueList(new ValueList(Separator::SpaceSeparator));
                CSSStyleValuePair r;
                if (!r.updateValueUnitLengthOrCalc(tokens[i - 1], option)) {
                    return false;
                }
                ret.multiValue()->push_back(r);
            }
        } else if (len == 2) {
            ret.setValueList(new ValueList(Separator::SpaceSeparator));
            CSSStyleValuePair r1, r2;
            if (!r1.updateValueUnitLengthOrCalc(tokens[i - 2], option) ||
                !r2.updateValueUnitLengthOrCalc(tokens[i - 1], option)) {
                return false;
            }
            ret.multiValue()->push_back(r1);
            ret.multiValue()->push_back(r2);
        } else {
            return false;
        }
        len = 0;
        multiValue()->push_back(ret);
    }

    return true;
}

bool CSSStyleValuePair::updateValueBorderImageSlice(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueUnitBorderImageSlice(tokens);
}

bool CSSStyleValuePair::updateValueUnitBorderImageSlice(
    const CSSTokenVector& tokens)
{
    // [<number> | <percentage>]{1,4} && fill?
    size_t size = tokens.size();
    if (size < 1 || size > 5) {
        return false;
    }

    CSSStyleValuePair fill;
    bool isFill = false;
    if (tokens[size - 1].equals("fill")) {
        if (size == 1) {
            return false;
        }
        isFill = true;
        fill.setKeywordValue(String::fromUTF8("fill"));
        size--;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (value.equals("fill")) {
            return false;
        } else if (CSSPropertyParser::parseNumber(value.data(), value.length(),
                                                  0, &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(
                    value, CSSPropertyParser::AllowPercent)) {
                return false;
            }
            if (ret.valueKind() != CSSStyleValuePair::ValueKind::Percentage) {
                return false;
            }
            values->push_back(ret);
        }
    }

    if (isFill) {
        values->push_back(fill);
    }

    m_value.m_multiValue = values;

    return true;
}

bool CSSStyleValuePair::updateValueFontSize(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitFontSize(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitFontSize(const CSSTokenValue& value)
{
    // absolute-size | relative-size | length | percentage | inherit // initial
    // value -> medium
    //        O      |       O       |   O    |    O       |    O
    m_valueKind = CSSStyleValuePair::ValueKind::FontSizeValueKind;
    if (value.equals("xx-small")) {
        m_value.m_fontSize = FontSizeValue::XXSmallFontSizeValue;
    } else if (value.equals("x-small")) {
        m_value.m_fontSize = FontSizeValue::XSmallFontSizeValue;
    } else if (value.equals("small")) {
        m_value.m_fontSize = FontSizeValue::SmallFontSizeValue;
    } else if (value.equals("medium")) {
        m_value.m_fontSize = FontSizeValue::MediumFontSizeValue;
    } else if (value.equals("large")) {
        m_value.m_fontSize = FontSizeValue::LargeFontSizeValue;
    } else if (value.equals("x-large")) {
        m_value.m_fontSize = FontSizeValue::XLargeFontSizeValue;
    } else if (value.equals("xx-large")) {
        m_value.m_fontSize = FontSizeValue::XXLargeFontSizeValue;
    } else if (value.equals("larger")) {
        m_value.m_fontSize = FontSizeValue::LargerFontSizeValue;
    } else if (value.equals("smaller")) {
        m_value.m_fontSize = FontSizeValue::SmallerFontSizeValue;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowPercent);
    }
    return true;
}

bool CSSStyleValuePair::updateValueLineHeight(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLineHeight(tokens[0]);
}

bool CSSStyleValuePair::updateValueLineClamp(Document* document,
                                             const CSSTokenVector& tokens)
{
    // https://www.w3.org/TR/css-overflow-3/#propdef-line-clamp
    // TODO : When the above specifications are confirmed, please complete the
    // implementation of line-clamp.
    return updateValueNumber(tokens, 0);
}

bool CSSStyleValuePair::updateValueUnitLineHeight(const CSSTokenValue& value)
{
    // <normal> | number | length | percentage | inherit
    float result = 0.f;
    if (value.equals("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    } else if (CSSPropertyParser::parseNumber(value.data(), value.length(), 0,
                                              &result)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
        m_value.m_floatValue = result;
        return true;
    } else {
        uint8_t option = CSSPropertyParser::AllowPercent;
        if (updateValueUnitLength(value, option)) {
            return true;
        } else {
            option = option | CSSPropertyParser::ParserOption::AllowWithoutUnit;
            return updateValueUnitCalc(
                value, CSSStyleValuePair::CalcParserOption::LineheightParser,
                option);
        }
    }
}

bool CSSStyleValuePair::updateValueUnitGap(const CSSTokenValue& value)
{
    return updateValueUnitLengthOrCalc(CSSTokenValue(value),
                                       CSSPropertyParser::AllowPercent);
}

#define UPDATE_VALUE_PADDING(POS, ...)                                     \
    bool CSSStyleValuePair::updateValuePadding##POS(                       \
        Document* document, const CSSTokenVector& tokens)                  \
    {                                                                      \
        return updateValueLength(tokens, CSSPropertyParser::AllowPercent); \
    }
GEN_FOURSIDE(UPDATE_VALUE_PADDING)
#undef UPDATE_VALUE_PADDING

bool CSSStyleValuePair::updateValuePaddingBlockStart(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValuePaddingBlockEnd(Document* document,
                                                   const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValuePaddingInlineEnd(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValuePaddingInlineStart(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

#define UPDATE_VALUE_SIDE(POS, ...)                                            \
    bool CSSStyleValuePair::updateValue##POS(Document* document,               \
                                             const CSSTokenVector& tokens)     \
    {                                                                          \
        return updateValueLength(tokens, CSSPropertyParser::AllowNegative |    \
                                             CSSPropertyParser::AllowPercent | \
                                             CSSPropertyParser::AllowAuto);    \
    }
GEN_FOURSIDE(UPDATE_VALUE_SIDE)
#undef UPDATE_VALUE_SIDE

#define UPDATE_VALUE_MARGIN(POS, ...)                                          \
    bool CSSStyleValuePair::updateValueMargin##POS(                            \
        Document* document, const CSSTokenVector& tokens)                      \
    {                                                                          \
        return updateValueLength(tokens, CSSPropertyParser::AllowNegative |    \
                                             CSSPropertyParser::AllowPercent | \
                                             CSSPropertyParser::AllowAuto);    \
    }
GEN_FOURSIDE(UPDATE_VALUE_MARGIN)
#undef UPDATE_VALUE_MARGIN

bool CSSStyleValuePair::updateValueMarginBlockStart(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueMarginBlockEnd(Document* document,
                                                  const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueMarginInlineEnd(Document* document,
                                                   const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueMarginInlineStart(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueWidthHeightKeyword(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitWidthHeightKeyword(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitWidthHeightKeyword(
    const CSSTokenValue& value)
{
    if (value.equals("available")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::AvailableValue;
    } else if (value.equals("min-content")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::MinContentValue;
    } else if (value.equals("max-content")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::MaxContentValue;
    } else if (value.equals("fit-content")) {
        m_value.m_widthHeightKeywordValue =
            WidthHeightKeywordValue::FitContentValue;
    } else {
        return false;
    }
    m_valueKind = CSSStyleValuePair::ValueKind::WidthHeightKeywordValueKind;
    return true;
}

bool CSSStyleValuePair::updateValueWidth(Document* document,
                                         const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    uint8_t options =
        CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowAuto;
    if (document->inQuirksMode()) {
        options |= CSSPropertyParser::AllowWithoutUnit;
    }
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, options);
}

bool CSSStyleValuePair::updateValueMaxWidth(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowNone);
}

bool CSSStyleValuePair::updateValueMinWidth(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueHeight(Document* document,
                                          const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    uint8_t options =
        CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowAuto;
    if (document->inQuirksMode()) {
        options |= CSSPropertyParser::AllowWithoutUnit;
    }
    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, options);
}

bool CSSStyleValuePair::updateValueMaxHeight(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowNone);
}

bool CSSStyleValuePair::updateValueMinHeight(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueWidthHeightKeyword(tokens) ||
           updateValueLength(tokens, CSSPropertyParser::AllowPercent |
                                         CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueVerticalAlign(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("baseline")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::BaselineVAlignValue;
    } else if (value.equals("sub")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::SubVAlignValue;
    } else if (value.equals("super")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::SuperVAlignValue;
    } else if (value.equals("top")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TopVAlignValue;
    } else if (value.equals("text-top")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TextTopVAlignValue;
    } else if (value.equals("middle")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::MiddleVAlignValue;
    } else if (value.equals("bottom")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::BottomVAlignValue;
    } else if (value.equals("text-bottom")) {
        m_valueKind = CSSStyleValuePair::ValueKind::VerticalAlignValueKind;
        m_value.m_verticalAlign = VerticalAlignValue::TextBottomVAlignValue;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowNegative |
                                               CSSPropertyParser::AllowPercent);
    }
    return true;
}

static bool parseRectFunctionPart(const CSSTokenValue& s, size_t* ret,
                                  GCVector<String*>& units)
{
    STARFISH_ASSERT(ret != nullptr);

    auto ss = s.trim();
    CSSPropertyParser parser((char*)ss.data(), ss.length());

    bool hasPoint = false;
    parser.consumeWhitespaces();
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();

    parser.consumeString(CSSPropertyParser::AllowWithoutUnit);

    String* str = parser.parsedStringToGCString();

    if (str->length() != 0 && !CSSPropertyParser::isLengthUnit(str)) {
        return false;
    }

    *ret = number;
    units.push_back(str);

    return true;
}

bool CSSStyleValuePair::updateValueClip(Document* document,
                                        const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    // https://www.w3.org/TR/css-masking-1/#clip-property
    if (tokens.size() != 1)
        return false;

    CSSTokenValue str = tokens[0];

    if (str.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else {
        // rect
        bool maybeRect = str.startsWith("rect(");

        if (!maybeRect)
            return false;

        size_t s1 = str.indexOf('(');
        size_t s2 = str.indexOf(')');

        if (s1 == SIZE_MAX || s2 != str.length() - 1 || s1 >= s2) {
            return false;
        }

        CSSTokenValue sub = str.substring(s1 + 1, s2 - s1 - 1);

        std::vector<CSSTokenValue> v;
        sub.split(',', v);

        size_t size = v.size();

        if (size != 4) {
            v.clear();
            sub.split(' ', v);
            size = v.size();
            if (size != 4) {
                return false;
            }
        }

        size_t value[4];
        GCVector<String*> units;
        for (size_t i = 0; i < size; i++) {
            if (!parseRectFunctionPart(v[i], &value[i], units)) {
                return false;
            }
        }

        setClipData(new RectData(CSSLength(units[0], value[0]).toLength(),
                                 CSSLength(units[1], value[1]).toLength(),
                                 CSSLength(units[2], value[2]).toLength(),
                                 CSSLength(units[3], value[3]).toLength()));
    }

    return true;
}

bool CSSStyleValuePair::updateValueClipPath(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);
    if (tokens.size() != 1)
        return false;
    return CSSPropertyParser::parseUrl(tokens[0].data(), this);
}

static bool parseMinMax(CSSTokenValue& token, GridLength& min, GridLength& max)
{
    CSSPropertyParser parser(const_cast<char*>(token.c_str()), token.length());
    parser.consumeString(0);
    parser.consumeIfNext('(');

    bool hasPoint = false;
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    float number = parser.parsedNumber();
    parser.consumeString(CSSPropertyParser::AllowWithoutUnit |
                         CSSPropertyParser::AllowPercent);
    const auto& str1 = parser.parsedString();
    if (str1.length() != 0 && (!CSSPropertyParser::isLengthUnit(str1) &&
                               !(str1 == "fr") && !(str1 == "%"))) {
        return false;
    }

    // TODO : Add the GridLine, GridArea and Repeat
    // Create GridTrack and push back into vector.
    if (str1 == "fr") {
        return false;
    } else if (CSSPropertyParser::isLengthUnit(str1)) {
        min = CSSLength(str1, number).toLength();
    } else {
        if (number == 0) {
            min = CSSLength("px", 0).toLength();
        } else {
            return false;
        }
    }

    parser.consumeWhitespaces();
    if (!parser.consumeIfNext(',')) {
        return false;
    }
    parser.consumeWhitespaces();

    hasPoint = false;
    if (!parser.consumeNumber(&hasPoint)) {
        return false;
    }

    number = parser.parsedNumber();
    parser.consumeString(CSSPropertyParser::AllowWithoutUnit |
                         CSSPropertyParser::AllowPercent);
    const auto& str2 = parser.parsedString();
    if (str2.length() != 0 && (!CSSPropertyParser::isLengthUnit(str2) &&
                               !(str2 == "fr") && !(str2 == "%"))) {
        return false;
    }

    // TODO : Add the GridLine, GridArea and Repeat
    // Create GridTrack and push back into vector.
    if (str2 == "fr") {
        max = number;
    } else if (CSSPropertyParser::isLengthUnit(str2)) {
        max = CSSLength(str2, number).toLength();
    } else {
        if (number == 0) {
            max = CSSLength("px", 0).toLength();
        } else {
            return false;
        }
    }

    parser.consumeWhitespaces();
    if (!parser.consumeIfNext(')')) {
        return false;
    }

    return true;
}

static bool parseRepeat(CSSTokenValue& str, GCVector<GridTrackSize*>* v)
{
    // https://drafts.csswg.org/css-grid/#track-sizing
    Optional<CSSTokenValue> repeat =
        CSSPropertyParser::parseFunctionBlock((char*)str.data(), "repeat");
    if (!repeat.hasValue()) {
        return false;
    }

    CSSTokenValue trimmed = repeat.getValue().trim();
    if (trimmed.startsWith("a")) {
        std::vector<CSSTokenValue> tokenValues;
        auto pos = trimmed.find(",");
        if (pos == std::string::npos) {
            return false;
        }
        auto repeatType = trimmed.substring(0, pos);
        auto repeatData =
            trimmed.substring(pos + 1, trimmed.length() - pos - 1);

        AutoRepeatType autoRepeatType;
        if (repeatType == "auto-fit") {
            autoRepeatType = AutoRepeatType::kAutoFit;
        } else if (repeatType == "auto-fill") {
            autoRepeatType = AutoRepeatType::kAutoFill;
        } else {
            return false;
        }

        CSSTokenVector tokens;
        CSSStyleDeclaration::tokenizeCSSValue(tokens, repeatData.c_str(),
                                              repeatData.length());
        GCVector<GridTrackSize*> gridTrackSizes;
        if (!parseGridTemplateRowsAndColumns(
                tokens, &gridTrackSizes,
                CSSPropertyParser::AllowNegative |
                    CSSPropertyParser::AllowPercent,
                false, false)) {
            return false;
        }
        GridTrackSizeAutoRepeat* autoRepeat =
            new GridTrackSizeAutoRepeat(gridTrackSizes, autoRepeatType);
        v->push_back(autoRepeat);
        return true;
    } else {
        CSSPropertyParser parser((char*)trimmed.data());
        int32_t count = 0;
        if (parser.consumeNumber()) {
            count = parser.parsedNumber();
            if (count < 0) {
                return false;
            }

            parser.consumeWhitespaces();
            if (parser.consumeIfNext(',')) {
                CSSTokenValue value = CSSTokenValue(
                    parser.m_curPos, parser.m_endPos - parser.m_curPos);
                const char* data = value.data();
                size_t len = std::strlen(data);

                CSSTokenVector tokens;
                CSSStyleDeclaration::tokenizeCSSValue(tokens, data, len);
                GCVector<GridTrackSize*> gridTrackSizes;
                if (!parseGridTemplateRowsAndColumns(
                        tokens, &gridTrackSizes,
                        CSSPropertyParser::AllowNegative |
                            CSSPropertyParser::AllowPercent |
                            CSSPropertyParser::AllowAuto,
                        false, true)) {
                    return false;
                }
                GridTrackSizeFixedRepeat* fixedRepeat =
                    new GridTrackSizeFixedRepeat(gridTrackSizes, count);
                v->push_back(fixedRepeat);
                return true;
            }
        }
    }
    return false;
}

static bool parseGridTemplateRowsAndColumns(const CSSTokenVector& tokens,
                                            GCVector<GridTrackSize*>* v,
                                            uint8_t option, bool allowRepeat,
                                            bool allowFlexible)
{
    STARFISH_ASSERT(v != nullptr);
    for (size_t i = 0; i < tokens.size(); i++) {
        // Trim token.
        CSSTokenValue token = tokens[i].trim();

        // Try to parse length, calc, auto.
        CSSStyleValuePair legnthOrCalc;
        if (legnthOrCalc.updateValueUnitLengthOrCalc(token, option)) {
            // Currently, 'var' is not supported in 'grid-template-rows'. ex)
            // "grid-template-rows : 1fr calc(var(--center-card-width) +
            // var(--center-pad)*2) 1fr " This is a temporary soluation to
            // prevent crashes.
            if (legnthOrCalc.valueKind() ==
                CSSStyleValuePair::ValueKind::VarFunctionValueKind) {
                return false;
            }
            Optional<Length> maybeLength = LengthUtil::convertValueToLength(
                legnthOrCalc.valueKind(), legnthOrCalc.value());
            if (!maybeLength.hasValue()) {
                return false;
            }
            v->push_back(new GridTrackSizeLength(maybeLength.value()));
        } else if (token == "min-content") {
            // Try to parse keyword: min-content.
            v->push_back(new GridTrackSizeMinContent());
        } else if (token == "max-content") {
            // Try to parse keyword: max-content.
            v->push_back(new GridTrackSizeMaxContent());
        } else if (token.startsWith("minmax(") &&
                   token[token.length() - 1] == ')') {
            // Try to minmax().
            GridLength min, max;
            if (!parseMinMax(token, min, max)) {
                return false;
            }
            v->push_back(new GridTrackSizeMinMax(min, max));
        } else if (allowRepeat && token.startsWith("repeat(") &&
                   token[token.length() - 1] == ')') {
            // repeat().
            if (!parseRepeat(token, v)) {
                return false;
            }
        } else if (allowFlexible) {
            // fr.
            CSSPropertyParser parser(const_cast<char*>(token.c_str()),
                                     token.length());
            bool hasPoint = false;
            if (!parser.consumeNumber(&hasPoint)) {
                return false;
            }
            float number = parser.parsedNumber();

            parser.consumeString(0);
            const auto& unit = parser.parsedString();
            if (unit != "fr") {
                return false;
            }
            v->push_back(new GridTrackSizeLength(GridLength(number)));
        } else {
            return false;
        }
    }

    return true;
}

bool CSSStyleValuePair::updateValueGridTemplateColumns(
    Document* document, const CSSTokenVector& tokens)
{
    // https://drafts.csswg.org/css-grid/#track-sizing
    // none | <track-list> | <auto-track-list> | subgrid <line-name-list>?
    STARFISH_ASSERT(document != nullptr);
    if (!tokens.size()) {
        return false;
    }

    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    }

    GCVector<GridTrackSize*>* v = new GCVector<GridTrackSize*>();
    ValueList* v1 = new ValueList(Separator::SpaceSeparator);
    if (!parseGridTemplateRowsAndColumns(tokens, v,
                                         CSSPropertyParser::AllowNegative |
                                             CSSPropertyParser::AllowPercent |
                                             CSSPropertyParser::AllowAuto,
                                         true, true)) {
        return false;
    }
    setGridTemplateUnits(v);

    return true;
}

bool CSSStyleValuePair::updateValueGridTemplateRows(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size())
        return false;

    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    }

    GCVector<GridTrackSize*>* v = new GCVector<GridTrackSize*>();

    if (!parseGridTemplateRowsAndColumns(tokens, v,
                                         CSSPropertyParser::AllowNegative |
                                             CSSPropertyParser::AllowPercent |
                                             CSSPropertyParser::AllowAuto,
                                         true, true)) {
        return false;
    }

    setGridTemplateUnits(v);

    return true;
}

static bool isValidForGridStartEnd(const CSSTokenVector& tokens)
{
    // Grammar = auto | <custom-ident> | [ <integer> && <custom-ident>?] |
    //           [ span && [ <integer> || <custom-ident>] ]
    if (!tokens.size()) {
        return false;
    }

    if (tokens[0].equals("auto")) {
        return true;
    }

    if (tokens.size() >= 2 && tokens[0].equals("span")) {
        auto ss = tokens[1].trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        parser.consumeWhitespaces();

        bool hasPoint = false;
        if (parser.consumeNumber(&hasPoint)) {
            if (hasPoint) {
                return false;
            }
            return true;
        } else if (parser.consumeString(CSSPropertyParser::AllowWithoutUnit)) {
            return true;
        } else {
            return false;
        }
    } else {
        auto ss = tokens[0].trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        parser.consumeWhitespaces();

        bool hasPoint = false;
        if (parser.consumeNumber(&hasPoint)) {
            if (hasPoint) {
                return false;
            }

            if (tokens.size() >= 2) {
                for (size_t i = 1; i < tokens.size(); i++) {
                    auto str = tokens[i].trim();
                    CSSPropertyParser sub((char*)str.data(), str.length());
                    sub.consumeWhitespaces();
                    if (!sub.consumeString(
                            CSSPropertyParser::AllowWithoutUnit)) {
                        return false;
                    }
                }
                return true;
            } else {
                return true;
            }
        } else if (parser.consumeString(CSSPropertyParser::AllowWithoutUnit)) {
            return true;
        }
    }

    return false;
}

bool CSSStyleValuePair::updateValueGridRowStart(Document* document,
                                                const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    bool result = isValidForGridStartEnd(tokens);

    if (result) {
        std::string str;
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            str += token;
            if (i != tokens.size() - 1) {
                str += " ";
            }
        }
        setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        setStringValue(String::createASCIIString(str.c_str(), str.size()));
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridRowEnd(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    bool result = isValidForGridStartEnd(tokens);

    if (result) {
        std::string str;
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            str += token;
            if (i != tokens.size() - 1) {
                str += " ";
            }
        }
        setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        setStringValue(String::createASCIIString(str.c_str(), str.size()));
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridColumnStart(Document* document,
                                                   const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    bool result = isValidForGridStartEnd(tokens);

    if (result) {
        std::string str;
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            str += token;
            if (i != tokens.size() - 1) {
                str += " ";
            }
        }
        setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        setStringValue(String::createASCIIString(str.c_str(), str.size()));
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridColumnEnd(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    bool result = isValidForGridStartEnd(tokens);

    if (result) {
        std::string str;
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& token = tokens[i];
            str += token;
            if (i != tokens.size() - 1) {
                str += " ";
            }
        }
        setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
        setStringValue(String::createASCIIString(str.c_str(), str.size()));
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridArea(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    std::string str;
    for (size_t i = 0; i < tokens.size(); i++) {
        auto ss = tokens[i];
        ss.trim();
        str += (ss + " ");
    }

    std::istringstream is(str);
    std::string part;

    std::vector<CSSTokenValue> parts;
    while (getline(is, part, '/')) {
        CSSTokenValue s(part.c_str());
        s = s.trim();
        parts.push_back(s);
    }

    bool result = true;
    setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    setValueList(new ValueList(Separator::SlashSeparator));
    CSSStyleValuePair rs;
    CSSStyleValuePair cs;
    CSSStyleValuePair re;
    CSSStyleValuePair ce;
    // First : row start, Second : column start
    // Third : row end, Fourth : column end
    if (parts.size() == 1) {
        if (parts[0].equals("auto")) {
            rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            rs.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(rs.valueKind(), rs.value());
            cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            cs.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(cs.valueKind(), cs.value());
            re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            re.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(re.valueKind(), re.value());
            ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            ce.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(ce.valueKind(), ce.value());
        } else {
            CSSTokenVector list;
            list.push_back(parts[0]);
            result = isValidForGridStartEnd(list);
            float x;
            if (CSSPropertyParser::parseNumber(parts[0].c_str(),
                                               parts[0].length(), 0, &x)) {
                if (result == true) {
                    rs.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    rs.setStringValue(String::createASCIIString(
                        parts[0].c_str(), parts[0].size()));
                    multiValue()->emplace_back(rs.valueKind(), rs.value());
                    cs.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    cs.setStringValue(String::createASCIIString("auto"));
                    multiValue()->emplace_back(cs.valueKind(), cs.value());
                    re.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    re.setStringValue(String::createASCIIString("auto"));
                    multiValue()->emplace_back(re.valueKind(), re.value());
                    ce.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    ce.setStringValue(String::createASCIIString("auto"));
                    multiValue()->emplace_back(ce.valueKind(), ce.value());
                }
            } else {
                if (result == true) {
                    rs.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    rs.setStringValue(String::createASCIIString(
                        parts[0].c_str(), parts[0].size()));
                    multiValue()->emplace_back(rs.valueKind(), rs.value());
                    cs.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    cs.setStringValue(String::createASCIIString(
                        parts[0].c_str(), parts[0].size()));
                    multiValue()->emplace_back(cs.valueKind(), cs.value());
                    re.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    re.setStringValue(String::createASCIIString(
                        parts[0].c_str(), parts[0].size()));
                    multiValue()->emplace_back(re.valueKind(), re.value());
                    ce.setValueKind(
                        CSSStyleValuePair::ValueKind::StringValueKind);
                    ce.setStringValue(String::createASCIIString(
                        parts[0].c_str(), parts[0].size()));
                    multiValue()->emplace_back(ce.valueKind(), ce.value());
                }
            }
        }
    } else if (parts.size() == 2) {
        CSSTokenVector list1, list2;
        list1.push_back(parts[0]);
        list2.push_back(parts[1]);
        result = isValidForGridStartEnd(list1);
        result = isValidForGridStartEnd(list2) & result;
        if (result == true) {
            rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            rs.setStringValue(
                String::createASCIIString(parts[0].c_str(), parts[0].size()));
            multiValue()->emplace_back(rs.valueKind(), rs.value());
            cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            cs.setStringValue(
                String::createASCIIString(parts[1].c_str(), parts[1].size()));
            multiValue()->emplace_back(cs.valueKind(), cs.value());
            re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            re.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(re.valueKind(), re.value());
            ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            ce.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(ce.valueKind(), ce.value());
        }
    } else if (parts.size() == 3) {
        CSSTokenVector list1, list2, list3;
        list1.push_back(parts[0]);
        list2.push_back(parts[1]);
        list3.push_back(parts[2]);
        result = isValidForGridStartEnd(list1);
        result = isValidForGridStartEnd(list2) & result;
        result = isValidForGridStartEnd(list3) & result;

        if (result == true) {
            rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            rs.setStringValue(
                String::createASCIIString(parts[0].c_str(), parts[0].size()));
            multiValue()->emplace_back(rs.valueKind(), rs.value());
            cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            cs.setStringValue(
                String::createASCIIString(parts[1].c_str(), parts[1].size()));
            multiValue()->emplace_back(cs.valueKind(), cs.value());
            re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            re.setStringValue(
                String::createASCIIString(parts[2].c_str(), parts[2].size()));
            multiValue()->emplace_back(re.valueKind(), re.value());
            ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            ce.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(ce.valueKind(), ce.value());
        }
    } else if (parts.size() == 4) {
        CSSTokenVector list1, list2, list3, list4;
        list1.push_back(parts[0]);
        list2.push_back(parts[1]);
        list3.push_back(parts[2]);
        list4.push_back(parts[3]);
        result = isValidForGridStartEnd(list1);
        result = isValidForGridStartEnd(list2) & result;
        result = isValidForGridStartEnd(list3) & result;
        result = isValidForGridStartEnd(list4) & result;

        if (result == true) {
            rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            rs.setStringValue(
                String::createASCIIString(parts[0].c_str(), parts[0].size()));
            multiValue()->emplace_back(rs.valueKind(), rs.value());
            cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            cs.setStringValue(
                String::createASCIIString(parts[1].c_str(), parts[1].size()));
            multiValue()->emplace_back(cs.valueKind(), cs.value());
            re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            re.setStringValue(
                String::createASCIIString(parts[2].c_str(), parts[2].size()));
            multiValue()->emplace_back(re.valueKind(), re.value());
            ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            ce.setStringValue(
                String::createASCIIString(parts[3].c_str(), parts[3].size()));
            multiValue()->emplace_back(ce.valueKind(), ce.value());
        }
    } else {
        result = false;
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridGap(Document* document,
                                           const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    if (tokens.size() > 2) {
        return false;
    }

    ValueList* values = new ValueList(Separator::SpaceSeparator);

    for (size_t i = 0; i < tokens.size(); i++) {
        auto ss = tokens[i];
        ss.trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        bool hasPoint = false;

        if (!parser.consumeNumber(&hasPoint)) {
            return false;
        }

        float number = parser.parsedNumber();
        parser.consumeString(CSSPropertyParser::AllowWithoutUnit);

        const auto& str = parser.parsedString();

        if (str.length() != 0 && !CSSPropertyParser::isLengthUnit(str)) {
            return false;
        }

        CSSLength length = CSSLength(str, number);
        CSSStyleValuePair ret;
        ret.setLengthValue(length);
        values->push_back(ret);
    }

    setValueList(values);

    return true;
}

bool CSSStyleValuePair::updateValueGridRow(Document* document,
                                           const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    std::string str;
    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& ss = tokens[i];
        str += (ss + " ");
    }

    std::istringstream is(str);
    std::string part;

    std::vector<CSSTokenValue> parts;
    while (getline(is, part, '/')) {
        CSSTokenValue s(part.c_str());
        s = s.trim();
        parts.push_back(s);
    }

    bool result = true;
    setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    setValueList(new ValueList(Separator::SlashSeparator));
    CSSStyleValuePair rs;
    CSSStyleValuePair re;

    if (parts.size() == 1) {
        if (parts[0].equals("auto")) {
            rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            rs.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(rs.valueKind(), rs.value());
            re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            re.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(re.valueKind(), re.value());
        } else {
            CSSTokenVector list;
            list.push_back(parts[0]);
            result = isValidForGridStartEnd(list);
            if (result == true) {
                rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
                rs.setStringValue(String::createASCIIString(parts[0].c_str(),
                                                            parts[0].size()));
                multiValue()->emplace_back(rs.valueKind(), rs.value());
                re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
                re.setStringValue(String::createASCIIString("auto"));
                multiValue()->emplace_back(re.valueKind(), re.value());
            }
        }
    } else if (parts.size() == 2) {
        CSSTokenVector list1, list2;
        list1.push_back(parts[0]);
        list2.push_back(parts[1]);
        result = isValidForGridStartEnd(list1);
        result = isValidForGridStartEnd(list2) & result;
        if (result == true) {
            rs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            rs.setStringValue(
                String::createASCIIString(parts[0].c_str(), parts[0].size()));
            multiValue()->emplace_back(rs.valueKind(), rs.value());
            re.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            re.setStringValue(
                String::createASCIIString(parts[1].c_str(), parts[1].size()));
            multiValue()->emplace_back(re.valueKind(), re.value());
        }
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridColumn(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    std::string str;
    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& ss = tokens[i];
        str += (ss + " ");
    }

    std::istringstream is(str);
    std::string part;

    std::vector<CSSTokenValue> parts;
    while (getline(is, part, '/')) {
        CSSTokenValue s(part.c_str());
        s = s.trim();
        parts.push_back(s);
    }

    bool result = true;
    setValueKind(CSSStyleValuePair::ValueKind::ValueListKind);
    setValueList(new ValueList(Separator::SlashSeparator));
    CSSStyleValuePair cs;
    CSSStyleValuePair ce;

    if (parts.size() == 1) {
        if (parts[0].equals("auto")) {
            cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            cs.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(cs.valueKind(), cs.value());
            ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            ce.setStringValue(String::createASCIIString("auto"));
            multiValue()->emplace_back(ce.valueKind(), ce.value());
        } else {
            CSSTokenVector list;
            list.push_back(parts[0]);
            result = isValidForGridStartEnd(list);
            if (result == true) {
                cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
                cs.setStringValue(String::createASCIIString(parts[0].c_str(),
                                                            parts[0].size()));
                multiValue()->emplace_back(cs.valueKind(), cs.value());
                ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
                ce.setStringValue(String::createASCIIString(parts[0].c_str(),
                                                            parts[0].size()));
                multiValue()->emplace_back(ce.valueKind(), ce.value());
            }
        }
    } else if (parts.size() == 2) {
        CSSTokenVector list1, list2;
        list1.push_back(parts[0]);
        list2.push_back(parts[1]);
        result = isValidForGridStartEnd(list1);
        result = isValidForGridStartEnd(list2) & result;
        if (result == true) {
            cs.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            cs.setStringValue(
                String::createASCIIString(parts[0].c_str(), parts[0].size()));
            multiValue()->emplace_back(cs.valueKind(), cs.value());
            ce.setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
            ce.setStringValue(
                String::createASCIIString(parts[1].c_str(), parts[1].size()));
            multiValue()->emplace_back(ce.valueKind(), ce.value());
        }
    }

    return result;
}

bool CSSStyleValuePair::updateValueGridTemplateAreas(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (!tokens.size()) {
        return false;
    }

    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    }

    size_t count = 0;
    std::unordered_multimap<std::string, GridAreaData> collector;
    std::unordered_set<std::string> areaSet;
    NamedGridAreaDataMap* namedGridAreaMap = new NamedGridAreaDataMap();

    for (size_t row = 0; row < tokens.size(); row++) {
        auto ss = tokens[row];
        ss.trim();
        CSSPropertyParser parser((char*)ss.data(), ss.length());
        parser.consumeContentString();
        const auto& separator = parser.parsedString();
        if (separator.length() == 0) {
            return false;
        }
        auto s = separator;
        CSSTokenVector areas;
        CSSStyleDeclaration::tokenizeCSSValue(areas, s.data(), s.length());
        for (size_t col = 0; col < areas.size(); col++) {
            std::string str = areas[col];
            GridAreaData area;
            area.columnStart = col + 1;
            area.columnEnd = area.columnStart + 1;
            area.rowStart = row + 1;
            area.rowEnd = area.rowStart + 1;
            collector.insert(std::make_pair(str, area));
            areaSet.insert(str);
        }
    }

    // Check the validation of 'grid-template-areas'.
    // eg. this is a valid case.
    // "head head"
    // "nav  nav"
    // "foot foot"
    // but this is a invalid case.
    // "head nav"
    // "nav  nav"
    // "foot foot"
    for (const std::string& name : areaSet) {
        std::vector<GridAreaData> stack;
        for (auto it = collector.find(name); it != collector.end(); it++) {
            if (name.compare(it->first)) {
                break;
            }

            if (!stack.size()) {
                stack.push_back(it->second);
            } else {
                bool merge = false;
                GridAreaData target = it->second;
                for (size_t i = 0; i < stack.size(); i++) {
                    GridAreaData* area = &stack[i];
                    std::set<size_t> set;
                    if (area->columnStart == target.columnStart &&
                        area->columnEnd == target.columnEnd) {
                        set.insert(area->rowStart);
                        set.insert(area->rowEnd);
                        set.insert(target.rowStart);
                        set.insert(target.rowEnd);

                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t> orderedTracks;
                        for (auto it = set.begin(); it != set.end(); ++it) {
                            orderedTracks.push_back(*it);
                        }

                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->rowStart = orderedTracks[0];
                        area->rowEnd = orderedTracks[2];
                        merge = true;
                        break;
                    } else if (area->rowStart == target.rowStart &&
                               area->rowEnd == target.rowEnd) {
                        set.insert(area->columnStart);
                        set.insert(area->columnEnd);
                        set.insert(target.columnStart);
                        set.insert(target.columnEnd);
                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t> orderedTracks;
                        for (auto it = set.begin(); it != set.end(); ++it) {
                            orderedTracks.push_back(*it);
                        }

                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->columnStart = orderedTracks[0];
                        area->columnEnd = orderedTracks[2];
                        merge = true;
                        break;
                    }
                }

                if (!merge) {
                    stack.push_back(it->second);
                }
            }
        }

        if (stack.size() != 1) {
            while (stack.size() != 1) {
                GridAreaData target = stack.back();
                stack.pop_back();
                bool merge = false;
                for (size_t i = 0; i < stack.size(); i++) {
                    GridAreaData* area = &stack[i];
                    std::set<size_t> set;
                    if (area->columnStart == target.columnStart &&
                        area->columnEnd == target.columnEnd) {
                        set.insert(area->rowStart);
                        set.insert(area->rowEnd);
                        set.insert(target.rowStart);
                        set.insert(target.rowEnd);

                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t> orderedTracks;
                        for (auto it = set.begin(); it != set.end(); ++it) {
                            orderedTracks.push_back(*it);
                        }

                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->rowStart = orderedTracks[0];
                        area->rowEnd = orderedTracks[2];
                        merge = true;
                        break;
                    } else if (area->rowStart == target.rowStart &&
                               area->rowEnd == target.rowEnd) {
                        set.insert(area->columnStart);
                        set.insert(area->columnEnd);
                        set.insert(target.columnStart);
                        set.insert(target.columnEnd);
                        if (set.size() != 3) {
                            continue;
                        }

                        std::vector<size_t> orderedTracks;
                        for (auto it = set.begin(); it != set.end(); ++it) {
                            orderedTracks.push_back(*it);
                        }

                        size_t previous = orderedTracks[0];

                        for (size_t i = 1; i < orderedTracks.size(); i++) {
                            if ((orderedTracks[i] - previous) != 1) {
                                continue;
                            }
                            previous = orderedTracks[i];
                        }

                        area->columnStart = orderedTracks[0];
                        area->columnEnd = orderedTracks[2];
                        merge = true;
                        break;
                    }
                }

                if (merge) {
                    namedGridAreaMap->insertNamedGridAreaData(
                        String::createASCIIString(name.c_str(), name.length()),
                        stack.back());
                } else {
                    return false;
                }
            }
        } else {
            namedGridAreaMap->insertNamedGridAreaData(
                String::createASCIIString(name.c_str(), name.length()),
                stack.back());
        }
    }

    setGridTemplateAreas(namedGridAreaMap);

    return true;
}

bool CSSStyleValuePair::updateValueTransformOrigin(Document* document,
                                                   const CSSTokenVector& tokens)
{
    return updateValueTransformOrigin(tokens, false);
}

bool CSSStyleValuePair::updateValueTransform(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueTransform(tokens, false);
}

bool CSSStyleValuePair::updateValueOpacity(Document* document,
                                           const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueNumber(tokens, CSSPropertyParser::AllowNegative);
}

bool CSSStyleValuePair::updateValueFontWeight(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitFontWeight(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitFontWeight(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FontWeightValueKind;

    // <normal> | bold | bolder | lighter | 100 | 200 | 300 | 400 | 500 | 600 |
    // 700 | 800 | 900 | inherit // initial -> normal
    if (value.equals("normal")) {
        m_value.m_fontWeight = FontWeightValue::NormalFontWeightValue;
    } else if (value.equals("bold")) {
        m_value.m_fontWeight = FontWeightValue::BoldFontWeightValue;
    } else if (value.equals("bolder")) {
        m_value.m_fontWeight = FontWeightValue::BolderFontWeightValue;
    } else if (value.equals("lighter")) {
        m_value.m_fontWeight = FontWeightValue::LighterFontWeightValue;
    } else if (value.equals("100")) {
        m_value.m_fontWeight = FontWeightValue::OneHundredFontWeightValue;
    } else if (value.equals("200")) {
        m_value.m_fontWeight = FontWeightValue::TwoHundredsFontWeightValue;
    } else if (value.equals("300")) {
        m_value.m_fontWeight = FontWeightValue::ThreeHundredsFontWeightValue;
    } else if (value.equals("400")) {
        m_value.m_fontWeight = FontWeightValue::FourHundredsFontWeightValue;
    } else if (value.equals("500")) {
        m_value.m_fontWeight = FontWeightValue::FiveHundredsFontWeightValue;
    } else if (value.equals("600")) {
        m_value.m_fontWeight = FontWeightValue::SixHundredsFontWeightValue;
    } else if (value.equals("700")) {
        m_value.m_fontWeight = FontWeightValue::SevenHundredsFontWeightValue;
    } else if (value.equals("800")) {
        m_value.m_fontWeight = FontWeightValue::EightHundredsFontWeightValue;
    } else if (value.equals("900")) {
        m_value.m_fontWeight = FontWeightValue::NineHundredsFontWeightValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFontFamily(const CSSTokenVector& tokens)
{
    if (tokens.size() == 1) {
        const std::string& str = tokens[0];
        if (str[0] == '\'' && str.length() > 2 && str.back() == '\'') {
            setKeywordValue(String::fromUTF8(str.data() + 1, str.length() - 2));
        } else if (str[0] == '"' && str.length() > 2 && str.back() == '"') {
            setKeywordValue(String::fromUTF8(str.data() + 1, str.length() - 2));
        } else {
            setKeywordValue(String::fromUTF8(str.data(), str.length()));
        }
        return true;
    }
    ValueList* val =
        new ValueList(Separator::CommaSeparatorAppendQuoteWhenMeetWhiteSpace);
    bool seenComma = false;
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == ",") {
            if (seenComma) {
                return false;
            }
            seenComma = true;
        } else {
            const std::string& str = tokens[i];
            if (str[0] == '\'' && str.length() > 2 && str.back() == '\'') {
                val->emplace_back(
                    ValueKind::KeywordValueKind,
                    String::fromUTF8(str.data() + 1, str.length() - 2));
            } else if (str[0] == '"' && str.length() > 2 && str.back() == '"') {
                val->emplace_back(
                    ValueKind::KeywordValueKind,
                    String::fromUTF8(str.data() + 1, str.length() - 2));
            } else {
                val->emplace_back(ValueKind::KeywordValueKind,
                                  String::fromUTF8(str.data(), str.length()));
            }
            seenComma = false;
        }
    }
    if (seenComma)
        return false;

    setValueList(val);
    return true;
}

bool CSSStyleValuePair::updateValueUnitWordWrap(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::WordWrapValueKind;

    if (value.equals("normal")) {
        m_value.m_wordWrap = WordWrapValue::NormalWordWrapValue;
    } else if (value.equals("break-word")) {
        m_value.m_wordWrap = WordWrapValue::BreakWordWordWrapValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWordWrap(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitWordWrap(tokens[0]);
}

bool CSSStyleValuePair::updateValueOverflowWrap(Document* document,
                                                const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitWordWrap(tokens[0]);
}

bool CSSStyleValuePair::updateValueOverflowX(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitOverflowX(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitOverflowX(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (value.equals("visible")) {
        m_value.m_overflow = OverflowValue::VisibleOverflow;
    } else if (value.equals("hidden")) {
        m_value.m_overflow = OverflowValue::HiddenOverflow;
    } else if (value.equals("auto")) {
        m_value.m_overflow = OverflowValue::AutoOverflow;
    } else if (value.equals("scroll")) {
        m_value.m_overflow = OverflowValue::ScrollOverflow;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueOverflowY(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitOverflowY(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitOverflowY(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::OverflowValueKind;

    if (value.equals("visible")) {
        m_value.m_overflow = OverflowValue::VisibleOverflow;
    } else if (value.equals("hidden")) {
        m_value.m_overflow = OverflowValue::HiddenOverflow;
    } else if (value.equals("auto")) {
        m_value.m_overflow = OverflowValue::AutoOverflow;
    } else if (value.equals("scroll")) {
        m_value.m_overflow = OverflowValue::ScrollOverflow;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValuePosition(Document* document,
                                            const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    // <static> | relative | absolute | inherit
    m_valueKind = CSSStyleValuePair::ValueKind::PositionValueKind;

    if (value.equals("static")) {
        m_value.m_position = PositionValue::StaticPositionValue;
    } else if (value.equals("relative")) {
        m_value.m_position = PositionValue::RelativePositionValue;
    } else if (value.equals("absolute")) {
        m_value.m_position = PositionValue::AbsolutePositionValue;
    } else if (value.equals("fixed")) {
        m_value.m_position = PositionValue::FixedPositionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueShadow(const CSSTokenVector& tokens,
                                          bool boxShadow)
{
    if (tokens.size() < 1) {
        return false;
    }
    if (tokens.size() == 1) {
        const CSSTokenValue& t = tokens[0];
        const char* value = t.data();
        if (compareCString("none", value)) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
        } else {
            return false;
        }
        return true;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;

        setValueList(new ValueList(Separator::CommaSeparator));
        uint8_t option = CSSPropertyParser::AllowNegative |
                         CSSPropertyParser::AllowWithoutUnit;

        // Value : none | <shadow>#
        // <shadow> = <color>? && <length>{2,4} && inset?
        // 1st <length> =  horizontal offset
        // 2nd <lennth> =  vertical offset
        // 3rd <length> =  blur radius
        // 4th <length> = spread distance

        size_t lengthSizeLimit = 0;
        size_t shadowSizeLimit = 0;
        if (boxShadow) {
            lengthSizeLimit = 4;
            shadowSizeLimit = lengthSizeLimit + 2;
        } else {
            lengthSizeLimit = 3;
            shadowSizeLimit = lengthSizeLimit + 1;
        }

        bool isValid = false;
        for (size_t i = 0; i < tokens.size();) {
            if (tokens[i].equals(",")) {
                i++;
                isValid = false;
                continue;
            }

            bool hasInset = false;
            bool hasColor = false;
            bool didParseLength = false;

            CSSStyleValuePair shadow;
            shadow.setValueList(new ValueList(Separator::SpaceSeparator));

            size_t j = i;
            size_t currentShadowSize = 0;

            while ((j < tokens.size() && !tokens[j].equals(",")) &&
                   currentShadowSize < shadowSizeLimit) {
                CSSStyleValuePair temp;

                if (temp.updateValueUnitLength(tokens[j], option)) {
                    if (didParseLength) {
                        return false;
                    }
                    didParseLength = true;
                    CSSStyleValuePair lengths;
                    lengths.setValueList(
                        new ValueList(Separator::SpaceSeparator));

                    CSSStyleValuePair length;
                    size_t len2 = 1;
                    for (; (j < tokens.size()) && (len2 <= lengthSizeLimit) &&
                           length.updateValueUnitLength(tokens[j], option);
                         j++, len2++) {
                        std::string str = tokens[j];
                        lengths.multiValue()->push_back(length);
                        currentShadowSize++;
                    }

                    if (lengths.multiValue()->size() < 2) {
                        return false;
                    }

                    shadow.multiValue()->push_back(lengths);

                } else if (temp.updateValueUnitColor(tokens[j])) {
                    // color
                    if (hasColor) {
                        return false;
                    }
                    shadow.multiValue()->push_back(temp);
                    hasColor = true;
                    currentShadowSize++;
                    j++;

                } else if (tokens[j].equals("inset")) {
                    if (hasInset == true || boxShadow == false) {
                        return false;
                    }
                    temp.setKeywordValue(
                        String::fromUTF8(tokens[j].data(), tokens[j].size()));
                    shadow.multiValue()->push_back(temp);
                    hasInset = true;
                    currentShadowSize++;
                    j++;
                } else {
                    return false;
                }
            }

            if (!didParseLength) {
                return false;
            }

            multiValue()->push_back(shadow);
            isValid = true;
            i = j;
        }
        return isValid;
    }
    return false;
}

bool CSSStyleValuePair::updateValueTextShadow(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    // none | [ <offset-x> <offset-y> <blur-radius>? && <color>? ]#
    // initial : none
    return updateValueShadow(tokens, false);
}

bool CSSStyleValuePair::updateValueBoxShadow(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    //  none | [inset? && [ <offset-x> <offset-y> <blur-radius>?
    //  <spread-radius>? <color>? ] ]#
    // initial : none
    return updateValueShadow(tokens, true);
}

bool CSSStyleValuePair::updateValueTextDecorationLine(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() < 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueListKind;
    m_value.m_multiValue = new ValueList(Separator::SpaceSeparator);
    std::set<TextDecorationLineValue> set;
    for (size_t i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        TextDecorationLineValue v;

        if (value.equals("none")) {
            v = TextDecorationLineValue::NoneTextDecorationLineValue;
        } else if (value.equals("underline")) {
            v = TextDecorationLineValue::UnderlineTextDecorationLineValue;
        } else if (value.equals("line-through")) {
            v = TextDecorationLineValue::LineThroughTextDecorationLineValue;
        } else if (value.equals("overline")) {
            v = TextDecorationLineValue::OverlineTextDecorationLineValue;
            return false; // unsupported yet
        } else if (value.equals("blink")) {
            v = TextDecorationLineValue::BlinkTextDecorationLineValue;
            return false; // unsupported yet
        } else {
            return false;
        }

        if (set.find(v) == set.end()) {
            CSSStyleValuePair p;
            p.setTextDecorationLineValue(v);
            m_value.m_multiValue->push_back(p);
            set.insert(v);
        } else {
            return false; // invalid when duplicate inputs are given
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextDecorationColor(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ColorValueKind;

    if (value.equals("currentcolor")) {
        return CSSPropertyParser::parseNamedColor(value, this);
    } else if (value.equals("transparent")) {
        m_value.m_color = Unit::Color(0, 0, 0, 0);
        return true;
    } else {
        return updateValueColor(document, tokens);
    }
    return false;
}

bool CSSStyleValuePair::updateValueTextDecorationStyle(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::TextDecorationStyleValueKind;

    if (value.equals("solid")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::SolidTextDecorationStyleValue;
    } else if (value.equals("double")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::DoubleTextDecorationStyleValue;
        return false; // unsupported yet
    } else if (value.equals("dotted")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::DottedTextDecorationStyleValue;
        return false; // unsupported yet
    } else if (value.equals("dashed")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::DashedTextDecorationStyleValue;
        return false; // unsupported yet
    } else if (value.equals("wavy")) {
        m_value.m_textDecorationStyle =
            TextDecorationStyleValue::WavyTextDecorationStyleValue;
        return false; // unsupported yet
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextUnderlinePosition(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::TextUnderlinePositionValueKind;

    if (value.equals("auto")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::AutoTextUnderlinePositionValue;
    } else if (value.equals("under")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::UnderTextUnderlinePositionValue;
        return false; // unsupported yet
    } else if (value.equals("left")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::LeftTextUnderlinePositionValue;
        return false; // unsupported yet
    } else if (value.equals("right")) {
        m_value.m_textUnderlinePosition =
            TextUnderlinePositionValue::RightTextUnderlinePositionValue;
        return false; // unsupported yet
    } else {
        return false;
    }

    return true;
}

bool CSSStyleValuePair::updateValueResize(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::ResizeValueKind;

    if (value.equals("none")) {
        m_value.m_resize = ResizeValue::NoneResizeValue;
    } else if (value.equals("both")) {
        m_value.m_resize = ResizeValue::BothResizeValue;
        return false; // unsupported yet
    } else if (value.equals("horizontal")) {
        m_value.m_resize = ResizeValue::HorizontalResizeValue;
        return false; // unsupported yet
    } else if (value.equals("vertical")) {
        m_value.m_resize = ResizeValue::VerticalResizeValue;
        return false; // unsupported yet
    } else if (value.equals("block")) {
        m_value.m_resize = ResizeValue::BlockResizeValue;
        return false; // unsupported yet
    } else if (value.equals("inline")) {
        m_value.m_resize = ResizeValue::InlineResizeValue;
        return false; // unsupported yet
    } else {
        return false;
    }

    return true;
}

bool CSSStyleValuePair::updateValueTextAlign(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("start")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::StartTextAlignValue;
    } else if (value.equals("end")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::EndTextAlignValue;
    } else if (value.equals("left")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::LeftTextAlignValue;
    } else if (value.equals("center")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::CenterTextAlignValue;
    } else if (value.equals("right")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::RightTextAlignValue;
    } else if (value.equals("-webkit-center") || value.equals("-moz-center")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextAlignValueKind;
        m_value.m_textAlign = TextAlignValue::WebKitCenterTextAlignValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextTransform(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform = TextTransformValue::NoneTextTransformValue;
    } else if (value.equals("capitalize")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform =
            TextTransformValue::CapitalizeTextTransformValue;
    } else if (value.equals("uppercase")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform =
            TextTransformValue::UppercaseTextTransformValue;
    } else if (value.equals("lowercase")) {
        m_valueKind = CSSStyleValuePair::ValueKind::TextTransformValueKind;
        m_value.m_textTransform =
            TextTransformValue::LowercaseTextTransformValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextOverflow(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    CSSPropertyParser parser(value);
    m_valueKind = CSSStyleValuePair::ValueKind::TextOverflowValueKind;

    String* stringValue = String::emptyString;
    if (value.equals("clip")) {
        m_value.m_textOverflowData =
            new TextOverflowData(TextOverflowValue::TextOverflowClipValue);
    } else if (value.equals("ellipsis")) {
        m_value.m_textOverflowData =
            new TextOverflowData(TextOverflowValue::TextOverflowEllipsisValue);
    } else if (parser.parseContentString(value.data(), value.length(),
                                         &(stringValue))) {
        m_value.m_textOverflowData = new TextOverflowData(stringValue);
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTextIndent(Document* document,
                                              const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowNegative |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueUnicodeBidi(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("normal")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::NormalUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else if (value.equals("embed")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::EmbedUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else if (value.equals("isolate")) {
        m_value.m_unicodeBidi = UnicodeBidiValue::IsolateUnicodeBidiValue;
        m_valueKind = CSSStyleValuePair::ValueKind::UnicodeBidiValueKind;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueVisibility(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::VisibilityValueKind;
    const CSSTokenValue& value = tokens[0];
    if (value.equals("visible")) {
        m_value.m_visibility = VisibilityValue::VisibleVisibilityValue;
    } else if (value.equals("hidden")) {
        m_value.m_visibility = VisibilityValue::HiddenVisibilityValue;
    } else if (value.equals("collapse")) {
        m_value.m_visibility = VisibilityValue::CollapseVisibilityValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueImageRendering(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ImageRenderingValueKind;
    const CSSTokenValue& value = tokens[0];
    if (value.equals("auto")) {
        m_value.m_imageRendering = ImageRenderingValue::ImageRenderingAutoValue;
    } else if (value.equals("crisp-edges")) {
        m_value.m_imageRendering =
            ImageRenderingValue::ImageRenderingCrispEdgesValue;
    } else if (value.equals("pixelated")) {
        m_value.m_imageRendering =
            ImageRenderingValue::ImageRenderingPixelatedValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueZIndex(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const char* value = tokens[0].data();
    int32_t val = 0;
    if (compareCString("auto", value)) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
    } else {
        return CSSPropertyParser::parseInt32(
            value, CSSPropertyParser::AllowNegative, this);
    }

    return true;
}

bool CSSStyleValuePair::updateValueBorderCollapse(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BorderCollapseValueKind;
    if (value.equals("separate")) {
        m_value.m_borderCollapse =
            BorderCollapseValue::SeparateBorderCollapseValue;
    } else if (value.equals("collapse")) {
        m_value.m_borderCollapse =
            BorderCollapseValue::CollapseBorderCollapseValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBorderSpacing(Document* document,
                                                 const CSSTokenVector& tokens)
{
    // <length> <length>? | inherit,
    // Initial : 0, Percentages: N/A, lengths may not be negative.
    if (tokens.size() == 1) {
        const CSSTokenValue& first = tokens[0];
        return CSSPropertyParser::parseLength(first.data(), 0, this);
    } else if (tokens.size() == 2) {
        const CSSTokenValue& first = tokens[0];
        const CSSTokenValue& second = tokens[1];
        ValueData firstData = { 0 };
        ValueData secondData = { 0 };
        if (CSSPropertyParser::parseLength(first.data(), 0, this)) {
            firstData.m_length = m_value.m_length;
            if (CSSPropertyParser::parseLength(second.data(), 0, this)) {
                secondData.m_length = m_value.m_length;
                m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
                m_value.m_multiValue = new ValueList(Separator::SpaceSeparator);
                m_value.m_multiValue->emplace_back(
                    CSSStyleValuePair::ValueKind::Length, firstData);
                m_value.m_multiValue->emplace_back(
                    CSSStyleValuePair::ValueKind::Length, secondData);
                return true;
            }
        }
    }
    return false;
}

bool CSSStyleValuePair::updateValueCaptionSide(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::CaptionSideValueKind;
    if (value.equals("top")) {
        m_value.m_captionSide = CaptionSideValue::TopCaptionSideValue;
    } else if (value.equals("bottom")) {
        m_value.m_captionSide = CaptionSideValue::BottomCaptionSideValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueEmptyCells(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::EmptyCellsValueKind;
    if (value.equals("show")) {
        m_value.m_emptyCells = EmptyCellsValue::ShowEmptyCellsValue;
    } else if (value.equals("hide")) {
        m_value.m_emptyCells = EmptyCellsValue::HideEmptyCellsValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueTableLayout(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::TableLayoutValueKind;
    if (value.equals("auto")) {
        m_value.m_tableLayout = TableLayoutValue::AutoTableLayoutValue;
    } else if (value.equals("fixed")) {
        m_value.m_tableLayout = TableLayoutValue::FixedTableLayoutValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueBoxSizing(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::BoxSizingValueKind;
    if (value.equals("content-box")) {
        m_value.m_boxSizing = BoxSizingValue::ContentBoxBoxSizingValue;
    } else if (value.equals("border-box")) {
        m_value.m_boxSizing = BoxSizingValue::BorderBoxBoxSizingValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitFlexDirection(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FlexDirectionValueKind;
    if (value.equals("row")) {
        m_value.m_flexDirection = FlexDirectionValue::RowFlexDirectionValue;
    } else if (value.equals("row-reverse")) {
        m_value.m_flexDirection =
            FlexDirectionValue::RowReverseFlexDirectionValue;
    } else if (value.equals("column")) {
        m_value.m_flexDirection = FlexDirectionValue::ColumnFlexDirectionValue;
    } else if (value.equals("column-reverse")) {
        m_value.m_flexDirection =
            FlexDirectionValue::ColumnReverseFlexDirectionValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFlexDirection(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitFlexDirection(value);
}

bool CSSStyleValuePair::updateValueBoxOrient(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("horizontal")) {
        m_value.m_boxOrient = BoxOrientValue::HorizontalBoxOrientValue;
    } else if (value.equals("vertical")) {
        m_value.m_boxOrient = BoxOrientValue::VerticalBoxOrientValue;
    } else {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::BoxOrientValueKind;
    return true;
}

bool CSSStyleValuePair::updateValueUnitFlexWrap(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::FlexWrapValueKind;
    if (value.equals("nowrap")) {
        m_value.m_flexWrap = FlexWrapValue::NoWrapFlexWrapValue;
    } else if (value.equals("wrap")) {
        m_value.m_flexWrap = FlexWrapValue::WrapFlexWrapValue;
    } else if (value.equals("wrap-reverse")) {
        m_value.m_flexWrap = FlexWrapValue::WrapReverseFlexWrapValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFlexWrap(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitFlexWrap(value);
}

bool CSSStyleValuePair::updateValueOrder(Document* document,
                                         const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const char* token = tokens[0].data();
    return CSSPropertyParser::parseInt32(
        token, CSSPropertyParser::AllowNegative, this);
}

bool CSSStyleValuePair::updateValueJustifyContent(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::JustifyContentValueKind;
    if (value.equals("normal")) {
        m_value.m_justifyContent =
            JustifyContentValue::NormalJustifyContentValue;
    } else if (value.equals("flex-start")) {
        m_value.m_justifyContent =
            JustifyContentValue::FlexStartJustifyContentValue;
    } else if (value.equals("flex-end")) {
        m_value.m_justifyContent =
            JustifyContentValue::FlexEndJustifyContentValue;
    } else if (value.equals("start")) {
        m_value.m_justifyContent =
            JustifyContentValue::StartJustifyContentValue;
    } else if (value.equals("center")) {
        m_value.m_justifyContent =
            JustifyContentValue::CenterJustifyContentValue;
    } else if (value.equals("end")) {
        m_value.m_justifyContent = JustifyContentValue::EndJustifyContentValue;
    } else if (value.equals("space-between")) {
        m_value.m_justifyContent =
            JustifyContentValue::SpaceBetweenJustifyContentValue;
    } else if (value.equals("space-around")) {
        m_value.m_justifyContent =
            JustifyContentValue::SpaceAroundJustifyContentValue;
    } else if (value.equals("stretch")) {
        m_value.m_justifyContent =
            JustifyContentValue::StretchJustifyContentValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitAlignItem(const CSSTokenValue& value)
{
    m_valueKind = CSSStyleValuePair::ValueKind::AlignItemValueKind;
    if (value.equals("flex-start")) {
        m_value.m_alignItem = AlignItemValue::FlexStartAlignItemValue;
    } else if (value.equals("flex-end")) {
        m_value.m_alignItem = AlignItemValue::FlexEndAlignItemValue;
    } else if (value.equals("start")) {
        m_value.m_alignItem = AlignItemValue::StartAlignItemValue;
    } else if (value.equals("center")) {
        m_value.m_alignItem = AlignItemValue::CenterAlignItemValue;
    } else if (value.equals("end")) {
        m_value.m_alignItem = AlignItemValue::EndAlignItemValue;
    } else if (value.equals("baseline")) {
        m_value.m_alignItem = AlignItemValue::BaselineAlignItemValue;
    } else if (value.equals("stretch")) {
        m_value.m_alignItem = AlignItemValue::StretchAlignItemValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueAlignItems(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitAlignItem(value);
}

bool CSSStyleValuePair::updateValueAlignSelf(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }
    return updateValueAlignItems(document, tokens);
}

bool CSSStyleValuePair::updateValueAlignContent(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::AlignContentValueKind;
    if (value.equals("flex-start")) {
        m_value.m_alignContent = AlignContentValue::FlexStartAlignContentValue;
    } else if (value.equals("flex-end")) {
        m_value.m_alignContent = AlignContentValue::FlexEndAlignContentValue;
    } else if (value.equals("center")) {
        m_value.m_alignContent = AlignContentValue::CenterAlignContentValue;
    } else if (value.equals("space-between")) {
        m_value.m_alignContent =
            AlignContentValue::SpaceBetweenAlignContentValue;
    } else if (value.equals("space-around")) {
        m_value.m_alignContent =
            AlignContentValue::SpaceAroundAlignContentValue;
    } else if (value.equals("stretch")) {
        m_value.m_alignContent = AlignContentValue::StretchAlignContentValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueFlexGrow(Document* document,
                                            const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, 0);
}

bool CSSStyleValuePair::updateValueUnitFlexGrow(const CSSTokenValue& value)
{
    return updateValueUnitNumber(value, 0);
}

bool CSSStyleValuePair::updateValueFlexShrink(Document* document,
                                              const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, 0);
}

bool CSSStyleValuePair::updateValueUnitFlexShrink(const CSSTokenValue& value)
{
    return updateValueUnitNumber(value, 0);
}

bool CSSStyleValuePair::updateValueFlexBasis(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    return updateValueUnitFlexBasis(value);
}

bool CSSStyleValuePair::updateValueUnitFlexBasis(const CSSTokenValue& value)
{
    if (value.equals("content")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FlexBasisValueKind;
        m_value.m_flexBasis = FlexBasisValue::ContentFlexBasisValue;
    } else if (value.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::FlexBasisValueKind;
        m_value.m_flexBasis = FlexBasisValue::AutoFlexBasisValue;
    } else {
        return updateValueUnitLengthOrCalc(value,
                                           CSSPropertyParser::AllowPercent |
                                               CSSPropertyParser::AllowAuto);
    }
    return true;
}

static CSSTransformFunction::Kind transformFunctionKind(
    const CSSTokenValue& name)
{
    if (name == "matrix") {
        return CSSTransformFunction::Kind::Matrix;
    } else if (name == "matrix3d") {
        return CSSTransformFunction::Kind::Matrix3D;
    } else if (name == "translate") {
        return CSSTransformFunction::Kind::Translate;
    } else if (name == "translate3d") {
        return CSSTransformFunction::Kind::Translate3D;
    } else if (name == "translatex") {
        return CSSTransformFunction::Kind::TranslateX;
    } else if (name == "translatey") {
        return CSSTransformFunction::Kind::TranslateY;
    } else if (name == "translatez") {
        return CSSTransformFunction::Kind::TranslateZ;
    } else if (name == "scale") {
        return CSSTransformFunction::Kind::Scale;
    } else if (name == "scale3d") {
        return CSSTransformFunction::Kind::Scale3D;
    } else if (name == "scalex") {
        return CSSTransformFunction::Kind::ScaleX;
    } else if (name == "scaley") {
        return CSSTransformFunction::Kind::ScaleY;
    } else if (name == "scalez") {
        return CSSTransformFunction::Kind::ScaleZ;
    } else if (name == "rotate") {
        return CSSTransformFunction::Kind::Rotate;
    } else if (name == "rotate3d") {
        return CSSTransformFunction::Kind::Rotate3D;
    } else if (name == "skew") {
        return CSSTransformFunction::Kind::Skew;
    } else if (name == "skewx") {
        return CSSTransformFunction::Kind::SkewX;
    } else if (name == "skewy") {
        return CSSTransformFunction::Kind::SkewY;
    } else if (name == "perspective") {
        return CSSTransformFunction::Kind::Perspective;
    } else {
        return CSSTransformFunction::Kind::None;
    }
}

bool CSSStyleValuePair::updateTransformUnit(CSSTransformFunction::Kind fkind,
                                            TransformUnit units[],
                                            int& minArgCnt, int& maxArgCnt)
{
    // https://drafts.csswg.org/css-transforms/#two-d-transform-functions
    // https://drafts.csswg.org/css-transforms-2/#three-d-transform-functions
    switch (fkind) {
    case CSSTransformFunction::Kind::Matrix:
        units[0] = units[1] = units[2] = units[3] = units[4] = units[5] =
            TransformUnit::Number;
        minArgCnt = maxArgCnt = 6;
        break;
    case CSSTransformFunction::Kind::Matrix3D:
        units[0] = units[1] = units[2] = units[3] = units[4] = units[5] =
            units[6] = units[7] = units[8] = units[9] = units[10] = units[11] =
                units[12] = units[13] = units[14] = units[15] =
                    TransformUnit::Number;
        minArgCnt = maxArgCnt = 16;
        break;
    case CSSTransformFunction::Kind::Translate:
        maxArgCnt = 2;
        units[0] = units[1] = TransformUnit::TranslationValue;
        break;
    case CSSTransformFunction::Kind::Translate3D:
        units[0] = units[1] = TransformUnit::TranslationValue;
        units[2] = TransformUnit::Length;
        maxArgCnt = 3;
        break;
    case CSSTransformFunction::Kind::TranslateX:
    case CSSTransformFunction::Kind::TranslateY:
        units[0] = TransformUnit::TranslationValue;
        break;
    case CSSTransformFunction::Kind::TranslateZ:
        units[0] = TransformUnit::Length;
        break;
    case CSSTransformFunction::Kind::Scale:
        units[0] = units[1] = TransformUnit::Number;
        maxArgCnt = 2;
        break;
    case CSSTransformFunction::Kind::Scale3D:
        units[0] = units[1] = units[2] = TransformUnit::Number;
        maxArgCnt = 3;
        break;
    case CSSTransformFunction::Kind::ScaleX:
    case CSSTransformFunction::Kind::ScaleY:
    case CSSTransformFunction::Kind::ScaleZ:
        units[0] = TransformUnit::Number;
        break;
    case CSSTransformFunction::Kind::Rotate:
        units[0] = TransformUnit::Angle;
        units[1] = units[2] = TransformUnit::Length;
        maxArgCnt = 3;
        break;
    case CSSTransformFunction::Kind::Rotate3D:
        units[0] = units[1] = units[2] = TransformUnit::Number;
        units[3] = TransformUnit::Angle;
        minArgCnt = maxArgCnt = 4;
        break;
    case CSSTransformFunction::Kind::Skew:
        units[0] = units[1] = TransformUnit::Angle;
        maxArgCnt = 2;
        break;
    case CSSTransformFunction::Kind::SkewX:
    case CSSTransformFunction::Kind::SkewY:
        units[0] = TransformUnit::Angle;
        break;
    case CSSTransformFunction::Kind::Perspective:
        units[0] = TransformUnit::Number;
        break;
    case CSSTransformFunction::Kind::None:
        return false;
    }
    return true;
}

bool CSSStyleValuePair::addTransformValueToList(
    const CSSTokenVector& transformValueTokens,
    CSSTokenVector& transformValueList)
{
    for (size_t i = 0; i < transformValueTokens.size(); i++) {
        const CSSTokenValue& value = transformValueTokens[i];
        if (value.equals(",")) {
            if (i == 0 || i == transformValueTokens.size() - 1) {
                return false;
            }
        } else {
            transformValueList.push_back(value);
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueTransformFunction(
    const CSSTokenValue& transformValue, CSSTransformFunction::Kind fkind,
    bool canIgnoreUnit, ValueList* values)
{
    TransformUnit units[16] = {
        TransformUnit::Number,
    };
    int minArgCnt = 1, maxArgCnt = 1;
    if (!updateTransformUnit(fkind, units, minArgCnt, maxArgCnt)) {
        return false;
    }

    CSSTokenVector transformValueTokens;
    CSSStyleDeclaration::tokenizeCSSValue(transformValueTokens,
                                          transformValue.data(),
                                          transformValue.size(), ",", 1);

    CSSTokenVector transformValueList;
    if (!addTransformValueToList(transformValueTokens, transformValueList)) {
        return false;
    }

    uint8_t option = canIgnoreUnit ? CSSPropertyParser::AllowWithoutUnit : 0;
    int valueListSize = (int)transformValueList.size();
    int idx = 0;
    for (; idx < maxArgCnt && idx < valueListSize; idx++) {
        TransformUnit unit = units[idx];
        CSSStyleValuePair ret;

        if (hasValidVarFunction(transformValueList[idx])) {
            ret.setValueKind(
                CSSStyleValuePair::ValueKind::VarFunctionValueKind);
            if (transformValueList[idx].startsWith("calc(")) {
                ret.setTemporaryValueKind(ValueKind::CalcValueKind);
            }

            ret.setValue(String::fromUTF8(transformValueList[idx].data(),
                                          transformValueList[idx].size()));

            values->emplace_back(ret);
        } else if (unit == TransformUnit::Number &&
                   ret.updateValueUnitNumber(
                       transformValueList[idx].trim(),
                       CSSPropertyParser::AllowNegative)) {
            values->emplace_back(ret);
        } else if (unit == TransformUnit::Angle &&
                   ret.updateValueUnitAngleOrCalc(
                       transformValueList[idx].trim(),
                       option | CSSPropertyParser::AllowNegative)) {
            values->emplace_back(ret);
        } else if (unit == TransformUnit::Length &&
                   ret.updateValueUnitLengthOrCalc(
                       transformValueList[idx].trim(),
                       option | CSSPropertyParser::AllowNegative)) {
            values->emplace_back(ret);
        } else if (unit == TransformUnit::TranslationValue &&
                   ret.updateValueUnitLengthOrCalc(
                       transformValueList[idx].trim(),
                       option | CSSPropertyParser::AllowNegative |
                           CSSPropertyParser::AllowPercent)) {
            values->emplace_back(ret);
        } else {
            return false;
        }
    }
    if (idx + 1 < minArgCnt) {
        return false;
    }

    return true;
}

bool CSSStyleValuePair::updateValueTransform(const CSSTokenVector& tokens,
                                             bool canIgnoreUnit, Separator sep)
{
    if (tokens.size() == 1 && tokens[0].equals("none")) {
        m_valueKind = CSSStyleValuePair::ValueKind::None;
        return true;
    } else {
        m_valueKind = CSSStyleValuePair::ValueKind::TransformFunctions;
        m_value.m_transforms = new CSSTransformFunctions();

        for (unsigned i = 0; i < tokens.size(); i++) {
            CSSPropertyParser parser((char*)tokens[i].data(),
                                     tokens[i].length());

            CSSTokenValue name;
            if (parser.consumeString(0)) {
                name = parser.parsedString();
                if (parser.consumeIfNext('X')) {
                    name += 'x';
                } else if (parser.consumeIfNext('Y')) {
                    name += (char)'y';
                } else if (parser.consumeIfNext('Z')) {
                    name += (char)'z';
                }
            } else {
                return false;
            }

            parser.consumeWhitespaces();
            parser.consumeParenthesisBlock();
            parser.consumeWhitespaces();
            if (!parser.isEnd()) {
                return false;
            }

            Optional<CSSTokenValue> transformValue = parser.parsedString();
            if (!transformValue) {
                return false;
            }

            CSSTransformFunction::Kind fkind = transformFunctionKind(name);
            if (fkind == CSSTransformFunction::Kind::None) {
                return false;
            }

            ValueList* values = new ValueList(sep);
            if (!updateValueTransformFunction(transformValue.getValue(), fkind,
                                              canIgnoreUnit, values)) {
                return false;
            }
            m_value.m_transforms->emplace_back(fkind, values);
        }
    }

    return true;
}

bool CSSStyleValuePair::updateValueTransformOrigin(const CSSTokenVector& tokens,
                                                   bool canIgnoreUnit)
{
    //  [ left | center | right | top | bottom | <percentage> | <length> ] |
    //  [ left | center | right | <percentage> | <length> ]
    //  [ top | center | bottom | <percentage> | <length> ] <length>? |
    //  [ center | [ left | right ] ] && [ center | [ top | bottom ] ] <length>?

    if (tokens.size() != 1 && tokens.size() != 2 && tokens.size() != 3) {
        return false;
    }

    if (tokens.size() >= 2) {
        const CSSTokenValue& f = tokens[0];
        const CSSTokenValue& s = tokens[1];
        if ((f.equals("left") && s.equals("right")) ||
            (f.equals("right") && s.equals("left")) ||
            (f.equals("top") && s.equals("bottom")) ||
            (f.equals("bottom") && s.equals("top")))
            return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(Separator::SpaceSeparator);

    CSSStyleValuePair xPair(CSSStyleValuePair::ValueKind::SideValueKind,
                            SideValue::CenterSideValue);
    CSSStyleValuePair yPair(CSSStyleValuePair::ValueKind::SideValueKind,
                            SideValue::CenterSideValue);
    CSSStyleValuePair zPair(CSSStyleValuePair::ValueKind::Length, CSSLength(0));

    uint8_t option =
        CSSPropertyParser::AllowPercent | CSSPropertyParser::AllowNegative;
    if (canIgnoreUnit) {
        option |= CSSPropertyParser::AllowWithoutUnit;
    }
    for (unsigned int i = 0; i < std::min(tokens.size(), (size_t)2); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals("left")) {
            xPair.setValue(SideValue::LeftSideValue);
        } else if (value.equals("right")) {
            xPair.setValue(SideValue::RightSideValue);
        } else if (value.equals("center")) {
        } else if (value.equals("top")) {
            yPair.setValue(SideValue::TopSideValue);
        } else if (value.equals("bottom")) {
            yPair.setValue(SideValue::BottomSideValue);
        } else {
            if (i == 0) {
                xPair.setValueKind(CSSStyleValuePair::ValueKind::None);
            } else {
                yPair.setValueKind(CSSStyleValuePair::ValueKind::None);
            }

            if (tokens.size() == 2) {
                if (i == 0) {
                    if (tokens[1].equals("left") || tokens[1].equals("right")) {
                        return false;
                    }
                } else {
                    if (tokens[0].equals("top") || tokens[0].equals("bottom")) {
                        return false;
                    }
                }
            }

            CSSStyleValuePair ret;
            if (!ret.updateValueUnitLengthOrCalc(value, option)) {
                return false;
            }
            values->push_back(ret);
        }
    }

    if (tokens.size() == 3) {
        const CSSTokenValue& s = tokens[2];
        if (!zPair.updateValueUnitLengthOrCalc(s, option)) {
            return false;
        }
        if (zPair.valueKind() == CSSStyleValuePair::ValueKind::Percentage) {
            return false;
        }
    }

    if (xPair.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        values->push_back(xPair);
    }
    if (yPair.valueKind() == CSSStyleValuePair::ValueKind::SideValueKind) {
        values->push_back(yPair);
    }

    values->push_back(zPair);

    m_value.m_multiValue = values;
    return true;
}

bool CSSStyleValuePair::updateValueFill(Document* document,
                                        const CSSTokenVector& tokens)
{
    if (tokens.size() == 1) {
        const CSSTokenValue& value = tokens[0];
        if (value.equals("none")) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
            return true;
        }
    }

    if (CSSPropertyParser::parseUrl(tokens[0].data(), this)) {
        return true;
    } else {
        return updateValueUnitColor(tokens[0]);
    }
}

bool CSSStyleValuePair::updateValueFillOpacity(Document* document,
                                               const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueOpacity(document, tokens);
}

bool CSSStyleValuePair::updateValueStopColor(Document* document,
                                             const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);
    if (tokens.size() != 1) {
        return false;
    }

    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueStopOpacity(Document* document,
                                               const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);
    return updateValueNumber(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueStrokeOpacity(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return updateValueOpacity(document, tokens);
}

bool CSSStyleValuePair::updateValueFillRule(Document* document,
                                            const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::FillRuleValueKind;
    if (value.equals("nonzero")) {
        m_value = FillRuleValue::FillRuleNonZero;
    } else if (value.equals("evenodd")) {
        m_value = FillRuleValue::FillRuleEvenOdd;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueStroke(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (tokens.size() == 1) {
        const CSSTokenValue& value = tokens[0];
        if (value.equals("none")) {
            m_valueKind = CSSStyleValuePair::ValueKind::None;
            return true;
        }
    }
    if (CSSPropertyParser::parseUrl(tokens[0].data(), this)) {
        return true;
    } else {
        return updateValueUnitColor(tokens[0]);
    }
}

bool CSSStyleValuePair::updateValueStrokeWidth(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLengthOrCalc(
        tokens[0], CSSPropertyParser::ParserOption::AllowPercent |
                       CSSPropertyParser::ParserOption::AllowWithoutUnit);
}

bool CSSStyleValuePair::updateValueStrokeLineCap(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    StringDataOnStackASCII ascii(value.data(), value.length());
    ::Starfish::StrokeLineCap e;
    bool result = stringToStrokeLineCap(&ascii, e);
    if (result) {
        setStrokeLineCapValue(e);
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueStrokeLineJoin(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    StringDataOnStackASCII ascii(value.data(), value.length());
    ::Starfish::StrokeLineJoin e;
    bool result = stringToStrokeLineJoin(&ascii, e);
    if (result) {
        setStrokeLineJoinValue(e);
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueStrokeMiterLimit(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, CSSPropertyParser::AllowNone);
}

bool CSSStyleValuePair::updateValueStrokeDasharray(Document* document,
                                                   const CSSTokenVector& tokens)
{
    size_t size = tokens.size();
    if (size < 1) {
        return false;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    ValueList* values = new ValueList(Separator::SpaceSeparator);

    float result = 0.f;
    for (unsigned int i = 0; i < size; i++) {
        CSSTokenValue value = tokens[i];
        if (CSSPropertyParser::parseNumber(value.data(), value.length(), 0,
                                           &result)) {
            values->push_back(CSSStyleValuePair(
                CSSStyleValuePair::ValueKind::Number, (float)result));
        } else {
            return false;
        }
    }
    m_value.m_multiValue = values;
    return true;
}

bool CSSStyleValuePair::updateValueStrokeDashoffset(
    Document* document, const CSSTokenVector& tokens)
{
    return updateValueNumber(tokens, CSSPropertyParser::AllowNone);
}

bool CSSStyleValuePair::updateValueX(Document* document,
                                     const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueY(Document* document,
                                     const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueX1(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueY1(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueX2(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueY2(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueR(Document* document,
                                     const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueRX(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueRY(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueCX(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueCY(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit |
                                         CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueFX(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueFY(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueFR(Document* document,
                                      const CSSTokenVector& tokens)
{
    return updateValueLength(tokens, CSSPropertyParser::AllowPercent);
}

bool CSSStyleValuePair::updateValueOutlineColor(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitColor(tokens[0]);
}

bool CSSStyleValuePair::updateValueOutlineWidth(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderWidth(tokens[0]);
}

bool CSSStyleValuePair::updateValueOutlineStyle(Document* document,
                                                const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitBorderStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueOutlineOffset(Document* document,
                                                 const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitLengthOrCalc(
        tokens[0], CSSPropertyParser::ParserOption::AllowNegative);
}

bool CSSStyleValuePair::updateValueSrc(const CSSTokenVector& tokens)
{
    FontFaceSrcData* src = new FontFaceSrcData;
    int mode = 0;
    // 0 -> expect url,local function
    // 1 -> expect format or comma
    // 2 -> expect comma

    FontFaceSrcData::LoadFrom loadFrom = FontFaceSrcData::LoadFrom::Local;
    FontFaceSrcData::Format format = FontFaceSrcData::Format::Unknown;
    String* srcStr = String::emptyString;

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];
        if (mode == 0) {
            CSSTokenValue s;
            if (token.startsWith("url(")) {
                loadFrom = FontFaceSrcData::URL;
                s = token.substring(4, token.size() - 5);
                mode = 1;
            } else if (token.startsWith("local(")) {
                loadFrom = FontFaceSrcData::Local;
                s = token.substring(6, token.size() - 7);
                mode = 1;
            } else {
                return false;
            }
            if (s.length() > 2 && s.startsWith("\"")) {
                s = s.substring(1, s.length() - 2);
            } else if (s.length() > 2 && s.startsWith("'")) {
                s = s.substring(1, s.length() - 2);
            }

            srcStr = String::fromUTF8(s.data(), s.length());

            if (i == tokens.size() - 1) {
                src->m_data.push_back(std::make_tuple(
                    srcStr, loadFrom, FontFaceSrcData::NotSpecified));
            }
        } else if (mode == 1) {
            if (token == ",") {
                src->m_data.push_back(std::make_tuple(
                    srcStr, loadFrom, FontFaceSrcData::NotSpecified));
                mode = 0;
                continue;
            } else if (!token.startsWith("format(")) {
                return false;
            }
            CSSTokenValue s = token;
            std::transform(s.begin(), s.end(), s.begin(), tolower);

            if (s.find("truetype") != std::string::npos) {
                format = FontFaceSrcData::Format::TrueType;
            } else if (s.find("woff2") != std::string::npos) {
                format = FontFaceSrcData::Format::WOFF2;
            } else if (s.find("woff") != std::string::npos) {
                format = FontFaceSrcData::Format::WOFF;
            } else if (s.find("opentype") != std::string::npos) {
                format = FontFaceSrcData::Format::OpenType;
            } else if (s.find("embedded-opentype") != std::string::npos) {
                format = FontFaceSrcData::Format::EmbeddedOpenType;
            } else if (s.find("svg") != std::string::npos) {
                format = FontFaceSrcData::Format::SVG;
            } else {
                format = FontFaceSrcData::Format::Unknown;
            }

            if (i == tokens.size() - 1) {
                src->m_data.push_back(
                    std::make_tuple(srcStr, loadFrom, format));
            }
            mode = 2;
        } else if (mode == 2) {
            if (token == ",") {
                src->m_data.push_back(
                    std::make_tuple(srcStr, loadFrom, format));
                mode = 0;
            } else {
                return false;
            }
        }
    }

    if (!src->data().size()) {
        return false;
    }

    setKeyKind(KeyKind::Src);
    setFontFaceSrcData(src);

    return true;
}

bool CSSStyleValuePair::updateValueMaskType(Document* document,
                                            const CSSTokenVector& tokens)
{
    return updateValueMaskType(tokens, true);
}

bool CSSStyleValuePair::updateValueMaskType(const CSSTokenVector& tokens,
                                            bool allowComma)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::MaskTypeValueKind;
    if (value.equals("luminance")) {
        m_value = MaskTypeValue::LuminanceMaskTypeValue;
    } else if (value.equals("alpha")) {
        m_value = MaskTypeValue::AlphaMaskTypeValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueMaskImage(Document* document,
                                             const CSSTokenVector& tokens)
{
    return updateValueMaskImage(tokens, true);
}

bool CSSStyleValuePair::updateValueMaskImage(const CSSTokenVector& tokens,
                                             bool allowComma)
{
    // <mask-reference>#
    // <mask-reference> = none | <image> | <mask-source>
    // <mask-source> = <url>
    bool shouldBeComma = false;
    ValueList* values = new ValueList(Separator::CommaSeparator);
    for (unsigned int i = 0; i < tokens.size(); i++) {
        const CSSTokenValue& value = tokens[i];
        if (value.equals(",")) {
            if (!allowComma || !shouldBeComma) {
                return false;
            }
            shouldBeComma = false;
            continue;
        }
        CSSStyleValuePair pair;
        if (shouldBeComma || !pair.updateValueUnitImageValue(value)) {
            return false;
        }
        shouldBeComma = true;
        values->push_back(pair);
    }
    m_valueKind = CSSStyleValuePair::ValueKind::ValueListKind;
    m_value.m_multiValue = values;
    return shouldBeComma;
}

bool CSSStyleValuePair::updateValueUnitFourSidedShorthandProperty(
    CSSStyleValuePair::KeyKind keyKind, const CSSTokenValue& token)
{
    switch (keyKind) {
    case CSSStyleValuePair::KeyKind::BorderColor:
        return updateValueUnitBorderColor(token);
    case CSSStyleValuePair::KeyKind::BorderStyle:
        return updateValueUnitBorderStyle(token);
    case CSSStyleValuePair::KeyKind::BorderWidth:
        return updateValueUnitBorderWidth(token);
    case CSSStyleValuePair::KeyKind::Inset:
        return updateValueUnitInset(token);
    case CSSStyleValuePair::KeyKind::Margin:
        return updateValueUnitMargin(token);
    case CSSStyleValuePair::KeyKind::Padding:
        return updateValueUnitPadding(token);
    default:
        STARFISH_LOG_ERROR("keyKind is wrong.");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return false;
    }
}

bool CSSStyleValuePair::updateValueMaskSize(Document* document,
                                            const CSSTokenVector& tokens)
{
    return updateValueBackgroundSize(tokens, true);
}

bool CSSStyleValuePair::updateValueMaskPositionX(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitPositionX(tokens[0]);
}

bool CSSStyleValuePair::updateValueMaskPositionY(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitPositionY(tokens[0]);
}

bool CSSStyleValuePair::updateValueMaskRepeatX(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitRepeatStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueMaskRepeatY(Document* document,
                                               const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitRepeatStyle(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitListStyleType(Document* document,
                                                     const CSSTokenValue& value)
{
    STARFISH_ASSERT(document != nullptr);

    String* parsed = String::emptyString;
    if (value.equals("none")) {
        setValueKind(CSSStyleValuePair::None);
    } else if (CSSPropertyParser::parseContentString(value.data(),
                                                     value.length(), &parsed)) {
        setValueKind(CSSStyleValuePair::StringValueKind);
        setValue(parsed);
    } else {
        String* customIdent = String::fromUTF8(value.data(), value.length());
        if (!CSSPropertyParser::stringIsIdent(customIdent)) {
            return false;
        }
        AtomicString aCustomIdent =
            AtomicString::createAtomicString(document->starfish(), customIdent);
        setAtomicStringValue(aCustomIdent);
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitListStylePosition(
    const CSSTokenValue& value)
{
    setValueKind(CSSStyleValuePair::ListStylePositionValueKind);
    if (value.equals("inside")) {
        setValue(ListStylePositionValue::ListStylePositionInside);
        return true;
    }
    if (value.equals("outside")) {
        setValue(ListStylePositionValue::ListStylePositionOutside);
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueUnitListStyleImage(
    const CSSTokenValue& value)
{
    if (value.equals("none")) {
        setValueKind(CSSStyleValuePair::None);
        return true;
    }
    return CSSPropertyParser::parseUrl(value.data(), this);
}

bool CSSStyleValuePair::updateValueListStyleType(Document* document,
                                                 const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitListStyleType(document, tokens[0]);
}

bool CSSStyleValuePair::updateValueListStylePosition(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitListStylePosition(tokens[0]);
}

bool CSSStyleValuePair::updateValueListStyleImage(Document* document,
                                                  const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitListStyleImage(tokens[0]);
}

static bool parseCounterPairList(Document* document,
                                 const CSSTokenVector& tokens,
                                 int32_t defaultValue, CSSStyleValuePair* pair)
{
    STARFISH_ASSERT(document != nullptr);

    size_t size = tokens.size();
    if (size == 1 && tokens[0].equals("none")) {
        pair->setValueKind(CSSStyleValuePair::None);
        return true;
    }
    // NOTE Use temp containers to prevent making unnecessary AtomicStrings
    GCVector<String*> tempIdent;
    GCAtomicVector<int32_t> tempInt;
    bool nameTurn = true;
    for (size_t i = 0; i < size; i++, nameTurn = !nameTurn) {
        const CSSTokenValue& token = tokens[i];
        if (nameTurn) {
            String* ident = String::emptyString;
            if (CSSPropertyParser::parseCustomIdent(token.data(),
                                                    token.length(), &ident) &&
                !ident->equals("none")) {
                tempIdent.push_back(ident);
            } else {
                return false;
            }
        } else {
            int32_t number;
            if (CSSPropertyParser::parseInt32(
                    token.data(), CSSPropertyParser::AllowNegative, number)) {
                tempInt.push_back(number);
            } else {
                tempInt.push_back(defaultValue);
                i--;
            }
        }
    }
    size = tempIdent.size();
    size_t intSize = tempInt.size();
    ValueList* list = new ValueList(Separator::SpaceSeparator);
    for (size_t i = 0; i < size; i++) {
        list->emplace_back(CSSStyleValuePair::AtomicStringValueKind,
                           AtomicString::createAtomicString(
                               document->starfish(), tempIdent[i]));
        if (i < intSize) {
            list->emplace_back(CSSStyleValuePair::Int32, tempInt[i]);
        } else {
            list->emplace_back(CSSStyleValuePair::Int32, defaultValue);
        }
    }
    pair->setValueList(list);
    return true;
}

bool CSSStyleValuePair::updateValueCounterReset(Document* document,
                                                const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return parseCounterPairList(document, tokens, 0, this);
}

bool CSSStyleValuePair::updateValueCounterIncrement(
    Document* document, const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    return parseCounterPairList(document, tokens, 1, this);
}

bool CSSStyleValuePair::updateValueUserSelect(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::UserSelectValueKind;
    if (value.equals("none")) {
        m_value.m_userSelect = UserSelectValue::NoneUserSelectValue;
    } else if (value.equals("text")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_userSelect = UserSelectValue::TextUserSelectValue;
        return false;
    } else if (value.equals("contain")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_userSelect = UserSelectValue::ContainUserSelectValue;
        return false;
    } else if (value.equals("all")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_userSelect = UserSelectValue::AllUserSelectValue;
        return false;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueHyphens(Document* document,
                                           const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::HyphensValueKind;
    if (value.equals("none")) {
        m_value.m_hyphens = HyphensValue::NoneHyphensValue;
    } else if (value.equals("manual")) {
        // TODO: enable the comment below when supporting this value
        // m_value.m_hyphens = HyphensValue::ManualHyphensValue;
        return false;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueLineBreak(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("auto")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Auto;
        return true;
    }

    m_valueKind = CSSStyleValuePair::ValueKind::LineBreakValueKind;
    if (value.equals("loose")) {
        m_value.m_lineBreak = LineBreakValue::LooseLineBreakValue;
    } else if (value.equals("normal")) {
        m_value.m_lineBreak = LineBreakValue::NormalLineBreakValue;
    } else if (value.equals("strict")) {
        m_value.m_lineBreak = LineBreakValue::StrictLineBreakValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWordBreak(Document* document,
                                             const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::WordBreakValueKind;
    if (value.equals("normal")) {
        m_value.m_wordBreak = WordBreakValue::NormalWordBreakValue;
    } else if (value.equals("break-all")) {
        m_value.m_wordBreak = WordBreakValue::BreakAllWordBreakValue;
    } else if (value.equals("keep-all")) {
        m_value.m_wordBreak = WordBreakValue::KeepAllWordBreakValue;
    } else if (value.equals("break-word")) {
        m_value.m_wordBreak = WordBreakValue::BreakWordWordBreakValue;
        return false; // unsupported yet
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueAppearance(Document* document,
                                              const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    m_valueKind = CSSStyleValuePair::ValueKind::AppearanceValueKind;
    if (value.equals("auto")) {
        m_value.m_appearance = AppearanceValue::AutoAppearanceValue;
    } else if (value.equals("none")) {
        m_value.m_appearance = AppearanceValue::NoneAppearanceValue;
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueWillChange(Document* document,
                                              const CSSTokenVector& tokens)
{
    STARFISH_ASSERT(document != nullptr);

    size_t size = tokens.size();
    if (size % 2 == 0) {
        // Including comma seperators, size of tokens has to be odd number
        return false;
    }
    if (size == 1 && tokens[0].equals("auto")) {
        setValueKind(Auto);
        return true;
    }
    auto list = new ValueList(Separator::CommaSeparator);
    for (size_t i = 0; i < size; i++) {
        if (i % 2 != 0) {
            if (!tokens[i].equals(",")) {
                return false;
            }
            continue;
        }
        if (tokens[i].equals("auto") || tokens[i].equals("initial") ||
            tokens[i].equals("inherit") || tokens[i].equals("unset") ||
            tokens[i].equals("will-change")) {
            return false;
        }
        String* item = String::fromUTF8(tokens[i].data(), tokens[i].length());
        if (!CSSPropertyParser::stringIsIdent(item)) {
            return false;
        }
        list->emplace_back(
            AtomicStringValueKind,
            AtomicString::createAtomicString(document->starfish(), item));
    }
    setValueList(list);
    return true;
}

bool CSSStyleValuePair::updateValueBoxDecorationBreak(
    Document* document, const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    const CSSTokenValue& value = tokens[0];
    if (value.equals("clone")) {
        setBoxDecorationBreakValue(CloneBoxDecorationBreakValue);
        return true;
    } else if (value.equals("slice")) {
        setBoxDecorationBreakValue(SliceBoxDecorationBreakValue);
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueRowGap(Document* document,
                                          const CSSTokenVector& tokens)
{
    // normal | <length-percentage [0,∞]>
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    }

    return updateValueUnitLengthOrCalc(value,
                                       CSSPropertyParser::AllowNegative |
                                           CSSPropertyParser::AllowPercent |
                                           CSSPropertyParser::AllowAuto);
}

bool CSSStyleValuePair::updateValueColumnGap(Document* document,
                                             const CSSTokenVector& tokens)
{
    // normal | <length-percentage [0,∞]>
    if (tokens.size() != 1) {
        return false;
    }

    const CSSTokenValue& value = tokens[0];
    if (value.equals("normal")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Normal;
        return true;
    }

    return updateValueUnitLengthOrCalc(value,
                                       CSSPropertyParser::AllowNegative |
                                           CSSPropertyParser::AllowPercent |
                                           CSSPropertyParser::AllowAuto);
}

static bool parseCubicBezierFunction(const CSSTokenValue& value,
                                     CSSStyleValuePair* result)
{
    STARFISH_ASSERT(result != nullptr);

    Optional<CSSTokenValue> mayBezier =
        CSSPropertyParser::parseFunctionBlock(value.data(), "cubic-bezier");
    if (!mayBezier.hasValue()) {
        return false;
    }
    std::vector<CSSTokenValue> tokens;
    mayBezier.getValue().split(',', tokens);
    if (tokens.size() != 4) {
        return false;
    }
    float x1, y1, x2, y2;
    auto trimmed = tokens[0].trim();
    if (!CSSPropertyParser::parseNumber(trimmed.data(), trimmed.length(),
                                        CSSPropertyParser::AllowNegative,
                                        &x1)) {
        return false;
    }
    trimmed = tokens[1].trim();
    if (!CSSPropertyParser::parseNumber(trimmed.data(), trimmed.length(),
                                        CSSPropertyParser::AllowNegative,
                                        &y1)) {
        return false;
    }
    trimmed = tokens[2].trim();
    if (!CSSPropertyParser::parseNumber(trimmed.data(), trimmed.length(),
                                        CSSPropertyParser::AllowNegative,
                                        &x2)) {
        return false;
    }
    trimmed = tokens[3].trim();
    if (!CSSPropertyParser::parseNumber(trimmed.data(), trimmed.length(),
                                        CSSPropertyParser::AllowNegative,
                                        &y2)) {
        return false;
    }
    if (x1 < 0 || x1 > 1 || x2 < 0 || x2 > 1) {
        return false;
    }
    result->setTimingFunctionPointerValue(new CubicBezier(x1, y1, x2, y2));
    return true;
}

static bool parseStepsFunction(const CSSTokenValue& value,
                               CSSStyleValuePair* result)
{
    STARFISH_ASSERT(result != nullptr);

    Optional<CSSTokenValue> maySteps =
        CSSPropertyParser::parseFunctionBlock(value.data(), "steps");
    if (!maySteps.hasValue()) {
        return false;
    }
    const CSSTokenValue& stepsContent = maySteps.getValue();
    CSSTokenVector args;
    if (!CSSPropertyParser::parseLayers(stepsContent.data(),
                                        stepsContent.length(), args)) {
        return false;
    }
    size_t size = args.size();
    if (size < 1 || size > 2) {
        return false;
    }
    int32_t number;
    Steps::StepPosition position = Steps::StepPosition::END;
    if (!CSSPropertyParser::parseInt32(args[0].data(), 0, number) ||
        number <= 0) {
        return false;
    }
    if (size == 2) {
        if (args[1] == "start") {
            position = Steps::StepPosition::START;
        } else if (args[1] != "end") {
            return false;
        }
    }
    result->setTimingFunctionPointerValue(Steps::createSteps(number, position));
    return true;
}

bool CSSStyleValuePair::updateValueUnitTransitionTimingFunction(
    const CSSTokenValue& value)
{
    if (value.equals("ease") == true) {
        setTimingFunctionValue(TimingFunctionEaseValue);
    } else if (value.equals("linear") == true) {
        setTimingFunctionValue(TimingFunctionLinearValue);
    } else if (value.equals("ease-in") == true) {
        setTimingFunctionValue(TimingFunctionEaseInValue);
    } else if (value.equals("ease-out") == true) {
        setTimingFunctionValue(TimingFunctionEaseOutValue);
    } else if (value.equals("ease-in-out") == true) {
        setTimingFunctionValue(TimingFunctionEaseInOutValue);
    } else if (value.equals("step-start") == true) {
        setTimingFunctionValue(TimingFunctionStepStartValue);
    } else if (value.equals("step-end") == true) {
        setTimingFunctionValue(TimingFunctionStepEndValue);
    } else if (parseCubicBezierFunction(value, this) == false &&
               parseStepsFunction(value, this) == false) {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueLayerTransitionProperty(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    const CSSTokenValue& value = tokens[0];
    return updateValueUnitTransitionProperty(value);
}

bool CSSStyleValuePair::updateValueLayerTransitionDuration(
    const CSSTokenVector& tokens)
{
    return updateValueTime(tokens, 0);
}

bool CSSStyleValuePair::updateValueLayerTransitionTimingFunction(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitTransitionTimingFunction(tokens[0]);
}

bool CSSStyleValuePair::updateValueLayerTransitionDelay(
    const CSSTokenVector& tokens)
{
    return updateValueTime(tokens, 0);
}

bool CSSStyleValuePair::updateValueUnitAnimationTimingFunction(
    const CSSTokenValue& value)
{
    if (value.equals("ease") == true) {
        setTimingFunctionValue(TimingFunctionEaseValue);
    } else if (value.equals("linear") == true) {
        setTimingFunctionValue(TimingFunctionLinearValue);
    } else if (value.equals("ease-in") == true) {
        setTimingFunctionValue(TimingFunctionEaseInValue);
    } else if (value.equals("ease-out") == true) {
        setTimingFunctionValue(TimingFunctionEaseOutValue);
    } else if (value.equals("ease-in-out") == true) {
        setTimingFunctionValue(TimingFunctionEaseInOutValue);
    } else if (value.equals("step-start") == true) {
        setTimingFunctionValue(TimingFunctionStepStartValue);
    } else if (value.equals("step-end") == true) {
        setTimingFunctionValue(TimingFunctionStepEndValue);
    } else if (parseCubicBezierFunction(value, this) == false &&
               parseStepsFunction(value, this) == false) {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitAnimationPlayState(
    const CSSTokenValue& value)
{
    if (value.equals("running") == true) {
        setAnimationPlayStateValue(AnimationPlayStateValue::Running);
    } else if (value.equals("paused") == true) {
        setAnimationPlayStateValue(AnimationPlayStateValue::Paused);
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueUnitAnimationFillMode(
    const CSSTokenValue& value)
{
    if (value.equals("none") == true) {
        setAnimationFillModeValue(AnimationFillModeValue::None);
    } else if (value.equals("forwards") == true) {
        setAnimationFillModeValue(AnimationFillModeValue::Forwards);
    } else if (value.equals("backwards") == true) {
        setAnimationFillModeValue(AnimationFillModeValue::Backwards);
    } else if (value.equals("both") == true) {
        setAnimationFillModeValue(AnimationFillModeValue::Both);
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueLayerAnimationName(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAnimationName(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitAnimationName(const CSSTokenValue& value)
{
    if (CSSPropertyParser::stringIsIdent(
            String::fromUTF8(value.data(), value.length())) == false) {
        return false;
    }
    setValueKind(CSSStyleValuePair::ValueKind::StringValueKind);
    setStringValue(String::fromUTF8(value.data(), value.size()));
    return true;
}

bool CSSStyleValuePair::updateValueLayerAnimationDuration(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueTime(tokens, 0);
}

bool CSSStyleValuePair::updateValueLayerAnimationTimingFunction(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAnimationTimingFunction(tokens[0]);
}

bool CSSStyleValuePair::updateValueLayerAnimationDelay(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueTime(tokens, CSSPropertyParser::AllowNegative);
}

bool CSSStyleValuePair::updateValueLayerAnimationIterationCount(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAnimationIterationCount(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitAnimationIterationCount(
    const CSSTokenValue& value)
{
    if (value.equals("infinite")) {
        m_valueKind = CSSStyleValuePair::ValueKind::Number;
        m_value.m_floatValue = std::numeric_limits<float>::infinity();
    } else {
        float f;
        if (CSSPropertyParser::parseNumber(value.data(), value.length(), 0,
                                           &f) == true) {
            m_valueKind = CSSStyleValuePair::ValueKind::Number;
            m_value.m_floatValue = f;
        } else {
            return false;
        }
    }
    return true;
}

bool CSSStyleValuePair::updateValueLayerAnimationDirection(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAnimationDirection(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitAnimationDirection(
    const CSSTokenValue& value)
{
    if (value.equals("normal") == true) {
        setAnimationDirectionValue(AnimationDirectionValue::Normal);
    } else if (value.equals("reverse") == true) {
        setAnimationDirectionValue(AnimationDirectionValue::Reverse);
    } else if (value.equals("alternate") == true) {
        setAnimationDirectionValue(AnimationDirectionValue::Alternate);
    } else if (value.equals("alternate-reverse") == true) {
        setAnimationDirectionValue(AnimationDirectionValue::AlternateReverse);
    } else {
        return false;
    }
    return true;
}

bool CSSStyleValuePair::updateValueLayerAnimationPlayState(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAnimationPlayState(tokens[0]);
}

bool CSSStyleValuePair::updateValueLayerAnimationFillMode(
    const CSSTokenVector& tokens)
{
    if (tokens.size() != 1) {
        return false;
    }
    return updateValueUnitAnimationFillMode(tokens[0]);
}

bool CSSStyleValuePair::updateValueUnitFilterFunction(
    const CSSTokenValue& token)
{
    CSSFilterFunction* result = CSSFilterFunction::parse(token);
    if (result) {
        setFilterFunctionValue(result);
        return true;
    }
    return false;
}

bool CSSStyleValuePair::updateValueFilter(Document* document,
                                          const CSSTokenVector& tokens)
{
    if (!tokens.size()) {
        return false;
    }

    if (tokens.size() == 1 && tokens[0].equals("none")) {
        setValueKind(ValueKind::None);
        return true;
    }

    ValueList* list = new ValueList(Separator::SpaceSeparator);
    for (size_t i = 0; i < tokens.size(); i++) {
        CSSStyleValuePair item;
        if (item.updateValueUnitFilterFunction(tokens[i])) {
            list->push_back(item);
        } else {
            return false;
        }
    }
    setValueList(list);
    return true;
}

#ifdef STARFISH_ENABLE_TEST
void dump(Node* node, unsigned depth)
{
    STARFISH_ASSERT(node != nullptr);

    if (node->isElement()) {
        for (unsigned i = 0; i < depth; i++) {
            printf("  ");
        }

        node->asElement()->dumpStyle();
        printf("\n");
    }

    Node* child = node->firstChild();
    while (child) {
        if (child->isElement()) {
            dump(child, depth + 1);
            if (child->isHTMLIFrameElement() &&
                child->asHTMLIFrameElement()->browsingContext()) {
                dump(child->asHTMLIFrameElement()
                         ->browsingContext()
                         ->document()
                         ->asNode(),
                     depth + 1);
            }
        }
        child = child->nextSibling();
    }
}

void StyleResolver::dumpDOMStyle(Document* document)
{
    STARFISH_ASSERT(document != nullptr);

    dump(document->asNode(), 0);
    printf("\n");
}
#endif
} // namespace Starfish
