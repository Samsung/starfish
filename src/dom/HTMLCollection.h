#ifndef __StarFishHTMLCollection__
#define __StarFishHTMLCollection__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Node;
class Element;

class HTMLCollection : public ScriptWrappable {
public:
    HTMLCollection(ScriptBindingInstance* instance, Node* root, std::function<bool(Node*)> filter)
        : ScriptWrappable(this), m_root(root), m_filter(filter)
    {
        initScriptWrappable(this, instance);
    }
    unsigned long length() const;
    Element* item(unsigned long index);
    Element* namedItem(String* name);
private:
    Node* m_root;
    std::function<bool(Node*)> m_filter;
};

}

#endif
