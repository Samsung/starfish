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

#ifndef __StarFishDocumentType__
#define __StarFishDocumentType__

#include "core/dom/Node.h"

namespace StarFish {

class DocumentType : public Node {
public:
    DocumentType(Document* document, String* name, String* publicId,
                 String* systemId)
        : Node(document)
        , m_name(name)
        , m_publicId(publicId)
        , m_systemId(systemId)
    {
#ifdef STARFISH_TC_COVERAGE
        if (name->equals("html")) {
            STARFISH_LOG_INFO("+++doctype:!DOCTYPE\n");
        }
#endif
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isDocumentType() const override;

    String* name() const
    {
        return m_name;
    }

    /* 4.4 Interface Node */

    virtual NodeType nodeType() const override
    {
        return DOCUMENT_TYPE_NODE;
    }

    virtual String* nodeName() override
    {
        return name();
    }

    virtual Node* clone() override
    {
        return new DocumentType(document(), m_name, m_publicId, m_systemId);
    }

    virtual Element* parentElement()
    {
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

    void remove()
    {
        // TODO
    }

    bool isContainerNode()
    {
        return false;
    }

protected:
    String* m_name;
    String* m_publicId;
    String* m_systemId;
};
}

#endif
