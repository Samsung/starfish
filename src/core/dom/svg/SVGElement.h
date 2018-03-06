/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishSVGElement__
#define __StarFishSVGElement__

#include "StarFish.h"
#include "core/dom/Element.h"
#include "core/util/AttributeName.h"
#include "core/dom/svg/SVGAnimatedLength.h"
#include "core/style/Style.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/modules/canvas/image/NativeImageData.h"

// TODO implement animVal
#define STARFISH_SVG_ANIMATED_LENGTH_GETTER(attrName)                       \
    SVGAnimatedLength* attrName()                                           \
    {                                                                       \
        SVGLength* baseVal =                                                \
            new SVGLength(this, starFish()->staticStrings()->m_##attrName); \
        return new SVGAnimatedLength(document(), baseVal, nullptr);         \
    }

#define STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(name, name2)         \
    {                                                                   \
        CSSStyleValuePair pair;                                         \
        String* name =                                                  \
            getAttributeOrEmpty(starFish()->staticStrings()->m_##name); \
        if (name->length()) {                                           \
            pair.setKeyKind(CSSStyleValuePair::KeyKind::name2);         \
            pair.setValueKind(CSSStyleValuePair::ValueKind::Length);    \
            auto s = name->toUTF8NonGCString();                         \
            if (CSSPropertyParser::parseLength(                         \
                    s.data(), CSSPropertyParser::AllowPercent |         \
                                  CSSPropertyParser::AllowWithoutUnit,  \
                    &pair)) {                                           \
                cssValues.push_back(pair);                              \
            }                                                           \
        }                                                               \
    }

namespace StarFish {

class SVGSVGElement;

class SVGElement : public Element {
public:
    SVGElement(Document* document)
        : Element(document)
        , m_preserveAspectRatioValue(
              NativeImageData::PreserveAspectRatioValue::None)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGElement() const override;

    String* xmlbase();
    void setXmlbase(String* str);

    SVGElement* ownerSVGElement();
    SVGElement* viewportElement();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        Element::fillGCDescriptor(desc);
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

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

    int tabIndex() const override;

    NativeImageData::PreserveAspectRatioValue preserveAspectRatioValue()
    {
        return m_preserveAspectRatioValue;
    }

protected:
    NativeImageData::PreserveAspectRatioValue m_preserveAspectRatioValue;
};

class SVGNamedElement : public SVGElement {
public:
    SVGNamedElement(Document* document, QualifiedName name)
        : SVGElement(document)
        , m_name(name)
    {
    }

    virtual QualifiedName name()
    {
        return m_name;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    QualifiedName m_name;
};
}

#endif
