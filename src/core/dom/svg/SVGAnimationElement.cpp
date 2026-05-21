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
#include "core/animation/AnimationTask.h"
#include "core/animation/SVGAnimationApplier.h"
#include "core/style/Style.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleLookupTrie.h"
#include "core/animation/CubicBezier.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/Document.h"

namespace Starfish {

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

void SVGAnimationElement::didNodeInsertedToDocumentTree()
{
    SVGElement::didNodeInsertedToDocumentTree();
    beginElement();
}

void SVGAnimationElement::didNodeRemovedFromDocumentTree()
{
    SVGElement::didNodeRemovedFromDocumentTree();
    // TODO stop animation
}

void SVGAnimationElement::didAttributeChanged(QualifiedName name,
                                              Optional<String*> old,
                                              String* value,
                                              bool attributeCreated,
                                              bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();

    // TODO: Apply changed valued to active animations.

    if (ss->m_attributeName == name) {
        CSSStyleValuePair::KeyKind keyKind;
        if (parseAttributeName(value, keyKind)) {
            if (!m_attributeName.hasValue() ||
                m_attributeName.value() != keyKind) {
                m_attributeName = keyKind;
                m_attributeNameAsString = value;
                updateValueFamilyAttribute();
            }
        } else if (attributeRemoved) {
            m_attributeName.reset();
            m_attributeNameAsString.reset();
            updateValueFamilyAttribute();
        } else {
            m_attributeName = CSSStyleValuePair::KeyKind::Unknown;
            m_attributeNameAsString = value;
            updateValueFamilyAttribute();
        }
    }

    if (m_attributeName.hasValue()) {
        if (ss->m_from == name) {
            updateFromTo(value, m_from);
        } else if (ss->m_to == name) {
            updateFromTo(value, m_to);
        } else if (ss->m_values == name) {
            updateValues(value);
        }
    }

    if (ss->m_dur == name) {
        CSSTime dur;
        if (parseDur(value, dur)) {
            if (!m_dur.hasValue() || m_dur.value() != dur) {
                m_dur = dur;
            }
        } else {
            m_dur.reset();
        }
    }
    if (ss->m_begin == name) {
        CSSTime begin;
        if (parseDur(value, begin)) {
            if (!m_begin.hasValue() || m_begin.value() != begin) {
                m_begin = begin;
            }
        } else {
            m_begin.reset();
        }
    } else if (ss->m_fill == name) {
        SVGAnimationFill fill;
        if (parseFill(value, fill)) {
            if (!m_fill.hasValue() || m_fill.value() != fill) {
                m_fill = fill;
            }
        } else {
            m_fill.reset();
        }
    } else if (ss->m_repeatCount == name) {
        float repeatCount;
        if (parseRepeatCount(value, repeatCount)) {
            if (!m_repeatCount.hasValue() ||
                m_repeatCount.value() != repeatCount) {
                m_repeatCount = repeatCount;
            }
        } else {
            m_repeatCount.reset();
        }
    } else if (ss->m_calcMode == name) {
        SVGAnimationCalcMode calcMode;
        if (parseCalcMode(value, calcMode)) {
            if (!m_calcMode.hasValue() || m_calcMode.value() != calcMode) {
                m_calcMode = calcMode;
            }
        } else {
            m_calcMode.reset();
        }
    } else if (ss->m_keySplines == name) {
        GCVector<TimingFunction*> keySplines;
        if (parseKeySplines(value, keySplines)) {
            if (!m_keySplines.hasValue() ||
                m_keySplines.value().size() != keySplines.size() ||
                !std::equal(
                    m_keySplines.value().begin(), m_keySplines.value().end(),
                    keySplines.begin(),
                    [](const TimingFunction* lhd, const TimingFunction* rhd) {
                        return *lhd == *rhd;
                    })) {
                m_keySplines = std::move(keySplines);
            }
        } else {
            m_keySplines.reset();
        }
    } else if (name == ss->m_onbegin) {
        setAttributeEventListener(ss->m_beginEvent, value, this);
    } else if (name == ss->m_onend) {
        setAttributeEventListener(ss->m_endEvent, value, this);
    } else if (name == ss->m_onrepeat) {
        setAttributeEventListener(ss->m_repeatEvent, value, this);
    } else if (ss->m_href == name || ss->m_xlinkHref == name ||
               (!name.hasPrefix() &&
                ss->m_xlinkHref.hasSameNamespaceURI(name.namespaceURI()) &&
                ss->m_xlinkHref.hasSameLocalName(name.localName()))) {
        if (attributeRemoved) {
            m_href.reset();
        } else {
            m_href = value;
        }
    }
}

Optional<Element*> SVGAnimationElement::targetElement()
{
    Optional<Element*> targetElement;
    if (m_href) {
        targetElement = findHrefTarget(m_href.value()).valueOr(nullptr);
    } else {
        targetElement = parentElement();
    }

    if (!targetElement || !targetElement->isSVGElement()) {
        return Optional<Element*>();
    }
    return targetElement;
}

void SVGAnimationElement::beginElement()
{
    document()->registerSVGAnimateElementsNeedExecuteAnimation(this);
}

void SVGAnimationElement::beginElementAt(float offset)
{
    if (!m_attributeName.hasValue()) {
        STARFISH_UNIMPLEMENTED("Handle invalid animation name.");
        return;
    }

    beginElementAtInternal(offset, m_attributeName.value(), m_from, m_to,
                           m_values);
}

void SVGAnimationElement::beginElementAtInternal(
    float offset, CSSStyleValuePair::KeyKind keyKind,
    const Optional<CSSStyleValuePair>& from,
    const Optional<CSSStyleValuePair>& to,
    const Optional<GCVector<CSSStyleValuePair>>& values)
{
    // TODO: Apply offset.

    AnimationKeyframes* animationKeyframes = new AnimationKeyframes();

    // set duration
    if (m_dur.hasValue()) {
        animationKeyframes->setDuration(m_dur.value());
    } else {
        // TODO: handle empty duration.
        // Fire beginEvent but never fire endEvent.
        STARFISH_UNIMPLEMENTED("Handle invalid duration");
        return;
    }

    // set begin
    if (m_begin.hasValue()) {
        animationKeyframes->setDelay(m_begin.value());
    }

    // set iteration count. The default value is 1.0.
    float repeatCount = 1.0f;
    if (m_repeatCount.hasValue()) {
        repeatCount = m_repeatCount.value();
    }
    animationKeyframes->setIterationCount(repeatCount);

    // set fill mode. The default value is "None".
    AnimationFillModeValue fillMode = AnimationFillModeValue::None;
    if (m_fill.hasValue()) {
        fillMode = svgAnimationFillToAnimationFillModeValue(m_fill.value());
    }
    animationKeyframes->setFillMode(fillMode);

    GCVector<CSSStyleValuePair> valueList;
    if (values.hasValue()) {
        valueList = values.value();
    } else if (from.hasValue() && to.hasValue()) {
        valueList.push_back(from.value());
        valueList.push_back(to.value());
    }

    // check values. At least two values are required.
    if (valueList.size() < 2) {
        STARFISH_UNIMPLEMENTED("Handle wrong size values");
        return;
    }

    // set calc mode. The default value is "linear".
    SVGAnimationCalcMode calcMode = SVGAnimationCalcMode::Linear;
    if (m_calcMode.hasValue()) {
        calcMode = m_calcMode.value();
    }

    // Check key splines. It size must match the number of values - 1.
    if (calcMode == SVGAnimationCalcMode::Spline) {
        if (!m_keySplines.hasValue() ||
            m_keySplines.value().size() != valueList.size() - 1) {
            // Fallback guarantee: An animation is to occur, but it should not
            // cause any changes.
            // FIXME: If you think of a better way, please replace it.
            // FIXME: In this case, improve it so that only minimal rendering
            // occurs.
            if (!convertFallbackValues(keyKind, valueList)) {
                STARFISH_LOG_ERROR("Failed to convert fallback values.");
                return;
            }
            calcMode = SVGAnimationCalcMode::Linear; // fallback to linear.
        }
    }

    // Set timing function based on calc mode.
    CubicBezierEaseType easeType =
        svgAnimationCalcModeToCubicBezierEaseType(calcMode);
    if (easeType != CubicBezierEaseType::Custom) {
        animationKeyframes->setTimingFunction(
            CubicBezier::createCubicBezier(easeType));
    }

    // Add keyframes using values to animationKeyframes.
    addAnimationKeyframe(keyKind, animationKeyframes, valueList, easeType,
                         m_keySplines);

    // get target element, if is not exist, return.
    Optional<Element*> maybeTargetElement = targetElement();
    if (!maybeTargetElement) {
        STARFISH_UNIMPLEMENTED("Handle invalid animation target element.");
        return;
    }
    Element* targetElement = maybeTargetElement.value();

    // Apply animation for svg.
    m_animationKeyframes = animationKeyframes;
    SVGAnimationApplier applier(targetElement, this);
    if (!applier.apply()) {
        m_animationKeyframes = nullptr;
        STARFISH_LOG_ERROR("Failed to apply animation.");
        return;
    }

    webView()->updateActiveAnimationExecutorRegistration(
        document()->animationExecutor());
    setNeedsStyleRecalcForAnimation();

    m_declarations->clear();
}

bool SVGAnimationElement::hasValidAttributes()
{
    Optional<Element*> maybeTargetElement = targetElement();
    return maybeTargetElement.hasValue() && m_attributeName.hasValue() &&
           (m_values.hasValue() || (m_from.hasValue() && m_to.hasValue()));
}

bool SVGAnimationElement::parseAttributeName(
    const String* attributeNameValue, CSSStyleValuePair::KeyKind& keyKind)
{
    attributeNameValue->peekUTF8Buffer(
        [](const char* buffer, size_t len, void* data) -> size_t {
            CSSStyleValuePair::KeyKind* keyKind =
                static_cast<CSSStyleValuePair::KeyKind*>(data);
            *keyKind = CSSStyleLookupTrie::lookupCSSStyle(buffer, len);
            return 0;
        },
        &keyKind);

    // TODO: Probably need to differentiate between the allowable keyKinds
    // depending on the target element.
    return keyKind != CSSStyleValuePair::KeyKind::Unknown;
}

bool SVGAnimationElement::parseValues(CSSStyleValuePair::KeyKind keyKind,
                                      const String* valuesValue,
                                      GCVector<CSSStyleValuePair>& values)
{
    GCVector<StringView> tokens;
    StringUtils::tokenize(const_cast<String*>(valuesValue), ";", 1, tokens);

    for (auto& token : tokens) {
        CSSStyleValuePair pair;
        if (!parseValue(keyKind, &token, pair)) {
            return false;
        }
        values.push_back(pair);
    }

    return true;
}

bool SVGAnimationElement::parseValue(CSSStyleValuePair::KeyKind keyKind,
                                     const String* value,
                                     CSSStyleValuePair& pair)
{
    // handle non-computed style attribute
    if (keyKind == CSSStyleValuePair::KeyKind::Unknown) {
        CSSTokenVector tokens;
        tokens.push_back(CSSTokenValue(value->toUTF8NonGCString()));
        return pair.updateValueLength(tokens,
                                      CSSPropertyParser::AllowWithoutUnit |
                                          CSSPropertyParser::AllowPercent);
    }
    // Parse each value in values using the rules for parsing the attribute
    // identified by the ‘attributeName’ attributes.
    // Note that ‘attributeName’ corresponds to an attribute name or a CSS
    // property name.

    struct Args {
        CSSStyleValuePair::KeyKind keyKind;
        CSSStyleValuePair* pair = nullptr;
        CSSStyleDeclaration* declarations = nullptr;
        bool ret = false;
    } args;

    args.keyKind = keyKind;
    args.pair = &pair;
    args.declarations = m_declarations;

    value->peekUTF8Buffer(
        [](const char* buffer, size_t len, void* data) -> size_t {
            Args* p = static_cast<Args*>(data);
            CSSStyleValuePair::KeyKind keyKind = p->keyKind;
            CSSStyleDeclaration* declarations = p->declarations;
            p->ret =
                declarations->setPropertyInternal(keyKind, buffer, len, false);
            if (p->ret) {
                *p->pair = declarations->getCSSValuePair(keyKind);
            }

            return 0;
        },
        &args);

    return args.ret;
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

bool SVGAnimationElement::parseDur(const String* durValue, CSSTime& duration)
{
    CSSStyleValuePair temp;
    auto str = durValue->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, str.c_str(), str.length());
    if (temp.updateValueTime(tokens, 0)) {
        duration = temp.timeValue();
        return true;
    } else if (temp.updateValueTime(tokens,
                                    CSSPropertyParser::AllowWithoutUnit)) {
        duration = CSSTime(temp.timeValue().value() * 1000);
        return true;
    }
    return false;
}

bool SVGAnimationElement::parseFill(const String* fillValue,
                                    SVGAnimationFill& fill)
{
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

bool SVGAnimationElement::parseRepeatCount(const String* repeatCountValue,
                                           float& repeatCount)
{
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

bool SVGAnimationElement::parseCalcMode(const String* caclModeValue,
                                        SVGAnimationCalcMode& calcMode)
{
    if (caclModeValue->equals("discrete")) {
        calcMode = SVGAnimationCalcMode::Discrete;
        return true;
    } else if (caclModeValue->equals("linear")) {
        calcMode = SVGAnimationCalcMode::Linear;
        return true;
    } else if (caclModeValue->equals("paced")) {
        calcMode = SVGAnimationCalcMode::Paced;
        return true;
    } else if (caclModeValue->equals("spline")) {
        calcMode = SVGAnimationCalcMode::Spline;
        return true;
    } else {
        STARFISH_LOG_WARN("Unknown calcMode value: %s",
                          caclModeValue->toUTF8NonGCString().c_str());
    }
    return false;
}

bool SVGAnimationElement::parseKeySplines(const String* keySplinesValue,
                                          GCVector<TimingFunction*>& keySplines)
{
    GCVector<StringView> tokensForKeySplinesValue;
    StringUtils::tokenize(const_cast<String*>(keySplinesValue), ";", 1,
                          tokensForKeySplinesValue);

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

void SVGAnimationElement::updateValueFamilyAttribute()
{
    if (!m_attributeName.hasValue()) {
        m_from.reset();
        m_to.reset();
        m_values.reset();
        return;
    }

    Optional<String*> maybeFrom =
        getAttribute(starfish()->staticStrings()->m_from);
    if (maybeFrom) {
        updateFromTo(maybeFrom.value(), m_from);
    }

    Optional<String*> maybeTo = getAttribute(starfish()->staticStrings()->m_to);
    if (maybeTo) {
        updateFromTo(maybeTo.value(), m_to);
    }

    Optional<String*> maybeValues =
        getAttribute(starfish()->staticStrings()->m_values);
    if (maybeValues) {
        updateValues(maybeValues.value());
    }
}

void SVGAnimationElement::updateFromTo(String* value,
                                       Optional<CSSStyleValuePair>& output)
{
    CSSStyleValuePair temp;
    if (parseFromTo(m_attributeName.value(), value, temp)) {
        if (!output.hasValue() || output.value() != temp) {
            output = temp;
        }
    } else {
        output.reset();
    }
}

void SVGAnimationElement::updateValues(String* value)
{
    GCVector<CSSStyleValuePair> values;
    if (parseValues(m_attributeName.value(), value, values)) {
        if (!m_values.hasValue() || m_values.value().size() != values.size() ||
            !std::equal(m_values.value().begin(), m_values.value().end(),
                        values.begin())) {
            m_values = std::move(values);
        }
    } else {
        m_values.reset();
    }
}

void SVGAnimationElement::addAnimationKeyframe(
    CSSStyleValuePair::KeyKind keyKind, AnimationKeyframes* animationKeyframes,
    const GCVector<CSSStyleValuePair>& values, CubicBezierEaseType easeType,
    Optional<GCVector<TimingFunction*>> maybeKeySplines)
{
    for (size_t i = 0; i < values.size(); ++i) {
        AnimationKeyframe* keyframe = new AnimationKeyframe();
        double offset = 100.0 / (values.size() - 1);
        double keyframeSelector = (offset * i) / 100;
        keyframe->setKeyframeSelector(keyframeSelector);
        keyframe->addProperty(keyKind, values[i]);
        keyframe->setDuration(animationKeyframes->duration());
        if (easeType == CubicBezierEaseType::Custom && i < values.size() - 1) {
            keyframe->setTimingFunction(maybeKeySplines.getValue()[i]);
        } else {
            keyframe->setTimingFunction(animationKeyframes->timingFunction());
        }
        animationKeyframes->animationKeyframeList().push_back(keyframe);
    }
}

EventListener* SVGAnimationElement::onbegin()
{
    QualifiedName attr = staticStrings()->m_beginEvent;
    return attributeEventListener(attr);
}

void SVGAnimationElement::setOnbegin(EventListener* onbegin)
{
    QualifiedName attr = staticStrings()->m_beginEvent;
    if (onbegin) {
        setAttributeEventListener(attr, onbegin);
    } else {
        clearAttributeEventListener(attr);
    }
}

EventListener* SVGAnimationElement::onend()
{
    QualifiedName attr = staticStrings()->m_endEvent;
    return attributeEventListener(attr);
}

void SVGAnimationElement::setOnend(EventListener* onend)
{
    QualifiedName attr = staticStrings()->m_endEvent;
    if (onend) {
        setAttributeEventListener(attr, onend);
    } else {
        clearAttributeEventListener(attr);
    }
}

EventListener* SVGAnimationElement::onrepeat()
{
    QualifiedName attr = staticStrings()->m_repeatEvent;
    return attributeEventListener(attr);
}

void SVGAnimationElement::setOnrepeat(EventListener* onrepeat)
{
    QualifiedName attr = staticStrings()->m_repeatEvent;
    if (onrepeat) {
        setAttributeEventListener(attr, onrepeat);
    } else {
        clearAttributeEventListener(attr);
    }
}

} // namespace Starfish
