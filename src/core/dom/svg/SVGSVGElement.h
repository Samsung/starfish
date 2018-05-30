/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishSVGSVGElement__
#define __StarFishSVGSVGElement__

#include "core/dom/svg/SVGElement.h"
#include "core/modules/canvas/image/NativeImageData.h"

#define STARFISH_DEFAULT_SVG_WIDTH 300
#define STARFISH_DEFAULT_SVG_HEIGHT 150

namespace StarFish {

class SVGSVGElement : public SVGElement {
public:
    SVGSVGElement(Document* document)
        : SVGElement(document)
        , m_hasViewBox(false)
        , m_viewBox(0, 0, 0, 0)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGSVGElement() const override;

    virtual QualifiedName name() override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    virtual bool needsPreserveAspectRatioValue() override
    {
        return true;
    }

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    bool hasViewBox() const
    {
        return m_hasViewBox;
    }

    Unit::Rect viewBox() const
    {
        STARFISH_ASSERT(m_hasViewBox);
        return m_viewBox;
    }

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

protected:
    bool m_hasViewBox;
    Unit::Rect m_viewBox;
};
}

#endif
