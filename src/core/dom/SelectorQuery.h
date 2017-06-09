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

    SelectorQuery(GCVector<GCDeque<CSSSelector*>*>& selector)
        : m_selectorListContainer(selector)
    {
    }
    Element* queryFirst(Node& rootNode);
    NodeList* queryAll(Node& rootNode);

private:
    bool canUseFastQuery(const Node& rootNode);
    void traverseDescendants(GCDeque<CSSSelector*>& selectors,
                             Node* traverseRoot, Node& rootNode,
                             GCVector<Element*>& collection,
                             bool shouldOnlyMatchFirstElement);
    void executeForTraverseRoot(GCDeque<CSSSelector*>& selector,
                                Node* traverseRoot,
                                MatchTraverseRootState matchTraverseRoot,
                                Node& rootNode, GCVector<Element*>& output,
                                bool shouldOnlyMatchFirstElement);
    template <typename SimpleElementListType>
    void executeForTraverseRoots(GCDeque<CSSSelector*>& selector,
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
    bool selectorMatches(GCDeque<CSSSelector*>& selector, Element* element,
                         Node& rootNode);
    CSSSelector* selectorForIdLookup(GCDeque<CSSSelector*>& firstSelector);
    GCVector<GCDeque<CSSSelector*>*>& m_selectorListContainer;
};
}
#endif
