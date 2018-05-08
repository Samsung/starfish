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
    enum Rules {
        UnsetRules,
        NoneRules,
        GroupsRules,
        RowsRules,
        ColsRules,
        AllRules
    };

    enum CellBorders {
        NoBorders,
        InsetBorders,
        SolidBordersRowsOnly,
        SolidBordersColsOnly,
        SolidBorders,
    };

    HTMLTableElement(Document* document)
        : HTMLElement(document)
        , m_tBodies(nullptr)
        , m_rows(nullptr)
        , m_hasBorder(false)
        , m_hasBorderColor(false)
        , m_rules(UnsetRules)
        , m_hasCellPaddingAttribute(false)
        , m_hasCellSpacingAttribute(false)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTableElement() const override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    bool hasCellPaddingAttribute()
    {
        return m_hasCellPaddingAttribute;
    }

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    HTMLTableCaptionElement* caption();
    void setCaption(HTMLTableCaptionElement* caption);
    HTMLTableCaptionElement* createCaption();
    void deleteCaption();

    HTMLTableSectionElement* tHead();
    void setTHead(HTMLTableSectionElement* tHead);
    HTMLTableSectionElement* createTHead();
    void deleteTHead();

    HTMLTableSectionElement* tFoot();
    void setTFoot(HTMLTableSectionElement* tFoot);
    HTMLTableSectionElement* createTFoot();
    void deleteTFoot();

    HTMLCollection* tBodies();
    HTMLTableSectionElement* createTBody();

    HTMLCollection* rows();
    HTMLTableRowElement* insertRow(int32_t index = -1);
    void deleteRow(int32_t index);

    /* Not in HTML5 */
    bool isValidAlign(String* align);
    TextAlignValue alignValue(String* align);

    bool groupRules();
    CellBorders cellBorders();

    String* cellpadding();

private:
    HTMLCollection* m_tBodies;
    HTMLCollection* m_rows;
    bool m_hasBorder;
    bool m_hasBorderColor;
    Rules m_rules;
    bool m_hasCellPaddingAttribute;
    bool m_hasCellSpacingAttribute;
};
}

#endif
