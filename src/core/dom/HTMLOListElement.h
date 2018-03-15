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

#ifndef __StarFishHTMLOListElement__
#define __StarFishHTMLOListElement__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLOListElement : public HTMLElement {
public:
    HTMLOListElement(Document* document)
        : HTMLElement(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLOListElement() const override;

    /* 4.4 Interface Node */
    virtual QualifiedName name();

    int32_t start();
    void setStart(int32_t v);

    void didNodeInserted(Node* parent, Node* newChild) override
    {
        HTMLElement::didNodeInserted(parent, newChild);
        setNeedsFrameTreeBuildWithoutSelf();
    }

    void didNodeRemoved(Node* parent, Node* oldChild) override
    {
        HTMLElement::didNodeRemoved(parent, oldChild);
        setNeedsFrameTreeBuildWithoutSelf();
    }
};
}

#endif
