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

#ifndef __StarfishSVGLinearGradientElement__
#define __StarfishSVGLinearGradientElement__

#include "SVGAnimatedLength.h"
#include "core/dom/svg/SVGGradientElement.h"

namespace Starfish {

class SVGSVGElement;
class ColorStop;

class SVGLinearGradientElement : public SVGGradientElement {
public:
    SVGLinearGradientElement(Document* document, const QualifiedName& qname)
        : SVGGradientElement(document, qname)
    {
        x2()->baseVal()->setValueAsString(String::createASCIIString("100%"),
                                          false);
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGLinearGradientElement() const override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(SVGLinearGradientElement, m_x1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGLinearGradientElement, m_y1));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGLinearGradientElement, m_x2));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGLinearGradientElement, m_y2));
        SVGGradientElement::fillGCDescriptor(desc);
    }

    virtual void didAttributeChanged(QualifiedName name, Nullable<String*> old,
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

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    GCVector<ColorStop*> colorStops();

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
