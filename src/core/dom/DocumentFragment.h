/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

    virtual String* nodeName();

    virtual Node* clone();

    Element* getElementById(String* id);
};
}

#endif
