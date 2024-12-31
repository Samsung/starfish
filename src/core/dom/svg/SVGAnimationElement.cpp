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
    default:
        STARFISH_UNIMPLEMENTED("Unimplemented calcMode value");
        break;
    }
    return CubicBezierEaseType::Linear;
}

SVGAnimationElement::SVGAnimationElement(Document* document,
                                         const QualifiedName& qname)
    : SVGElement(document, qname)
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
    STARFISH_UNIMPLEMENTED();
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

    String* targetAttrName = maybeAttributeName;
    keyKind = CSSStyleLookupTrie::lookupCSSStyle(
        targetAttrName->toUTF8NonGCString().data(), targetAttrName->length());

    // TODO: Probably need to differentiate between the allowable keyKinds
    // depending on the target element.
    return keyKind != CSSStyleValuePair::KeyKind::Unknown;
}

bool SVGAnimationElement::parseFrom(CSSStyleValuePair::KeyKind keyKind,
                                    CSSStyleValuePair& from)
{
    Optional<String*> maybeFrom =
        getAttribute(starfish()->staticStrings()->m_from);
    if (!maybeFrom) {
        return false;
    }

    uint8_t option =
        CSSPropertyParser::AllowWithoutUnit | CSSPropertyParser::AllowPercent;
    auto str = maybeFrom->toUTF8NonGCString();
    if (!parseLengthValue(maybeFrom, option, from)) {
        return false;
    }
    from.setKeyKind(keyKind);
    return true;
}

bool SVGAnimationElement::parseTo(CSSStyleValuePair::KeyKind keyKind,
                                  CSSStyleValuePair& to)
{
    Optional<String*> maybeTo = getAttribute(starfish()->staticStrings()->m_to);
    if (!maybeTo) {
        return false;
    }

    uint8_t option =
        CSSPropertyParser::AllowWithoutUnit | CSSPropertyParser::AllowPercent;
    auto str = maybeTo->toUTF8NonGCString();
    if (!parseLengthValue(maybeTo, option, to)) {
        return false;
    }
    to.setKeyKind(keyKind);
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
    auto str = maybeDur->toUTF8NonGCString();
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

    if (maybeFill->equals("remove")) {
        fill = SVGAnimationFill::Remove;
        return true;
    } else if (maybeFill->equals("freeze")) {
        fill = SVGAnimationFill::Freeze;
        return true;
    } else {
        STARFISH_LOG_WARN("Unknown fill value: %s",
                          maybeFill->toUTF8NonGCString().c_str());
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

} // namespace Starfish
