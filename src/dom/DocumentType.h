#ifndef __StarFishDocumentType__
#define __StarFishDocumentType__

#include "dom/Node.h"

namespace StarFish {

class DocumentType : public Node {
public:
    DocumentType(Document* document)
        : Node(document)
        // FIXME: should be given by the parser
        , m_name(String::createASCIIString("html"))
        , m_publicId(String::emptyString)
        , m_systemId(String::emptyString)
    {
    }

    DocumentType(Document* document, String* name, String* publicId, String* systemId)
        : Node(document)
        , m_name(name)
        , m_publicId(publicId)
        , m_systemId(systemId)
    {
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
        DocumentType* n = new DocumentType(document());
        n->m_name = m_name;
        n->m_publicId = m_publicId;
        n->m_systemId = m_systemId;
        return n;
    }

protected:
    String* m_name;
    String* m_publicId;
    String* m_systemId;
};
}

#endif
