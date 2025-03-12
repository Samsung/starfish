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

    Optional<Element*> targetElement();

    Optional<AnimationKeyframes*> animationKeyframes()
    {
        return m_animationKeyframes;
    }

    void beginElement();

    virtual void beginElementAt(float offset);

protected:
    bool parseAttributeName(CSSStyleValuePair::KeyKind& keyKind);
    bool parseValues(CSSStyleValuePair::KeyKind keyKind,
                     GCVector<CSSStyleValuePair>& values);
    bool parseValue(CSSStyleValuePair::KeyKind keyKind, const char* buffer,
                    size_t len, CSSStyleValuePair& pair);
    bool convertFallbackValues(CSSStyleValuePair::KeyKind keyKind,
                               GCVector<CSSStyleValuePair>& values);

    bool parseFrom(CSSStyleValuePair::KeyKind keyKind,
                   GCVector<CSSStyleValuePair>& values);
    bool parseTo(CSSStyleValuePair::KeyKind keyKind,
                 GCVector<CSSStyleValuePair>& values);
    bool parseFromAndToInternal(CSSStyleValuePair::KeyKind keyKind,
                                String* value,
                                GCVector<CSSStyleValuePair>& values);
    bool parseDur(CSSTime& duration);
    bool parseFill(SVGAnimationFill& fill);
    bool parseCalcMode(SVGAnimationCalcMode& calcMode);
    bool parseKeySplines(GCVector<TimingFunction*>& keySplines);
    bool hasValues();
    bool parseRepeatCount(float& repeatCount);

    CSSStyleDeclaration* m_declarations;
    Optional<AnimationKeyframes*> m_animationKeyframes;
};
} // namespace Starfish

#endif
