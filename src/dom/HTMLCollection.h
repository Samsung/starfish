#ifndef __StarFishHTMLCollection__
#define __StarFishHTMLCollection__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Node;
class Element;

typedef bool (*HTMLCollectionFilterFunction)(Node*, void*);

class HTMLCollection : public ScriptWrappable {
public:
    HTMLCollection(ScriptBindingInstance* instance, Node* root, HTMLCollectionFilterFunction filter, void* data)
        : ScriptWrappable(this), m_root(root), m_filter(filter), m_data(data)
    {
        initScriptWrappable(this, instance);
    }
    unsigned long length() const;
    Element* item(unsigned long index);
    Element* namedItem(String* name);
private:
    Node* m_root;
    HTMLCollectionFilterFunction m_filter;
    void* m_data;
};

}

#endif
