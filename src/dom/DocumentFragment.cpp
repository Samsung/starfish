#include "StarFishConfig.h"
#include "Document.h"
#include "DocumentFragment.h"

#include "Text.h"

namespace StarFish {

void DocumentFragment::setTextContent(String* text)
{
    Text* node = new Text(document(), text);

    while (firstChild()) {
        removeChild(firstChild());
    }
    if (!text->equals(String::emptyString)) {
        appendChild(node);
    }
}

String* DocumentFragment::nodeName()
{
    return document()->window()->starFish()->staticStrings()->m_documentFragmentLocalName;
}

Node* DocumentFragment::clone()
{
    DocumentFragment* newNode = new DocumentFragment(document());
    return newNode;
}

}
