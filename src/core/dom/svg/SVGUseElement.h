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

#ifndef __StarfishSVGUseElement__
#define __StarfishSVGUseElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class ResourceURL;

class SVGUseElement : public SVGElement {
public:
    SVGUseElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGUseElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    Optional<SVGElement*> updateShadowTree();

    Optional<SVGElement*> target() const
    {
        return m_target;
    }

private:
    String* m_href;
    Optional<SVGElement*> m_target;
};
} // namespace Starfish

#endif
