#ifndef __StarFishActiveNodeList__
#define __StarFishActiveNodeList__

namespace StarFish {

class Node;
typedef bool (*ActiveNodeListFilterFunction)(Node*, void*);

class ActiveNodeList : public gc {
public:
    ActiveNodeList(Node* root, ActiveNodeListFilterFunction filter, void* data)
        : m_root(root)
        , m_filter(filter)
        , m_data(data)
    {
    }
    unsigned long length() const;
    Node* item(unsigned long index);

private:
    Node* m_root;
    ActiveNodeListFilterFunction m_filter;
    void* m_data;
};

}


#endif
