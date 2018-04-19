/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLTableElement__
#define __StarFishHTMLTableElement__

#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLTableCaptionElement.h"

namespace StarFish {

class HTMLCollection;

class HTMLTableElement : public HTMLElement {
public:
    HTMLTableElement(Document* document)
        : HTMLElement(document)
        , m_hasCellPaddingAttribute(false)
        , m_hasCellSpacingAttribute(false)
        , m_rows(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTableElement() const override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    bool hasCellPaddingAttribute()
    {
        return m_hasCellPaddingAttribute;
    }

    HTMLTableCaptionElement* caption();
    void setCaption(HTMLTableCaptionElement* caption);

    HTMLTableCaptionElement* createCaption();
    void deleteCaption();

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    /* Not in HTML5 */
    String* cellspacing();
    void setCellspacing(String* cellspacing);

    String* cellpadding();
    void setCellpadding(String* cellpadding);

    HTMLCollection* rows();

    bool isValidAlign(String* align);
    TextAlignValue alignValue(String* align);

private:
    bool m_hasCellPaddingAttribute;
    bool m_hasCellSpacingAttribute;
    HTMLCollection* m_rows;
};
}

#endif
