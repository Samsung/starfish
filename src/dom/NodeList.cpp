#include "StarFishConfig.h"
#include "NodeList.h"
#include "dom/Document.h"
#include "dom/Traverse.h"

namespace StarFish {

template<typename Func>
static void getherDescendant(std::vector<Node*, gc_allocator<Node*>>* collection, Node* root, Func filter)
{
    Node* child = root->firstChild();
    while (child) {
        if (filter(child))
            collection->push_back(child);

        getherDescendant(collection, child, filter);
        child = child->nextSibling();
    }
}

unsigned long NodeList::length() const
{
    std::vector<Node*, gc_allocator<Node*>> collection;
    getherDescendant(&collection, m_root, [this](Node* child) -> bool {
        return m_filter(child, this->m_data);
    });
    return collection.size();
}

Node* NodeList::item(unsigned long index)
{
    std::vector<Node*, gc_allocator<Node*>> collection;
    getherDescendant(&collection, m_root, [this](Node* child) -> bool {
        return m_filter(child, this->m_data);
    });
    if (index < collection.size()) {
        return collection.at(index);
    }
    return nullptr;
}

}
