/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
bool isTBodiesElement(Node* node, void* data, GCVector<Node*>* collection);
bool isTableCellsElement(Node* node, void* data, GCVector<Node*>* collection);
bool isFormElements(Node* node, void* data, GCVector<Node*>* collection);
bool isOptionElement(Node* node, void* data, GCVector<Node*>* collection);
bool isSelectedOption(Node* node, void* data, GCVector<Node*>* collection);
bool isMapAreasElement(Node* node, void* data, GCVector<Node*>* collection);
bool isAssociatedLabelElement(Node* node, void* data,
                              GCVector<Node*>* collection);

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
        TBodiesFilter,
        TableCellsFilter,
        FormElementsFiliter,
        OptionElementFilter,
        SelectedOptionsFilter,
        MapAreasElementFilter,
        AssociatedLabelElementFilter,
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
        case TBodiesFilter:
            m_filter = isTBodiesElement;
            break;
        case TableCellsFilter:
            m_filter = isTableCellsElement;
            break;
        case FormElementsFiliter:
            m_filter = isFormElements;
            break;
        case OptionElementFilter:
            m_filter = isOptionElement;
            break;
        case SelectedOptionsFilter:
            m_filter = isSelectedOption;
            break;
        case MapAreasElementFilter:
            m_filter = isMapAreasElement;
            break;
        case AssociatedLabelElementFilter:
            m_filter = isAssociatedLabelElement;
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
