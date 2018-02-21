/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

// TODO implement animVal
#define STARFISH_SVG_ANIMATED_LENGTH_GETTER(attrName)                       \
    SVGAnimatedLength* attrName()                                           \
    {                                                                       \
        SVGLength* baseVal =                                                \
            new SVGLength(this, starFish()->staticStrings()->m_##attrName); \
        return new SVGAnimatedLength(document(), baseVal, baseVal);         \
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

    int tabIndex() const override;
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
