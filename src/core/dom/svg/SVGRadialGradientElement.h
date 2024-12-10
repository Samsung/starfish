/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGRadialGradientElement__
#define __StarfishSVGRadialGradientElement__

#include "SVGAnimatedLength.h"
#include "core/dom/svg/SVGGradientElement.h"

namespace Starfish {

class SVGSVGElement;
class ColorStop;

class SVGRadialGradientElement : public SVGGradientElement {
public:
    SVGRadialGradientElement(Document* document, const QualifiedName& qname)
        : SVGGradientElement(document, qname)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGRadialGradientElement() const override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(SVGRadialGradientElement, m_cx));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGRadialGradientElement, m_cy));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGRadialGradientElement, m_r));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGRadialGradientElement, m_fx));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGRadialGradientElement, m_fy));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGRadialGradientElement, m_fr));

        SVGGradientElement::fillGCDescriptor(desc);
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool needsClipPathAttributes() override
    {
        return false;
    }

    virtual bool needsTransparentAttributes() override
    {
        return false;
    }

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    GCVector<ColorStop*> colorStops();

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(cx);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(cy);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(r);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(fx);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(fy);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(fr);

private:
    SVGAnimatedLength* m_cx{ nullptr };
    SVGAnimatedLength* m_cy{ nullptr };
    SVGAnimatedLength* m_r{ nullptr };
    SVGAnimatedLength* m_fx{ nullptr };
    SVGAnimatedLength* m_fy{ nullptr };
    SVGAnimatedLength* m_fr{ nullptr };
};
} // namespace Starfish

#endif
