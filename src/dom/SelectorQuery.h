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

#ifndef __StarFishSelectorQuery__
#define __StarFishSelectorQuery__

namespace StarFish {

class Element;
class Node;
class NodeList;
class CSSSelector;
class CSSSelectorList;

class SelectorQuery : public gc {
public:
    enum VisitedMatchType { VisitedMatchDisabled, VisitedMatchEnabled };
    enum MatchTraverseRootState { DoesNotMatchTraverseRoots, MatchesTraverseRoots };

    SelectorQuery(std::vector<CSSSelectorList*, gc_allocator_ignore_off_page<CSSSelectorList*>>& selector)
        : m_selectorListContainer(selector)
    { }
    Element* queryFirst(Node& rootNode);
    NodeList* queryAll(Node& rootNode);

    struct SelectorCheckingContext {
        // Initial selector constructor
        SelectorCheckingContext(Element* e, VisitedMatchType v)
            : element(e)
            , previousElement(nullptr)
            , scope(nullptr)
            , visitedMatchType(v)
            , isSubSelector(false)
            , inRightmostCompound(true)
            , hasScrollbarPseudo(false)
            , hasSelectionPseudo(false)
            , treatShadowHostAsNormalScope(false)

        {
        }

        std::deque<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*>> selector;
        Element* element;
        Element* previousElement;
        Node* scope;
        VisitedMatchType visitedMatchType;
        bool isSubSelector;
        bool inRightmostCompound;
        bool hasScrollbarPseudo;
        bool hasSelectionPseudo;
        bool treatShadowHostAsNormalScope;
    };

    struct MatchResult {
        MatchResult()
            : specificity(0) { }
        unsigned specificity;
    };

    bool match(const SelectorCheckingContext& context, MatchResult& result);
    bool match(const SelectorCheckingContext& context);

private:
    enum Match { SelectorMatches, SelectorFailsLocally, SelectorFailsAllSiblings, SelectorFailsCompletely };

    bool canUseFastQuery(const Node& rootNode);
    bool checkPseudoClass(const SelectorCheckingContext& context, MatchResult& result);
    bool checkOne(const SelectorCheckingContext& context, MatchResult& result);
    Match matchForRelation(const SelectorCheckingContext& context, MatchResult& result);
    Match matchForSubSelector(const SelectorCheckingContext& context, MatchResult& result);
    Match matchSelector(const SelectorCheckingContext&, MatchResult&);
    void traverseDescendants(std::deque<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selectors, Node* traverseRoot, Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement);
    void executeForTraverseRoot(std::deque<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selector, Node* traverseRoot, MatchTraverseRootState matchTraverseRoot, Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement);
    template <typename SimpleElementListType>
    void executeForTraverseRoots(std::deque<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selector, SimpleElementListType& traverseRoots, MatchTraverseRootState matchTraverseRoots, Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement);
    void findTraverseRootsAndExecute(Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement);
    bool selectorListMatches(Node& rootNode, Element* element, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output);
    void executeSlow(Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement);
    void execute(Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& matchedElement, bool shouldOnlyMatchFirstElement);
    void collectElementsById(Node& rootNode, String* id, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection);
    void collectElementsByClassName(Node& rootNode, const String* className,  std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement);
    void collectElementsByTagName(Node& rootNode, const String* tagName, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement);
    bool selectorMatches(std::deque<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selector, Element* element, Node& rootNode);
    CSSSelector* selectorForIdLookup(std::deque<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& firstSelector);
    std::vector<CSSSelectorList*, gc_allocator_ignore_off_page<CSSSelectorList*>>& m_selectorListContainer;
};

}
#endif
