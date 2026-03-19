/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGElement__
#define __StarfishSVGElement__

#include "core/dom/Element.h"
#include "core/dom/svg/SVGAnimatedLengthList.h"
#include "core/dom/svg/SVGAnimatedLength.h"
#include "core/dom/svg/SVGAngle.h"
#include "core/dom/svg/SVGNumber.h"
#include "core/style/Style.h"
#include "core/modules/canvas/image/NativeImageData.h"

#define STARFISH_SVG_ANIMATED_LENGTH_GETTER(attrName)                        \
    SVGAnimatedLength* attrName()                                            \
    {                                                                        \
        if (!m_##attrName.hasValue()) {                                      \
            SVGLength* baseVal =                                             \
                new SVGLength(this, staticStrings()->m_##attrName);          \
            SVGLength* animVal =                                             \
                new SVGLength(this, staticStrings()->m_##attrName, baseVal); \
            m_##attrName =                                                   \
                new SVGAnimatedLength(document(), baseVal, animVal);         \
        }                                                                    \
        return m_##attrName.value();                                         \
    }

#define STARFISH_SVG_ANIMATED_LENGTH_GETTER_TYPE_DEFAULT(attrName, type,     \
                                                         defaultValue)       \
    SVGAnimatedLength* attrName()                                            \
    {                                                                        \
        if (!m_##attrName.hasValue()) {                                      \
            SVGLength* baseVal =                                             \
                new SVGLength(this, staticStrings()->m_##attrName,           \
                              SVGLength::type, defaultValue);                \
            SVGLength* animVal =                                             \
                new SVGLength(this, staticStrings()->m_##attrName, baseVal); \
            m_##attrName =                                                   \
                new SVGAnimatedLength(document(), baseVal, animVal);         \
        }                                                                    \
        return m_##attrName.value();                                         \
    }

#define STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(name, name2, customs) \
    {                                                                    \
        auto aniVal = animatedAttributeAsStyleValue(                     \
            staticStrings()->m_##name.localNameAtomic());                \
        if (aniVal) {                                                    \
            auto& s = aniVal.value();                                    \
            s.setKeyKind(CSSStyleValuePair::KeyKind::name2);             \
            cssValues.push_back(s);                                      \
        } else {                                                         \
            CSSStyleValuePair pair;                                      \
            String* name = getAttributeOrVarReferencedValue(             \
                staticStrings()->m_##name, customs);                     \
            if (name->length()) {                                        \
                pair.setKeyKind(CSSStyleValuePair::KeyKind::name2);      \
                pair.setValueKind(CSSStyleValuePair::ValueKind::Length); \
                auto s = name->toUTF8NonGCString();                      \
                if (CSSPropertyParser::parseLength(                      \
                        s.data(),                                        \
                        CSSPropertyParser::AllowPercent |                \
                            CSSPropertyParser::AllowWithoutUnit |        \
                            CSSPropertyParser::AllowNegative,            \
                        &pair)) {                                        \
                    cssValues.push_back(pair);                           \
                }                                                        \
            }                                                            \
        }                                                                \
    }

namespace Starfish {

class SVGClipPathElement;
class SVGFilterElement;
class SVGSVGElement;
class SVGMaskElement;
class ActiveSVGLengthAnimationTask;

class SVGElement : public Element {
    friend class ActiveSVGLengthAnimationTask;

public:
    SVGElement(Document* document, const QualifiedName& qname);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGElement() const override;

    String* xmlbase();
    void setXmlbase(String* str);

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;
    virtual void didNodeRemovedFromDocumentTree() override;

    Optional<SVGElement*> ownerSVGElement();
    Optional<SVGElement*> viewportElement();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual QualifiedName name() override
    {
        return m_name;
    }

    static inline void fillGCDescriptor(GC_word* desc)
    {
        Element::fillGCDescriptor(desc);
        GC_set_bit(desc, GC_WORD_OFFSET(SVGElement, m_animatedAttributes));
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name){};

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        MatchedStyleRules<>& matchedRules,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    virtual bool needsGeometryAttributes()
    {
        return false;
    }

    virtual bool needsSizingAttributes()
    {
        return false;
    }

    virtual bool needsFillAttributes()
    {
        return true;
    }

    virtual bool needsTransparentAttributes()
    {
        return true;
    }

    virtual bool needsStrokeAttributes()
    {
        return true;
    }

    virtual bool needsTransformAttributes()
    {
        return true;
    }

    virtual bool needsPreserveAspectRatioValue()
    {
        return false;
    }

    virtual bool needsClipPathAttributes()
    {
        return true;
    }

    virtual bool isRenderableElement()
    {
        // https://svgwg.org/svg2-draft/render.html#TermRenderableElement
        return false;
    }

    virtual bool isShapeElement()
    {
        // https://svgwg.org/svg2-draft/shapes.html#TermShapeElemenet
        return false;
    }

    virtual bool isStructuralElement()
    {
        // https://svgwg.org/svg2-draft/struct.html#TermStructuralElement
        return false;
    }

    // clip-path, mask, filter, gradient
    virtual bool isPaintServerLikeElement()
    {
        return false;
    }

    int tabIndex() override;

    virtual NativeImageData::PreserveAspectRatioAlign preserveAspectRatioAlign()
    {
        return m_preserveAspectRatioAlign;
    }

    virtual NativeImageData::PreserveAspectRatioMeetOrSlice
    preserveAspectRatioMeetOrSlice()
    {
        return m_preserveAspectRatioMeetOrSlice;
    }

    virtual bool hasViewBox() const
    {
        return false;
    }

    virtual Unit::Rect viewBox() const
    {
        STARFISH_ASSERT_NOT_REACHED();
        return Unit::Rect();
    }

    bool hasClipPath()
    {
        return !style()->clipPath()->equals(String::emptyString);
    }

    bool hasMask()
    {
        // Note : mask property in SVG doemasn't allow multi layer
        return style()->maskImage(0) != nullptr;
    }

    Optional<SVGClipPathElement*> clipPathElement();
    Optional<SVGMaskElement*> maskElement();
    Optional<SVGFilterElement*> filterElement();
    SVGElement* getSVGElementById(const AtomicString& id);
    Optional<SVGElement*> findHrefTarget(String* href);

    virtual void attributeOfPaintServerLikeUpdated(bool alsoNeedsLayout);

    void setAnimatedAttribute(AtomicString s, Optional<Length> rawLengthValue,
                              Optional<StyleTransformData*> rawStringValue,
                              ActiveSVGLengthAnimationTask* task)
    {
        for (auto& e : ensureAnimatedAttributes()) {
            if (std::get<0>(e) == s) {
                std::get<1>(e) = rawLengthValue;
                std::get<2>(e) = rawStringValue;
                computeAttributeChangeDamage(s);
                return;
            }
        }
        ensureAnimatedAttributes().push_back(
            std::make_tuple(s, rawLengthValue, rawStringValue, task));
        computeAttributeChangeDamage(s);
    }

    void removeAnimatedAttribute(AtomicString s,
                                 ActiveSVGLengthAnimationTask* task)
    {
        if (!m_animatedAttributes) {
            return;
        }
        for (size_t i = 0; i < m_animatedAttributes->size(); i++) {
            if (std::get<0>(m_animatedAttributes->at(i)) == s &&
                std::get<3>(m_animatedAttributes->at(i)) == task) {
                m_animatedAttributes->erase(i);
                computeAttributeChangeDamage(s);
                return;
            }
        }
    }

    Optional<Length> animatedLengthAttribute(AtomicString s) const
    {
        if (!m_animatedAttributes) {
            return nullptr;
        }
        for (auto& e : *m_animatedAttributes) {
            if (std::get<0>(e) == s && std::get<1>(e)) {
                return std::get<1>(e);
            }
        }
        return nullptr;
    }

    Optional<StyleTransformData*> animatedTransformAttribute(
        AtomicString s) const
    {
        if (!m_animatedAttributes) {
            return nullptr;
        }
        for (auto& e : *m_animatedAttributes) {
            if (std::get<0>(e) == s && std::get<2>(e)) {
                return std::get<2>(e);
            }
        }
        return nullptr;
    }

    Optional<CSSStyleValuePair> animatedAttributeAsStyleValue(
        AtomicString s) const;

protected:
    virtual void computeAttributeChangeDamage(AtomicString attrName);

    GCVector<std::tuple<AtomicString, Optional<Length>,
                        Optional<StyleTransformData*>,
                        ActiveSVGLengthAnimationTask*>>&
    ensureAnimatedAttributes()
    {
        if (!m_animatedAttributes) {
            m_animatedAttributes =
                new GCVector<std::tuple<AtomicString, Optional<Length>,
                                        Optional<StyleTransformData*>,
                                        ActiveSVGLengthAnimationTask*>>();
        }
        return *m_animatedAttributes.value();
    }

    NativeImageData::PreserveAspectRatioAlign m_preserveAspectRatioAlign;
    NativeImageData::PreserveAspectRatioMeetOrSlice
        m_preserveAspectRatioMeetOrSlice;
    Optional<GCVector<std::tuple<AtomicString, Optional<Length>,
                                 Optional<StyleTransformData*>,
                                 ActiveSVGLengthAnimationTask*>>*>
        m_animatedAttributes;
};
} // namespace Starfish

#endif
