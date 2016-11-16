/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishActiveNodeList__
#define __StarFishActiveNodeList__

namespace StarFish {

class Node;
class Element;
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

    ActiveNodeList(Node* root, bool canCache = true)
        : m_canCache(canCache)
        , m_isCacheValid(true)
        , m_root(root)
        , m_filter(nullptr)
        , m_data(nullptr)
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

    void setItems(std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& elements);
private:
    void fillCacheIfNeed() const;
    bool m_canCache;
    mutable bool m_isCacheValid;

    Node* m_root;
    ActiveNodeListFilterFunction m_filter;
    void* m_data;
    mutable std::vector<Node*, gc_allocator_ignore_off_page<Node*>> m_cachedNodeList;
};

}


#endif
