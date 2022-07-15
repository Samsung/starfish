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

#ifndef __StarfishSVGSVGElement__
#define __StarfishSVGSVGElement__

#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGTransform.h"

#define STARFISH_DEFAULT_SVG_WIDTH 300
#define STARFISH_DEFAULT_SVG_HEIGHT 150

namespace Starfish {

class SVGSVGElement : public SVGElement {
public:
    SVGSVGElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGSVGElement() const override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Nullable<const MutablePropertyValueList*> cssCustomValues) override;

    virtual bool needsPreserveAspectRatioValue() override
    {
        return true;
    }

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool needsSizingAttributes() override
    {
        return true;
    }

    virtual bool needsClipPathAttributes()
    {
        return false;
    }

    virtual bool isRenderableElement() override
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

    virtual NativeImageData::PreserveAspectRatioAlign preserveAspectRatioAlign()
        override;
    virtual NativeImageData::PreserveAspectRatioMeetOrSlice
    preserveAspectRatioMeetOrSlice() override;

    virtual void updateSVGAttributeNeeded(QualifiedName name);

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

    SVGNumber* createSVGNumber();
    SVGLength* createSVGLength();
    SVGAngle* createSVGAngle();
    SVGTransform* createSVGTransform();

protected:
    bool m_hasViewBox{ false };
    Unit::Rect m_viewBox;

    SVGAnimatedLength* m_x{ nullptr };
    SVGAnimatedLength* m_y{ nullptr };
    SVGAnimatedLength* m_width{ nullptr };
    SVGAnimatedLength* m_height{ nullptr };
};
} // namespace Starfish

#endif
