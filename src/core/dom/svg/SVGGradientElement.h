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

#ifndef __StarfishSVGGradientElement__
#define __StarfishSVGGradientElement__

#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"
#include "core/dom/svg/SVGAnimatedTransformList.h"

namespace Starfish {

class SVGSVGElement;

class SVGGradientElement : public SVGElement {
public:
    enum SpreadMethod {
        SVG_SPREADMETHOD_UNKNOWN = 0,
        SVG_SPREADMETHOD_PAD,
        SVG_SPREADMETHOD_REFLECT,
        SVG_SPREADMETHOD_REPEAT
    };

    SVGGradientElement(Document* document, const QualifiedName& qname);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGGradientElement() const override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(SVGGradientElement, m_gradientUnits));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGGradientElement, m_gradientTransform));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGGradientElement, m_spreadMethod));
        SVGElement::fillGCDescriptor(desc);
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual bool needsClipPathAttributes() override
    {
        return false;
    }

    virtual bool needsTransparentAttributes() override
    {
        return false;
    }

    SVGAnimatedEnumeration* gradientUnits();
    SVGAnimatedTransformList* gradientTransform();
    SVGAnimatedEnumeration* spreadMethod();

protected:
    Optional<SVGAnimatedEnumeration*> m_gradientUnits;
    Optional<SVGAnimatedTransformList*> m_gradientTransform;
    Optional<SVGAnimatedEnumeration*> m_spreadMethod;
};
} // namespace Starfish

#endif
