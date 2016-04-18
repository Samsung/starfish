#ifndef __StarFishNodeList__
#define __StarFishNodeList__

#include "dom/binding/ScriptWrappable.h"
#include "dom/ActiveNodeList.h"

namespace StarFish {

class Node;

class NodeList : public ScriptWrappable {
public:
    NodeList(ScriptBindingInstance* instance, Node* root, ActiveNodeListFilterFunction filter, void* data, bool canCache = false)
        : ScriptWrappable(this)
        , m_activeNodeList(root, filter, data, canCache)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    unsigned long length() const;
    Node* item(unsigned long index);
    ActiveNodeList& activeNodeList()
    {
        return m_activeNodeList;
    }
private:
    ActiveNodeList m_activeNodeList;
};
}

#endif
