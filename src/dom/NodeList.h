#ifndef __StarFishNodeList__
#define __StarFishNodeList__

#include "dom/binding/ScriptWrappable.h"
#include "dom/ActiveNodeList.h"

namespace StarFish {

class Node;

class NodeList : public ScriptWrappable {
public:
    NodeList(ScriptBindingInstance* instance, Node* root, ActiveNodeListFilterFunction filter, void* data)
        : ScriptWrappable(this)
        , m_activeNodeList(root, filter, data)
    {
        initScriptWrappable(this, instance);
    }
    unsigned long length() const;
    Node* item(unsigned long index);

private:
    ActiveNodeList m_activeNodeList;
};
}

#endif
