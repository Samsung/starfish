/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGEllipseElement__
#define __StarfishSVGEllipseElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGEllipseElement : public SVGElement {
public:
    SVGEllipseElement(Document* document, const QualifiedName& qname)
        : SVGElement(document, qname)
    {
        STARFISH_ASSERT(document != nullptr);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGEllipseElement() const override;

    virtual bool needsGeometryAttributes() override
    {
        return false;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(cx);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(cy);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(rx);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(ry);
};
} // namespace Starfish

#endif
