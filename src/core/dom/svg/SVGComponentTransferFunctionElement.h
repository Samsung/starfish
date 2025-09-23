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

#ifndef __StarfishSVGComponentTransferFunctionElement__
#define __StarfishSVGComponentTransferFunctionElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGComponentTransferFunctionElement : public SVGElement {
public:
    enum ComponentTransferType {
        SVG_FECOMPONENTTRANSFER_TYPE_UNKNOWN,
        SVG_FECOMPONENTTRANSFER_TYPE_IDENTITY,
        SVG_FECOMPONENTTRANSFER_TYPE_TABLE,
        SVG_FECOMPONENTTRANSFER_TYPE_DISCRETE,
        SVG_FECOMPONENTTRANSFER_TYPE_LINEAR,
        SVG_FECOMPONENTTRANSFER_TYPE_GAMMA
    };

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGComponentTransferFunctionElement() const override;

    static inline void fillGCDescriptor(GC_word* desc)
    {
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGComponentTransferFunctionElement, m_type));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGComponentTransferFunctionElement,
                                        m_tableValues));
        GC_set_bit(
            desc, GC_WORD_OFFSET(SVGComponentTransferFunctionElement, m_slope));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGComponentTransferFunctionElement,
                                        m_intercept));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGComponentTransferFunctionElement,
                                        m_amplitude));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGComponentTransferFunctionElement,
                                        m_exponent));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGComponentTransferFunctionElement,
                                        m_offset));
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    SVGAnimatedEnumeration* type();
    SVGAnimatedNumberList* tableValues();
    SVGAnimatedNumber* slope();
    SVGAnimatedNumber* intercept();
    SVGAnimatedNumber* amplitude();
    SVGAnimatedNumber* exponent();
    SVGAnimatedNumber* offset();

protected:
    SVGComponentTransferFunctionElement(Document* document,
                                        const QualifiedName& qname)
        : SVGElement(document, qname)
    {
    }

    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    void notifyAttributeOfPaintServerLikeUpdated();

    Optional<SVGAnimatedEnumeration*> m_type;
    Optional<SVGAnimatedNumberList*> m_tableValues;
    Optional<SVGAnimatedNumber*> m_slope;
    Optional<SVGAnimatedNumber*> m_intercept;
    Optional<SVGAnimatedNumber*> m_amplitude;
    Optional<SVGAnimatedNumber*> m_exponent;
    Optional<SVGAnimatedNumber*> m_offset;
};

#define DEFINE_FUNC_ELEMENT(Channel)                                   \
    class SVGFEFunc##Channel##Element                                  \
        : public SVGComponentTransferFunctionElement {                 \
    public:                                                            \
        SVGFEFunc##Channel##Element(Document* document,                \
                                    const QualifiedName& qname)        \
            : SVGComponentTransferFunctionElement::                    \
                  SVGComponentTransferFunctionElement(document, qname) \
        {                                                              \
        }                                                              \
        virtual void init(ScriptBindingInstance* instance,             \
                          void* domObjectPointer) override;            \
        virtual bool isSVGFEFunc##Channel##Element() const override;   \
    };

DEFINE_FUNC_ELEMENT(R)
DEFINE_FUNC_ELEMENT(G)
DEFINE_FUNC_ELEMENT(B)
DEFINE_FUNC_ELEMENT(A)

#undef DEFINE_FUNC_ELEMENT

} // namespace Starfish

#endif
