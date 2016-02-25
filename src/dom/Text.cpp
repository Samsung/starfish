#include "StarFishConfig.h"
#include "Text.h"
#include "Traverse.h"

#include "Document.h"

namespace StarFish {

String* Text::nodeName()
{
    return document()->window()->starFish()->staticStrings()->m_textLocalName;
}

String* Text::localName()
{
    return document()->window()->starFish()->staticStrings()->m_textLocalName;
}

// Text* Text::splitText(unsigned long offset)
// {
//    return nullptr;
// }

String* Text::wholeText()
{
    auto filter = [&](Node* node)
    {
        return node->isText();
    };
    Traverse::NodeCollection* pSiblings = Traverse::previousSiblings(this, filter);
    Traverse::NodeCollection* nSiblings = Traverse::nextSiblings(this, filter);

    String* str = String::createASCIIString("");
    std::for_each(pSiblings->begin(), pSiblings->end(), [&](Node* n) {
        str = str->concat(n->asText()->data());
    });

    str = str->concat(data());

    std::for_each(nSiblings->begin(), nSiblings->end(), [&](Node* n) {
        str = str->concat(n->asText()->data());
    });

    return str;
}
}
