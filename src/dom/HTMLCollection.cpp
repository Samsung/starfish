#include "StarFishConfig.h"
#include "HTMLCollection.h"
#include "dom/Document.h"
#include "dom/HTMLElement.h"
#include "dom/Traverse.h"

namespace StarFish {

unsigned long HTMLCollection::length() const
{
    return m_activeNodeList.length();
}

Element* HTMLCollection::item(unsigned long index)
{
    return m_activeNodeList.item(index)->asElement();
}

Element* HTMLCollection::namedItem(String* key)
{
    if (key->length()) {
        for (unsigned i = 0; i < m_activeNodeList.length(); i++) {
            Element* elem = m_activeNodeList.item(i)->asElement();
            if (elem->asElement()->asHTMLElement()->id()->equals(key))
                return elem;
        }
    }
    return nullptr;
}

}
