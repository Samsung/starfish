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

#ifndef __StarfishHTMLHTMLTablePartElement__
#define __StarfishHTMLHTMLTablePartElement__

#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLTableElement.h"

namespace Starfish {
class HTMLTablePartElement : public HTMLElement {
public:
    HTMLTablePartElement(Document* document, const QualifiedName& qname)
        : HTMLElement(document, qname)
    {
    }

    virtual bool isHTMLTablePartElement() const override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    bool isValidAlign(String* align);
    TextAlignValue alignValue(String* align);

    bool isValidValign(String* valign);
    VerticalAlignValue valignValue(String* valign);

    HTMLTableElement* findParentTable();

    void additionalBorderRulesTop(CSSStyleValuePairVectorHolder& cssValues,
                                  const char* width, const char* style);
    void additionalBorderRulesRight(CSSStyleValuePairVectorHolder& cssValues,
                                    const char* width, const char* style);
    void additionalBorderRulesBottom(CSSStyleValuePairVectorHolder& cssValues,
                                     const char* width, const char* style);
    void additionalBorderRulesLeft(CSSStyleValuePairVectorHolder& cssValues,
                                   const char* width, const char* style);
    void additionalPadding(CSSStyleValuePairVectorHolder& cssValues,
                           String* padding);
};
} // namespace Starfish
#endif
