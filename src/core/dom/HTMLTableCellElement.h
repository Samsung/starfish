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

#ifndef __StarFishHTMLTableCellElement__
#define __StarFishHTMLTableCellElement__

#include "core/dom/HTMLTablePartElement.h"

namespace StarFish {

class HTMLTableElement;

class HTMLTableCellElement : public HTMLTablePartElement {
public:
    static const int MAX_COLSPAN = 1000;
    static const int MAX_ROWSPAN = 65534;

    HTMLTableCellElement(Document* document)
        : HTMLTablePartElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTableCellElement() const override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues);

    HTMLTableElement* tableElement();

    // 4.4 Interface Node
    virtual QualifiedName name() = 0;

    // table cell related

    uint32_t colSpan();
    void setColSpan(uint32_t colSpan);

    uint32_t rowSpan();
    void setRowSpan(uint32_t rowSpan);

    // only for DOM conformance test
    String* ch();
    void setCh(String* ch);
};
}

#endif
