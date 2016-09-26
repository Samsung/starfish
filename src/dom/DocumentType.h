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

#ifndef __StarFishDocumentType__
#define __StarFishDocumentType__

#include "dom/Node.h"

namespace StarFish {

class DocumentType : public Node {
public:
    DocumentType(Document* document, String* name, String* publicId, String* systemId)
        : Node(document)
        , m_name(name)
        , m_publicId(publicId)
        , m_systemId(systemId)
    {
#ifdef STARFISH_TC_COVERAGE
        if (name->equals("html"))
            STARFISH_LOG_INFO("+++doctype:!DOCTYPE\n");
#endif
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */

    virtual NodeType nodeType()
    {
        return DOCUMENT_TYPE_NODE;
    }

    virtual String* nodeName()
    {
        return m_name;
    }

    virtual Element* parentElement()
    {
        return nullptr;
    }

    virtual String* textContent()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    /* 4.7 Interface DocumentType */

    String* publicId()
    {
        return m_publicId;
    }

    String* systemId()
    {
        return m_systemId;
    }

    /* Other methods (not in DOM API) */
    virtual bool isDocumentType() const
    {
        return true;
    }

    virtual Node* clone()
    {
        DocumentType* n = new DocumentType(document(), m_name, m_publicId, m_systemId);
        return n;
    }

protected:
    String* m_name;
    String* m_publicId;
    String* m_systemId;
};
}

#endif
