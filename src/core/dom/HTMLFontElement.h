/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishHTMLFontElement__
#define __StarFishHTMLFontElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLFontElement : public HTMLElement {
public:
    HTMLFontElement(Document* document)
        : HTMLElement(document)
        , m_hasColorAttribute(false)
        , m_hasSizeAttribute(false)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFontElement() const override;
    virtual QualifiedName name() override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    /* DOM APIs */
    String* color();
    void setColor(String* color);

    String* size();
    void setSize(String* size);

private:
    bool m_hasColorAttribute;
    bool m_hasSizeAttribute;
};
}

#endif
