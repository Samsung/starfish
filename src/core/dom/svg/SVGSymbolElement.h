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

#ifndef __StarfishSVGSymbolElement__
#define __StarfishSVGSymbolElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGSymbolElement : public SVGElement {
public:
    SVGSymbolElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGSymbolElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual bool needsPreserveAspectRatioValue() override
    {
        return true;
    }

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool isStructuralElement() override
    {
        return true;
    }

    virtual bool hasViewBox() const override
    {
        return m_hasViewBox;
    }

    virtual Unit::Rect viewBox() const override
    {
        STARFISH_ASSERT(m_hasViewBox);
        return m_viewBox;
    }

    virtual NativeImageData::PreserveAspectRatioAlign preserveAspectRatioAlign()
        override;
    virtual NativeImageData::PreserveAspectRatioMeetOrSlice
    preserveAspectRatioMeetOrSlice() override;

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);

protected:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    bool m_hasViewBox{ false };
    Unit::Rect m_viewBox;

    Optional<SVGAnimatedLength*> m_x;
    Optional<SVGAnimatedLength*> m_y;
};
} // namespace Starfish

#endif
