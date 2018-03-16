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

#ifndef __StarFishHTMLListContainer__
#define __StarFishHTMLListContainer__

#include "core/dom/HTMLElement.h"

namespace StarFish {

class HTMLListContainer : public HTMLElement {
public:
    HTMLListContainer(Document* document)
        : HTMLElement(document)
    {
    }

    virtual bool isHTMLListContainer() const override
    {
        return true;
    }

    int32_t start();
    void setStart(int32_t v);

    virtual void didNodeInserted(Node* parent, Node* newChild) override
    {
        HTMLElement::didNodeInserted(parent, newChild);
        setNeedsFrameTreeBuildWithoutSelf();
    }

    virtual void didNodeRemoved(Node* parent, Node* oldChild) override
    {
        HTMLElement::didNodeRemoved(parent, oldChild);
        setNeedsFrameTreeBuildWithoutSelf();
    }
};
}

#endif
