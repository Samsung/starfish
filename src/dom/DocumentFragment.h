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
