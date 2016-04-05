#include "StarFishConfig.h"
#include "HTMLCollection.h"
#include "dom/Document.h"
#include "dom/Element.h"
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

unsigned long HTMLCollection::length() const
{
    std::vector<Node*, gc_allocator<Node*>> collection;
    getherDescendant(&collection, m_root, [this](Node* child) -> bool {
        return m_filter(child, this->m_data);
    });
    return collection.size();
}

Element* HTMLCollection::item(unsigned long index)
{
    std::vector<Node*, gc_allocator<Node*>> collection;
    getherDescendant(&collection, m_root, [this](Node* child) -> bool {
        return m_filter(child, this->m_data);
    });
    if (index < collection.size()) {
        return collection.at(index)->asElement();
    }
    return nullptr;
}

Element* HTMLCollection::namedItem(String* key)
{
    if (key->length() != 0) {
        std::vector<Node*, gc_allocator<Node*>> collection;
        getherDescendant(&collection, m_root, [this](Node* child) -> bool {
            return m_filter(child, this->m_data);
        });
        for (unsigned i = 0; i < collection.size(); i++) {
            Element* elem = collection.at(i)->asElement();
            fprintf(stderr, "id: %s\n\n", elem->id()->utf8Data());
            if (elem->id()->equals(key))
                return elem;
        }
    }
    return nullptr;
}

}
