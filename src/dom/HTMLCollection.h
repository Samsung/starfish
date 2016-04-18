#ifndef __StarFishHTMLCollection__
#define __StarFishHTMLCollection__

#include "dom/binding/ScriptWrappable.h"
#include "dom/ActiveNodeList.h"

namespace StarFish {

class Node;
class Element;

class HTMLCollection : public ScriptWrappable {
public:
    HTMLCollection(ScriptBindingInstance* instance, Node* root, ActiveNodeListFilterFunction filter, void* data, bool canCache = false)
        : ScriptWrappable(this), m_activeNodeList(root, filter, data, canCache)
    {
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }
    unsigned long length() const;
    Element* item(unsigned long index);
    Element* namedItem(String* name);
    ActiveNodeList& activeNodeList()
    {
        return m_activeNodeList;
    }
private:
    ActiveNodeList m_activeNodeList;
};

}

#endif
