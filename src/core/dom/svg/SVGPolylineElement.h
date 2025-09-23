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

#ifndef __StarfishSVGPolylineElement__
#define __StarfishSVGPolylineElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGPolylineElement : public SVGElement {
public:
    SVGPolylineElement(Document* document, const QualifiedName& qname)
        : SVGElement(document, qname)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGPolylineElement() const override;

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual bool isShapeElement() override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

private:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;
};
} // namespace Starfish

#endif
