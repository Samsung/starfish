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

#ifndef __StarfishSVGPathElement__
#define __StarfishSVGPathElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class Path;

class SVGPathElement : public SVGElement {
public:
    SVGPathElement(Document* document, const QualifiedName& qname);

    Optional<Path*> path()
    {
        return m_path;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGPathElement() const override;

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

    virtual void didComputedStyleChanged(
        ComputedStyle* oldStyle, ComputedStyle* newStyle,
        Optional<StyleResolveContext*> ctx) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

private:
    static void parsePath(String* d, Path* path);
    Path* m_path;
};
} // namespace Starfish

#endif
