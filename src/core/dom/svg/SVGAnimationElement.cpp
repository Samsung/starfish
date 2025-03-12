/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "SVGAnimationElement.h"

#include "StaticStrings.h"
#include "Starfish.h"
#include "core/style/Style.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/animation/CubicBezier.h"

namespace Starfish {

static bool parseLengthValue(String* lengthValue, uint8_t option,
                             CSSStyleValuePair& cssStyleValuePair)
{
    auto str = lengthValue->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, str.c_str(), str.length());
    return cssStyleValuePair.updateValueLength(tokens, option);
}

AnimationFillModeValue svgAnimationFillToAnimationFillModeValue(
    SVGAnimationFill fill)
{
    switch (fill) {
    case SVGAnimationFill::Freeze:
        return AnimationFillModeValue::Forwards;
    case SVGAnimationFill::Remove:
        return AnimationFillModeValue::None;
    default:
        STARFISH_UNIMPLEMENTED("Unimplemented fill value");
        break;
    }

    return AnimationFillModeValue::None;
}

CubicBezierEaseType svgAnimationCalcModeToCubicBezierEaseType(
    SVGAnimationCalcMode calcMode)
{
    switch (calcMode) {
    case SVGAnimationCalcMode::Linear:
        return CubicBezierEaseType::Linear;
    case SVGAnimationCalcMode::Spline:
        return CubicBezierEaseType::Custom;
    default:
        STARFISH_UNIMPLEMENTED("Unimplemented calcMode value");
        break;
    }
    return CubicBezierEaseType::Linear;
}

SVGAnimationElement::SVGAnimationElement(Document* document,
                                         const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_declarations(new CSSStyleDeclaration(this))
{
}

void* SVGAnimationElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGAnimationElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGAnimationElement)] = { 0 };
        SVGAnimationElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGAnimationElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Optional<Element*> SVGAnimationElement::targetElement()
{
    // TODO: Use href if present.
    Element* targetElement = parentElement();
    if (!targetElement->isSVGElement() ||
        !targetElement->asSVGElement()->isRenderableElement()) {
        return Optional<Element*>();
    }
    return targetElement;
}

void SVGAnimationElement::beginElement()
{
    beginElementAt(0);
}

void SVGAnimationElement::beginElementAt(float offset)
{
    STARFISH_UNIMPLEMENTED();
}

bool SVGAnimationElement::parseAttributeName(
    CSSStyleValuePair::KeyKind& keyKind)
{
    Optional<String*> maybeAttributeName =
        getAttribute(starfish()->staticStrings()->m_attributename);
    if (!maybeAttributeName) {
        return false;
    }

    String* targetAttrName = maybeAttributeName.value();
    keyKind = CSSStyleLookupTrie::lookupCSSStyle(
        targetAttrName->toUTF8NonGCString().data(), targetAttrName->length());

    // TODO: Probably need to differentiate between the allowable keyKinds
    // depending on the target element.
    return keyKind != CSSStyleValuePair::KeyKind::Unknown;
}

bool SVGAnimationElement::parseValues(CSSStyleValuePair::KeyKind keyKind,
                                      GCVector<CSSStyleValuePair>& values)
{
    Optional<String*> maybeValues =
        getAttribute(starfish()->staticStrings()->m_values);

    if (!maybeValues) {
        return false;
    }
    String* valuesValue = maybeValues.getValue();

    GCVector<StringView> tokens;
    StringUtils::tokenize(valuesValue, ";", 1, tokens);

    for (auto& token : tokens) {
        StringBufferAccessData bad = token.bufferAccessData();
        CSSStyleValuePair pair;
        if (!parseValue(keyKind, bad.asciiData(), bad.length, pair)) {
            return false;
        }
        values.push_back(pair);
    }

    return true;
}

bool SVGAnimationElement::parseValue(CSSStyleValuePair::KeyKind keyKind,
                                     const char* buffer, size_t len,
                                     CSSStyleValuePair& pair)
{
    // Parse each value in values using the rules for parsing the attribute
    // identified by the ‘attributeName’ attributes.
    // Note that ‘attributeName’ corresponds to an attribute name or a CSS
    // property name.
    bool ret = m_declarations->setPropertyInternal(keyKind, buffer, len, false);
    if (!ret) {
        return false;
    }
    pair = m_declarations->getCSSValuePair(keyKind);
    return true;
}

bool SVGAnimationElement::convertFallbackValues(
    CSSStyleValuePair::KeyKind keyKind, GCVector<CSSStyleValuePair>& values)
{
    Optional<Element*> maybeTargetElement = targetElement();
    if (!maybeTargetElement) {
        return false;
    }

    CSSStyleDeclaration* cssStyleDeclaration =
        maybeTargetElement.value()->getComputedStyle();
    cssStyleDeclaration->updateValue(keyKind);
    CSSStyleValuePair originValue =
        cssStyleDeclaration->getCSSValuePair(keyKind);
    if (originValue.keyKind() == CSSStyleValuePair::KeyKind::Unknown) {
        return false;
    }

    // Fill values as an original computed style value.
    for (size_t i = 0; i < values.size(); i++) {
        values[i] = originValue;
    }
    return true;
}

bool SVGAnimationElement::parseFrom(CSSStyleValuePair::KeyKind keyKind,
                                    GCVector<CSSStyleValuePair>& values)
{
    Optional<String*> maybeFrom =
        getAttribute(starfish()->staticStrings()->m_from);
    if (!maybeFrom) {
        return false;
    }
    return parseFromAndToInternal(keyKind, maybeFrom.getValue(), values);
}

