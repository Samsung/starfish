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

namespace Starfish {

class SVGMaskElement : public SVGElement {
public:
    SVGMaskElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGMaskElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Nullable<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Nullable<const MutablePropertyValueList*> cssCustomValues) override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x1);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y1);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x2);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y2);

private:
    SVGAnimatedLength* m_x1{ nullptr };
    SVGAnimatedLength* m_y1{ nullptr };
    SVGAnimatedLength* m_x2{ nullptr };
    SVGAnimatedLength* m_y2{ nullptr };
};
} // namespace Starfish

#endif
