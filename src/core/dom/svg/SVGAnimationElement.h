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

#ifndef __StarfishSVGAnimationElement__
#define __StarfishSVGAnimationElement__

#include "core/dom/svg/SVGElement.h"
#include "core/style/StyleAnimationData.h"

namespace Starfish {

enum class CubicBezierEaseType : uint8_t;
class CSSStyleDeclaration;
class TimingFunction;

// https://svgwg.org/specs/animations/#FillAttribute
enum class SVGAnimationFill {
    Freeze,
    Remove,
};

AnimationFillModeValue svgAnimationFillToAnimationFillModeValue(
    SVGAnimationFill fill);

// https://svgwg.org/specs/animations/#CalcModeAttribute
enum class SVGAnimationCalcMode {
    Discrete,
    Linear,
    Paced,
    Spline,
};

CubicBezierEaseType svgAnimationCalcModeToCubicBezierEaseType(
    SVGAnimationCalcMode calcMode);

class SVGAnimationElement : public SVGElement {
public:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        SVGElement::fillGCDescriptor(desc);
        // Fill GC descriptor here if needed
        GC_set_bit(desc, GC_WORD_OFFSET(SVGAnimationElement, m_declarations));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGAnimationElement, m_animationKeyframes));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGAnimationElement, m_values));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGAnimationElement, m_keySplines));
    }

    SVGAnimationElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGAnimationElement() const override;

    virtual bool needsClipPathAttributes() override
    {
        return false;
    }

    virtual bool needsTransparentAttributes() override
    {
        return false;
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    Optional<Element*> targetElement();

    Optional<AnimationKeyframes*> animationKeyframes()
    {
        return m_animationKeyframes;
    }

    void beginElement();

    virtual void beginElementAt(float offset);

protected:
    bool hasValidAttributes();

    bool parseAttributeName(const String* attributeNameValue,
                            CSSStyleValuePair::KeyKind& keyKind);
    bool parseValues(CSSStyleValuePair::KeyKind keyKind,
                     const String* valuesValue,
                     GCVector<CSSStyleValuePair>& values);
    bool parseValue(CSSStyleValuePair::KeyKind keyKind, const char* buffer,
                    size_t len, CSSStyleValuePair& pair);
    bool convertFallbackValues(CSSStyleValuePair::KeyKind keyKind,
                               GCVector<CSSStyleValuePair>& values);

    virtual bool parseFrom(CSSStyleValuePair::KeyKind keyKind,
                           const String* fromValue, CSSStyleValuePair& values);
    virtual bool parseTo(CSSStyleValuePair::KeyKind keyKind,
                         const String* toValue, CSSStyleValuePair& to);
    bool parseFromAndToInternal(CSSStyleValuePair::KeyKind keyKind,
                                const String* value, CSSStyleValuePair& values);
    bool parseDur(const String* durValue, CSSTime& duration);
    bool parseFill(const String* fillValue, SVGAnimationFill& fill);
    bool parseRepeatCount(const String* repeatCountValue, float& repeatCount);
    bool parseCalcMode(const String* caclModeValue,
                       SVGAnimationCalcMode& calcMode);
    bool parseKeySplines(const String* keySplinesValue,
                         GCVector<TimingFunction*>& keySplines);

    void AddAnimationKeyframe(
        CSSStyleValuePair::KeyKind keyKind,
        AnimationKeyframes* animationKeyframes,
        const GCVector<CSSStyleValuePair>& values, CubicBezierEaseType easeType,
        Optional<GCVector<TimingFunction*>> maybeKeySplines);

    CSSStyleDeclaration* m_declarations;

    Optional<AnimationKeyframes*> m_animationKeyframes;
    Optional<CSSStyleValuePair::KeyKind> m_animationName;
    Optional<CSSStyleValuePair> m_from;
    Optional<CSSStyleValuePair> m_to;
    Optional<GCVector<CSSStyleValuePair>> m_values;
    Optional<CSSTime> m_dur;
    Optional<SVGAnimationFill> m_fill;
    Optional<float> m_repeatCount;
    Optional<SVGAnimationCalcMode> m_calcMode;
    Optional<GCVector<TimingFunction*>> m_keySplines;
};
} // namespace Starfish

#endif
