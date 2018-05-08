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

#ifndef __StarFishHTMLHtmlElement__
#define __StarFishHTMLHtmlElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLHtmlElement : public HTMLElement {
public:
    HTMLHtmlElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLHtmlElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    /* Other methods (not in DOM API) */

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle,
                                         ComputedStyle* newStyle) override;

    HTMLBodyElement* body()
    {
        // root element of html document is HTMLHtmlElement
        // https://www.w3.org/TR/html-markup/html.html
        Node* n = firstChild();
        while (n) {
            if (n->isHTMLBodyElement()) {
                return n->asHTMLBodyElement();
            }
            n = n->nextSibling();
        }

        return nullptr;
    }
};
}

#endif
