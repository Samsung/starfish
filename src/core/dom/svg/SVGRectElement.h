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

#ifndef __StarfishSVGRectElement__
#define __StarfishSVGRectElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGSVGElement;

class SVGRectElement : public SVGElement {
public:
    SVGRectElement(Document* document, const QualifiedName& qname)
        : SVGElement(document, qname)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGRectElement() const override;

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

    virtual void didAttributeChanged(QualifiedName name, Nullable<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Nullable<const MutablePropertyValueList*> cssCustomValues) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(rx);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(ry);

private:
    SVGAnimatedLength* m_x{ nullptr };
    SVGAnimatedLength* m_y{ nullptr };
    SVGAnimatedLength* m_width{ nullptr };
    SVGAnimatedLength* m_height{ nullptr };
    SVGAnimatedLength* m_rx{ nullptr };
    SVGAnimatedLength* m_ry{ nullptr };
};
} // namespace Starfish

#endif
