/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishNodeListImpl__
#define __StarFishNodeListImpl__

namespace StarFish {

class Node;
class Element;

typedef bool (*NodeListFilterFunction)(Node*, void*, GCVector<Node*>*);
bool isChildNode(Node* node, void* data, GCVector<Node*>* collection);
bool isChildElement(Node* node, void* data, GCVector<Node*>* collection);
bool isSameTagName(Node* node, void* data, GCVector<Node*>* collection);
bool hasClassNames(Node* node, void* data, GCVector<Node*>* collection);
bool isSameNamedAccess(Node* node, void* data, GCVector<Node*>* collection);
bool isSameTableElement(Node* node, void* data, GCVector<Node*>* collection);
bool isFormElements(Node* node, void* data, GCVector<Node*>* collection);
bool isSelectedOption(Node* node, void* data, GCVector<Node*>* collection);

class NodeListImpl : public gc {
public:
    enum FilterFunctionType {
        None,
        ChildNodeFilter,
        ChildElementFilter,
        TagNameFilter,
        ClassNamesFilter,
        NamedAccessFilter,
        TableRowsFilter,
        FormElementsFiliter,
        SelectedOptionsFilter,
    };

    NodeListImpl(Node* root, FilterFunctionType filterType, void* data,
                 bool canCache = false)
        : m_canCache(canCache)
        , m_isCacheValid(false)
        , m_root(root)
        , m_filter(nullptr)
        , m_data(data)
    {
        switch (filterType) {
        case ChildNodeFilter:
            m_filter = isChildNode;
            break;
        case ChildElementFilter:
            m_filter = isChildElement;
            break;
        case TagNameFilter:
            m_filter = isSameTagName;
            break;
        case ClassNamesFilter:
            m_filter = hasClassNames;
            break;
        case NamedAccessFilter:
            m_filter = isSameNamedAccess;
            break;
        case TableRowsFilter:
            m_filter = isSameTableElement;
            break;
        case FormElementsFiliter:
            m_filter = isFormElements;
            break;
        case SelectedOptionsFilter:
            m_filter = isSelectedOption;
            break;
        case None:
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    NodeListImpl(Node* root, bool canCache = true)
        : m_canCache(canCache)
        , m_isCacheValid(true)
        , m_root(root)
        , m_filter(nullptr)
        , m_data(nullptr)
    {
    }

    NodeListImpl(Node* root, NodeListFilterFunction filter, void* data,
                 bool canCache)
        : m_canCache(canCache)
        , m_isCacheValid(true)
        , m_root(root)
        , m_filter(filter)
        , m_data(data)
    {
    }

    size_t length() const;
    Node* item(uint32_t index) const;
    void invalidateCache() const
    {
        STARFISH_ASSERT(m_canCache);
        m_isCacheValid = false;
        m_cachedNodeList.clear();
    }

    void setItems(GCVector<Element*>& elements);
    void getherDescendant(GCVector<Node*>* collection, Node* root) const;
    Node* root()
    {
        return m_root;
    }

private:
    void fillCacheIfNeed() const;
    bool m_canCache;
    mutable bool m_isCacheValid;

    Node* m_root;
    NodeListFilterFunction m_filter;
    void* m_data;
    mutable GCVector<Node*> m_cachedNodeList;
};
}

#endif
