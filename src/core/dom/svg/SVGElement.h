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

// TODO implement animVal
#define STARFISH_SVG_ANIMATED_LENGTH_GETTER(attrName)                \
    SVGAnimatedLength* attrName()                                    \
    {                                                                \
        if (m_##attrName == nullptr) {                               \
            SVGLength* baseVal =                                     \
                new SVGLength(this, staticStrings()->m_##attrName);  \
            m_##attrName =                                           \
                new SVGAnimatedLength(document(), baseVal, nullptr); \
        }                                                            \
        return m_##attrName;                                         \
    }

#define STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(name, name2, customs) \
    {                                                                    \
        CSSStyleValuePair pair;                                          \
        String* name = getAttributeOrVarReferencedValue(                 \
            staticStrings()->m_##name, customs);                         \
        if (name->length()) {                                            \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::name2);          \
            pair.setValueKind(CSSStyleValuePair::ValueKind::Length);     \
            auto s = name->toUTF8NonGCString();                          \
            if (CSSPropertyParser::parseLength(                          \
                    s.data(),                                            \
                    CSSPropertyParser::AllowPercent |                    \
                        CSSPropertyParser::AllowWithoutUnit |            \
                        CSSPropertyParser::AllowNegative,                \
                    &pair)) {                                            \
                cssValues.push_back(pair);                               \
            }                                                            \
        }                                                                \
    }

namespace Starfish {

class SVGSVGElement;

class SVGElement : public Element {
public:
    SVGElement(Document* document, const QualifiedName& qname);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGElement() const override;

    String* xmlbase();
    void setXmlbase(String* str);

    SVGElement* ownerSVGElement();
    SVGElement* viewportElement();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual QualifiedName name() override
    {
        return m_name;
    }

    static inline void fillGCDescriptor(GC_word* desc)
    {
        Element::fillGCDescriptor(desc);
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name){};

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Nullable<const MutablePropertyValueList*> cssCustomValues) override;

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

    virtual bool needsMaskAttributes()
    {
        return true;
    }

    virtual bool isRenderableElement()
    {
        // https://svgwg.org/svg2-draft/render.html#TermRenderableElement
        return false;
    }

    int tabIndex() override;

    virtual NativeImageData::PreserveAspectRatioValue preserveAspectRatioValue()
    {
        return m_preserveAspectRatioValue;
    }

    bool hasClipPath()
    {
        return !style()->clipPath()->equals(String::emptyString);
    }

    bool hasMask()
    {
        // Note : mask property in SVG doesn't allow multi layer
        return style()->maskImage(0) != nullptr;
    }

    SVGElement* clipPathElement();

    SVGElement* maskElement();

    SVGElement* getSVGElementById(String* id);

protected:
    NativeImageData::PreserveAspectRatioValue m_preserveAspectRatioValue;
    SVGElement* m_clipPathElement;
    SVGElement* m_maskElement;
};
} // namespace Starfish

#endif
