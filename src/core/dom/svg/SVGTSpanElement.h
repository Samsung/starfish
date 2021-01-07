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

#ifndef __StarfishSVGTSpanElement__
#define __StarfishSVGTSpanElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGSVGElement;

class SVGTSpanElement : public SVGElement {
public:
    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(SVGTSpanElement));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(SVGTSpanElement)] = { 0 };
            SVGElement::fillGCDescriptor(desc);
            descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGTSpanElement));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

    SVGTSpanElement(Document* document, const QualifiedName& qname)
        : SVGElement(document, qname)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGTSpanElement() const override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;
};
} // namespace Starfish

#endif
