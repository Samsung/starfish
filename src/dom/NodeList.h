#ifndef __StarFishNodeList__
#define __StarFishNodeList__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Node;

typedef bool (*NodeListFilterFunction)(Node*, void*);

class NodeList : public ScriptWrappable {
public:
    NodeList(ScriptBindingInstance* instance, Node* root, NodeListFilterFunction filter, void* data)
        : ScriptWrappable(this)
        , m_root(root)
        , m_filter(filter)
        , m_data(data)
    {
        initScriptWrappable(this, instance);
    }
    unsigned long length() const;
    Node* item(unsigned long index);

private:
    Node* m_root;
    NodeListFilterFunction m_filter;
    void* m_data;
};
}

#endif
