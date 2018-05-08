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

#ifndef __StarFishDocumentFragment__
#define __StarFishDocumentFragment__

#include "core/dom/Node.h"

namespace StarFish {

class DocumentFragment : public Node {
public:
    DocumentFragment(Document* document)
        : Node(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDocumentFragment() const override;

    /* 4.4 Interface Node */
    virtual NodeType nodeType() const override
    {
        return Node::NodeType::DOCUMENT_FRAGMENT_NODE;
    }

    virtual String* nodeName() override;

    virtual Node* clone() override;

    Element* getElementById(String* id);
};
}

#endif
