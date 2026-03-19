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

#ifndef __StarfishSVGFilterPrimitiveStandardAttributes__
#define __StarfishSVGFilterPrimitiveStandardAttributes__

#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGAnimatedNumber.h"
#include "core/dom/svg/SVGAnimatedString.h"
#include "core/dom/svg/SVGAnimatedInteger.h"
#include "core/dom/svg/SVGAnimatedBoolean.h"

namespace Starfish {

class SVGFilterElement;

class SVGFilterPrimitiveStandardAttributes : public SVGElement {
public:
    SVGFilterPrimitiveStandardAttributes(Document* document,
                                         const QualifiedName& qname);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGFilterPrimitiveStandardAttributes() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        MatchedStyleRules<>& matchedRules,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

    SVGAnimatedString* output();
    SVGAnimatedString* result()
    {
        return output();
    }

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        SVGElement::fillGCDescriptor(desc);
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes, m_x));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes, m_y));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes,
                                        m_width));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes,
                                        m_height));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGFilterPrimitiveStandardAttributes,
                                        m_output));
    }

    Optional<SVGFilterElement*> filterElement();
    void notifyAttributeOfPaintServerLikeUpdated(bool needsLayoutAlso);
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

private:
    Optional<SVGAnimatedLength*> m_x;
    Optional<SVGAnimatedLength*> m_y;
    Optional<SVGAnimatedLength*> m_width;
    Optional<SVGAnimatedLength*> m_height;
    Optional<SVGAnimatedString*> m_output;
};
} // namespace Starfish

#endif
