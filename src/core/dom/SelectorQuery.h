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

#ifndef __StarFishSelectorQuery__
#define __StarFishSelectorQuery__

namespace StarFish {

class Element;
class Node;
class NodeList;
class CSSSelector;

class SelectorQuery : public gc {
public:
    enum MatchTraverseRootState {
        DoesNotMatchTraverseRoots,
        MatchesTraverseRoots
    };

    SelectorQuery(GCVector<CSSSelectorList*>& selector)
        : m_selectorListContainer(selector)
    {
    }
    Element* queryFirst(Node& rootNode);
    NodeList* queryAll(Node& rootNode);
    bool matches(Element& element);

private:
    bool canUseFastQuery(const Node& rootNode);
    void traverseDescendants(CSSSelectorList& selectors, Node* traverseRoot,
                             Node& rootNode, GCVector<Element*>& collection,
                             bool shouldOnlyMatchFirstElement);
    void executeForTraverseRoot(CSSSelectorList& selector, Node* traverseRoot,
                                MatchTraverseRootState matchTraverseRoot,
                                Node& rootNode, GCVector<Element*>& output,
                                bool shouldOnlyMatchFirstElement);
    template <typename SimpleElementListType>
    void executeForTraverseRoots(CSSSelectorList& selector,
                                 SimpleElementListType& traverseRoots,
                                 MatchTraverseRootState matchTraverseRoots,
                                 Node& rootNode, GCVector<Element*>& output,
                                 bool shouldOnlyMatchFirstElement);
    void findTraverseRootsAndExecute(Node& rootNode, GCVector<Element*>& output,
                                     bool shouldOnlyMatchFirstElement);
    bool selectorListMatches(Node& rootNode, Element* element);
    void executeSlow(Node& rootNode, GCVector<Element*>& collection,
                     bool shouldOnlyMatchFirstElement);
    void execute(Node& rootNode, GCVector<Element*>& matchedElement,
                 bool shouldOnlyMatchFirstElement);
    void collectElementsById(Node& rootNode, const String* id,
                             GCVector<Element*>& collection,
                             bool shouldOnlyMatchFirstElement);
    void collectElementsByClassName(Node& rootNode, const String* className,
                                    GCVector<Element*>& collection,
                                    bool shouldOnlyMatchFirstElement);
    void collectElementsByTagName(Node& rootNode, const String* tagName,
                                  GCVector<Element*>& collection,
                                  bool shouldOnlyMatchFirstElement);
    bool selectorMatches(CSSSelectorList& selector, Element* element,
                         Node& rootNode);
    CSSSelector* selectorForIdLookup(CSSSelectorList& firstSelector);
    GCVector<CSSSelectorList*>& m_selectorListContainer;
};
}
#endif
