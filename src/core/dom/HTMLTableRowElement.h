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

#ifndef __StarFishHTMLTableRowElement__
#define __StarFishHTMLTableRowElement__

#include "core/dom/HTMLTablePartElement.h"

namespace StarFish {

class HTMLTableRowElement : public HTMLTablePartElement {
public:
    HTMLTableRowElement(Document* document)
        : HTMLTablePartElement(document)
        , m_cells(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTableRowElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved);
    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues);

    // Not in HTML5
    String* bgColor();
    void setBgColor(String* bgColor);

    // only for DOM conformance test
    String* ch();
    void setCh(String* ch);

    int32_t rowIndex();

    int32_t sectionRowIndex();

    HTMLCollection* cells();

    HTMLTableCellElement* insertCell(int32_t index = -1);

    void deleteCell(long index);

private:
    HTMLCollection* m_cells;
};
}

#endif
