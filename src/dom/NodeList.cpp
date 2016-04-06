#include "StarFishConfig.h"
#include "NodeList.h"
#include "dom/Document.h"
#include "dom/Traverse.h"

namespace StarFish {

unsigned long NodeList::length() const
{
    return m_activeNodeList.length();
}

Node* NodeList::item(unsigned long index)
{
    return m_activeNodeList.item(index);
}

}
