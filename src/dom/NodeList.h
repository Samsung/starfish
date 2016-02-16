#ifndef __StarFishNodeList__
#define __StarFishNodeList__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Node;
class NodeList : public ScriptWrappable {
public:
    NodeList(ScriptBindingInstance* instance, Node* root, std::function<bool (Node*)> filter)
        : ScriptWrappable(this), m_root(root), m_filter(filter) {
        initScriptWrappable(this, instance);
    }
    unsigned long length() const;
    Node* item(unsigned long index);
private:
    Node* m_root;
    std::function<bool (Node*)> m_filter;
};

}

#endif