bool SVGAnimationElement::parseTo(CSSStyleValuePair::KeyKind keyKind,
                                  GCVector<CSSStyleValuePair>& values)
{
    Optional<String*> maybeTo = getAttribute(starfish()->staticStrings()->m_to);
    if (!maybeTo) {
        return false;
    }

    return parseFromAndToInternal(keyKind, maybeTo.getValue(), values);
}

bool SVGAnimationElement::parseFromAndToInternal(
    CSSStyleValuePair::KeyKind keyKind, String* value,
    GCVector<CSSStyleValuePair>& values)
{
    struct Args {
        CSSStyleValuePair::KeyKind keyKind;
        CSSStyleValuePair pair;
        SVGAnimationElement* self = nullptr;
        bool ret = false;
    } args;
    args.keyKind = keyKind;
    args.self = this;
    value->peekUTF8Buffer(
        [](const char* buffer, size_t len, void* data) -> size_t {
            Args* p = static_cast<Args*>(data);
            p->ret = p->self->parseValue(p->keyKind, buffer, len, p->pair);
            return 0;
        },
        &args);
    if (!args.ret) {
        return false;
    }
    values.push_back(args.pair);
    return true;
}

bool SVGAnimationElement::parseDur(CSSTime& duration)
{
    Optional<String*> maybeDur =
        getAttribute(starfish()->staticStrings()->m_dur);
    if (!maybeDur) {
        return false;
    }

    CSSStyleValuePair temp;
    auto str = maybeDur.value()->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, str.c_str(), str.length());
    if (!temp.updateValueTime(tokens, 0)) {
        return false;
    }
    duration = temp.timeValue();
    return true;
}

bool SVGAnimationElement::parseFill(SVGAnimationFill& fill)
{
    Optional<String*> maybeFill =
        getAttribute(starfish()->staticStrings()->m_fill);
    if (!maybeFill) {
        return false;
    }
    String* fillValue = maybeFill.value();
    if (fillValue->equals("remove")) {
        fill = SVGAnimationFill::Remove;
        return true;
    } else if (fillValue->equals("freeze")) {
        fill = SVGAnimationFill::Freeze;
        return true;
    } else {
        STARFISH_LOG_WARN("Unknown fill value: %s",
                          fillValue->toUTF8NonGCString().c_str());
    }
    return false;
}

bool SVGAnimationElement::parseCalcMode(SVGAnimationCalcMode& calcMode)
{
    Optional<String*> maybeCalcMode =
        getAttribute(starfish()->staticStrings()->m_calcMode);
    if (!maybeCalcMode) {
        return false;
    }

    if (maybeCalcMode->equals("discrete")) {
        calcMode = SVGAnimationCalcMode::Discrete;
        return true;
    } else if (maybeCalcMode->equals("linear")) {
        calcMode = SVGAnimationCalcMode::Linear;
        return true;
    } else if (maybeCalcMode->equals("paced")) {
        calcMode = SVGAnimationCalcMode::Paced;
        return true;
    } else if (maybeCalcMode->equals("spline")) {
        calcMode = SVGAnimationCalcMode::Spline;
        return true;
    } else {
        STARFISH_LOG_WARN("Unknown calcMode value: %s",
                          maybeCalcMode->toUTF8NonGCString().c_str());
    }
    return false;
}

bool SVGAnimationElement::parseKeySplines(GCVector<TimingFunction*>& keySplines)
{
    Optional<String*> maybeKeySplines =
        getAttribute(starfish()->staticStrings()->m_keySplines);

    if (!maybeKeySplines) {
        return false;
    }
    String* keySplinesValue = maybeKeySplines.getValue();

    GCVector<StringView> tokensForKeySplinesValue;
    StringUtils::tokenize(keySplinesValue, ";", 1, tokensForKeySplinesValue);

    for (auto& token : tokensForKeySplinesValue) {
        StringBufferAccessData bad = token.bufferAccessData();
        if (bad.bufferDataKind !=
            StringBufferAccessData::BufferDataKind::ASCIIData) {
            return false;
        }

        CSSTokenVector tokensForKeySpline;
        CSSStyleDeclaration::tokenizeCSSValue(tokensForKeySpline,
                                              bad.asciiData(), bad.length);
        if (tokensForKeySpline.size() != 4) {
            return keySplines.size() != 0;
        }

        float splines[4];
        for (size_t i = 0; i < tokensForKeySpline.size(); i++) {
            CSSTokenValue cssToken = tokensForKeySpline[i].trim();
            if (!CSSPropertyParser::parseNumber(
                    cssToken.data(), cssToken.length(), 0, &splines[i])) {
                return false;
            }
        }

        keySplines.push_back(
            new CubicBezier(splines[0], splines[1], splines[2], splines[3]));
    }

    return true;
}

bool SVGAnimationElement::hasValues()
{
    return hasAttribute(starfish()->staticStrings()->m_values.localName());
}

bool SVGAnimationElement::parseRepeatCount(float& repeatCount)
{
    Optional<String*> maybeRepeatCount =
        getAttribute(starfish()->staticStrings()->m_repeatCount);
    if (!maybeRepeatCount) {
        return false;
    }

    String* repeatCountValue = maybeRepeatCount.value();
    if (repeatCountValue->equals("indefinite")) {
        repeatCount = std::numeric_limits<float>::infinity();
        return true;
    } else {
        struct Args {
            float value;
            bool result;
        } args;
        repeatCountValue->peekUTF8Buffer(
            [](const char* buffer, size_t len, void* data) -> size_t {
                Args* args = static_cast<Args*>(data);
                args->result = CSSPropertyParser::parseNumber(buffer, len, 0,
                                                              &args->value);
                return 0;
            },
            &args);
        if (args.result) {
            repeatCount = args.value;
            return true;
        }
    }
    return false;
}

} // namespace Starfish
