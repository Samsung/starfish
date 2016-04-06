#ifndef __StarFishActiveNodeList__
#define __StarFishActiveNodeList__

namespace StarFish {

class Node;
typedef bool (*ActiveNodeListFilterFunction)(Node*, void*);

class ActiveNodeList : public gc {
public:
    ActiveNodeList(Node* root, ActiveNodeListFilterFunction filter, void* data, bool canCache = false)
        : m_canCache(canCache)
        , m_isCacheValid(false)
        , m_root(root)
        , m_filter(filter)
        , m_data(data)
    {
    }

    unsigned long length() const;
    Node* item(unsigned long index);
    void invalidateCache() const
    {
        STARFISH_ASSERT(m_canCache);
        m_isCacheValid = false;
        m_cachedNodeList.clear();
    }
private:
    void fillCacheIfNeed() const;
    bool m_canCache;
    mutable bool m_isCacheValid;

    Node* m_root;
    ActiveNodeListFilterFunction m_filter;
    void* m_data;
    mutable std::vector<Node*, gc_allocator<Node*>> m_cachedNodeList;
};

}


#endif
