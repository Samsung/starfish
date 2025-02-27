/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGFilterElement__
#define __StarfishSVGFilterElement__

#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {
class Filter;
class SVGFilterElement : public SVGElement {
public:
    SVGFilterElement(Document* document, const QualifiedName& qname)
        : SVGElement(document, qname)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGFilterElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool needsSizingAttributes() override
    {
        return true;
    }

    virtual bool isPaintServerLikeElement() override
    {
        return true;
    }

    Optional<Filter*> filter();

    SVGAnimatedEnumeration* filterUnits();
    SVGAnimatedEnumeration* primitiveUnits();

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

private:
    Optional<Filter*> m_filter;
    Optional<SVGAnimatedEnumeration*> m_filterUnits;
    Optional<SVGAnimatedEnumeration*> m_primitiveUnits;
    Optional<SVGAnimatedLength*> m_x;
    Optional<SVGAnimatedLength*> m_y;
    Optional<SVGAnimatedLength*> m_width;
    Optional<SVGAnimatedLength*> m_height;
};
} // namespace Starfish

#endif
