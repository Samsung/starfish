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
class CSSSelector;
class CSSSelectorList;

class SelectorQuery : public gc {
public:
    enum VisitedMatchType { VisitedMatchDisabled, VisitedMatchEnabled };

    SelectorQuery(CSSSelectorList* selector)
        : m_selectors(selector)
    { }
    Element* queryFirst(Node& rootNode);

    struct SelectorCheckingContext {
        // Initial selector constructor
        SelectorCheckingContext(Element* e, VisitedMatchType v)
        {
            /*selector(nullptr);*/
            element = e;
            previousElement = nullptr;
            scope = nullptr;
            visitedMatchType = v;
            /*, pseudoId(PseudoIdNone)*/
            isSubSelector = false;
            inRightmostCompound = true;
            hasScrollbarPseudo = false;
            hasSelectionPseudo = false;
            treatShadowHostAsNormalScope = false;
        }

        std::vector<CSSSelector*, gc_allocator<CSSSelector*>> selector;
        Element* element;
        Element* previousElement;
        Node* scope;
        VisitedMatchType visitedMatchType;
        /*PseudoId pseudoId;*/
        bool isSubSelector;
        bool inRightmostCompound;
        bool hasScrollbarPseudo;
        bool hasSelectionPseudo;
        bool treatShadowHostAsNormalScope;
    };

    struct MatchResult {
        MatchResult()
            : /*dynamicPseudo(PseudoIdNone)
            , */ specificity(0) { }

        /*PseudoId dynamicPseudo;*/
        unsigned specificity;
    };

    bool match(const SelectorCheckingContext& context, MatchResult& result);
    bool match(const SelectorCheckingContext& context);

private:
    enum Match { SelectorMatches, SelectorFailsLocally, SelectorFailsAllSiblings, SelectorFailsCompletely };

    bool checkPseudoClass(const SelectorCheckingContext& context, MatchResult& result);
    bool checkOne(const SelectorCheckingContext& context, MatchResult& result);
    Match matchForRelation(const SelectorCheckingContext& context, MatchResult& result);
    Match matchForSubSelector(const SelectorCheckingContext& context, MatchResult& result);
    Match matchSelector(const SelectorCheckingContext&, MatchResult&);
    void execute(Node& rootNode, std::vector<Element*, gc_allocator<Element*>>* matchedElement, bool shouldOnlyMatchFirstElement);
    void collectElementsById(Node& rootNode, AtomicString& id, std::vector<Node*, gc_allocator<Node*>>* collection);
    bool selectorMatches(std::vector<CSSSelector*, gc_allocator<CSSSelector*> >& selector, Element& element, Node& rootNode);
    CSSSelector* selectorForIdLookup(std::vector<CSSSelector*, gc_allocator<CSSSelector*> >& firstSelector);
    CSSSelectorList* m_selectors;
};

}
#endif
