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

#if defined(STARFISH_ENABLE_MULTIMEDIA) && \
    !defined(__StarFishHTMLSourceElement__)
#define __StarFishHTMLSourceElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLSourceElement : public HTMLElement {
public:
    HTMLSourceElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLSourceElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name() override;

    /* Other methods (not in DOM API) */

    // https://html.spec.whatwg.org/multipage/embedded-content.html#the-source-element
    // Attributes
    // NOTE : Current version of HTMLSourceElement considers only media element
    // related case.
    //        (picture element related case has not been considered.
    //         Hence attribute "media", "srcset", "sizes" are not currently
    //         supported)
    String* src();
    void setSrc(String* src);

    String* type();
    void setType(String* type);
};
}

#endif
