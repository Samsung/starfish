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

#ifndef __StarfishSVGFEColorMatrixElement__
#define __StarfishSVGFEColorMatrixElement__

#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

class SVGFEColorMatrixElement : public SVGFilterPrimitiveStandardAttributes {
public:
    enum MatrixTypes {
        SVG_FECOLORMATRIX_TYPE_UNKNOWN = 0,
        SVG_FECOLORMATRIX_TYPE_MATRIX,
        SVG_FECOLORMATRIX_TYPE_SATURATE,
        SVG_FECOLORMATRIX_TYPE_HUEROTATE,
        SVG_FECOLORMATRIX_TYPE_LUMINANCETOALPHA
    };

    SVGFEColorMatrixElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGFEColorMatrixElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    SVGAnimatedString* in();
    SVGAnimatedString* in1()
    {
        return in();
    }
    SVGAnimatedEnumeration* type();
    SVGAnimatedNumberList* values();

private:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    Optional<SVGAnimatedString*> m_in;
    Optional<SVGAnimatedEnumeration*> m_type;
    Optional<SVGAnimatedNumberList*> m_values;
};
} // namespace Starfish

#endif
