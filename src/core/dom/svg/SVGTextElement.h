/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGTextElement__
#define __StarfishSVGTextElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGSVGElement;

class SVGTextElement : public SVGElement {
public:
    // ‘text-anchor’
    //  start | middle | end | inherit
    enum class TextAnchor { START, MIDDLE, END };

    // ‘alignment-baseline'
    // auto | baseline | before-edge | text-before-edge | middle | central |
    // after-edge | text-after-edge | ideographic | alphabetic | hanging |
    // mathematical | inherit
    enum class AlignmentBaseline { AUTO, MIDDLE };

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(SVGTextElement));
        static bool typeInited = false;
        static GC_descr descr;

        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(SVGTextElement)] = { 0 };
            SVGElement::fillGCDescriptor(desc);

            GC_set_bit(desc, GC_WORD_OFFSET(SVGTextElement, m_x));
            GC_set_bit(desc, GC_WORD_OFFSET(SVGTextElement, m_y));

            descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGTextElement));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    SVGTextElement(Document* document, const QualifiedName& qname);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGTextElement() const override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateAttributeNeeded(QualifiedName name);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    void setTextAnchor(TextAnchor value)
    {
        m_textAnchor = value;
    }

    TextAnchor textAnchor()
    {
        return m_textAnchor;
    }

    void setAlignmentBaseline(AlignmentBaseline value)
    {
        m_alignmentBaseline = value;
    }

    AlignmentBaseline alignmentBaseline()
    {
        return m_alignmentBaseline;
    }

    SVGAnimatedLengthList* x();
    SVGAnimatedLengthList* y();

private:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    TextAnchor m_textAnchor{ TextAnchor::START };
    AlignmentBaseline m_alignmentBaseline{ AlignmentBaseline::AUTO };

    SVGAnimatedLengthList* m_x;
    SVGAnimatedLengthList* m_y;
};
} // namespace Starfish

#endif
