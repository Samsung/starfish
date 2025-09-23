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

#ifndef __StarfishSVGFECompositeElement__
#define __StarfishSVGFECompositeElement__

#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

class SVGFECompositeElement : public SVGFilterPrimitiveStandardAttributes {
public:
    enum CompositeOperator {
        SVG_FECOMPOSITE_OPERATOR_UNKNOWN = 0,
        SVG_FECOMPOSITE_OPERATOR_OVER,
        SVG_FECOMPOSITE_OPERATOR_IN,
        SVG_FECOMPOSITE_OPERATOR_OUT,
        SVG_FECOMPOSITE_OPERATOR_ATOP,
        SVG_FECOMPOSITE_OPERATOR_XOR,
        SVG_FECOMPOSITE_OPERATOR_ARITHMETIC,
        SVG_FECOMPOSITE_OPERATOR_LIGHTER
    };
    SVGFECompositeElement(Document* document, const QualifiedName& qname)
        : SVGFilterPrimitiveStandardAttributes(document, qname)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGFECompositeElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    SVGAnimatedString* in1();
    SVGAnimatedString* in2();
    SVGAnimatedEnumeration* domOperator();
    SVGAnimatedNumber* k1();
    SVGAnimatedNumber* k2();
    SVGAnimatedNumber* k3();
    SVGAnimatedNumber* k4();

private:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    Optional<SVGAnimatedString*> m_in1;
    Optional<SVGAnimatedString*> m_in2;
    Optional<SVGAnimatedEnumeration*> m_operator;
    Optional<SVGAnimatedNumber*> m_k1;
    Optional<SVGAnimatedNumber*> m_k2;
    Optional<SVGAnimatedNumber*> m_k3;
    Optional<SVGAnimatedNumber*> m_k4;
};
} // namespace Starfish

#endif
