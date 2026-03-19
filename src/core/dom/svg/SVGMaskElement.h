/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGMaskElement__
#define __StarfishSVGMaskElement__

#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

class SVGMaskElement : public SVGElement {
public:
    SVGMaskElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGMaskElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        MatchedStyleRules<>& matchedRules,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool needsSizingAttributes() override
    {
        return true;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual bool isPaintServerLikeElement() override
    {
        return true;
    }

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

    SVGAnimatedEnumeration* maskUnits();
    SVGAnimatedEnumeration* maskContentUnits();

private:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    Optional<SVGAnimatedLength*> m_x;
    Optional<SVGAnimatedLength*> m_y;
    Optional<SVGAnimatedLength*> m_width;
    Optional<SVGAnimatedLength*> m_height;
    Optional<SVGAnimatedEnumeration*> m_maskUnits;
    Optional<SVGAnimatedEnumeration*> m_maskContentUnits;
};
} // namespace Starfish

#endif
