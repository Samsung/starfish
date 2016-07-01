/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "dom/Node.h"

namespace StarFish {

class DocumentFragment : public Node {
public:
    DocumentFragment(Document* document)
        : Node(document)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */
    virtual NodeType nodeType()
    {
        return Node::NodeType::DOCUMENT_FRAGMENT_NODE;
    }

    virtual String* nodeName();
    virtual void setTextContent(String* val);
    virtual String* textContent()
    {
        if ((nodeType() == TEXT_NODE) || (nodeType() == DOCUMENT_FRAGMENT_NODE)) {
            return textContent();
        }
        String* str = String::createASCIIString("");
        for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
            if (child->nodeType() == TEXT_NODE || child->nodeType() == ELEMENT_NODE) {
                str = str->concat(child->textContent());
            }
        }
        return str;
    }

    virtual Node* clone();

    /* Other methods (not in DOM API) */

    virtual bool isDocumentFragment() const
    {
        return true;
    }

    Text* asText()
    {
        STARFISH_ASSERT(isText());
        return (Text*)this;
    }

private:
};
}

#endif
