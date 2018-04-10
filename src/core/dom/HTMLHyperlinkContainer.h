/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLHyperlinkContainer__
#define __StarFishHTMLHyperlinkContainer__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLHyperlinkContainer : public HTMLElement {
public:
    HTMLHyperlinkContainer(Document* document)
        : HTMLElement(document)
    {
    }

    virtual bool isHTMLHyperlinkContainer() const override
    {
        return true;
    }

    String* href();
    void setHref(String* href);

    String* host();
    void setHost(String* host);

    String* pathname();
    void setPathname(String* host);

    String* protocol();
    void setProtocol(String* protocol);

    String* target();
    void setTarget(String* target);
};
}
#endif
