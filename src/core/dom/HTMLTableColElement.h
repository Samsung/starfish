/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishHTMLTableColElement__
#define __StarfishHTMLTableColElement__

#include "core/dom/HTMLTablePartElement.h"

namespace Starfish {

class HTMLTableColElement : public HTMLTablePartElement {
public:
    HTMLTableColElement(Document* document, const QualifiedName& qname)
        : HTMLTablePartElement(document, qname)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLTableColElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    /* Other methods (not in DOM API) */

    void setSpan(uint32_t span);
    uint32_t span();

    // only for DOM conformance test
    String* ch();
    void setCh(String* ch);
};
} // namespace Starfish

#endif
