#ifndef __StarFishComment__
#define __StarFishComment__

#include "dom/CharacterData.h"

namespace StarFish {

class Comment : public CharacterData {
public:
    Comment(Document* document, String* data = String::emptyString)
        : CharacterData(document, data)
    {
        initScriptWrappable(this);
    }

    /* 4.4 Interface Node */
    virtual NodeType nodeType()
    {
        return Node::COMMENT_NODE;
    }

    virtual String* nodeName();
    virtual String* localName();
    virtual String* textContent()
    {
        return data();
    }

    virtual Node* clone()
    {
        return new Comment(document(), data());
    }

    /* 4.12 Interface Comment */
    // Comment node does not have any public APIs

    /* Other methods (not in DOM API) */

    virtual bool isComment() const
    {
        return true;
    }

protected:
};

}

#endif
