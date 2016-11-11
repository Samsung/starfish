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

#include "StarFishConfig.h"
#include "SelectorQuery.h"

#include "dom/HTMLElement.h"
#include "dom/Document.h"
#include "dom/Traverse.h"

namespace StarFish {

static inline SelectorQuery::SelectorCheckingContext prepareNextContextForRelation(const SelectorQuery::SelectorCheckingContext& context)
{
    SelectorQuery::SelectorCheckingContext nextContext(context);
    STARFISH_ASSERT(context.selector[1]);
    nextContext.selector.assign(context.selector.begin() + 1, context.selector.end());
    return nextContext;
}

Element* SelectorQuery::queryFirst(Node& rootNode)
{
    std::vector<Element*, gc_allocator<Element*>> matchedElement;
    execute(rootNode, &matchedElement, true);

    if (matchedElement.size() > 0)
        return matchedElement[0];
    else
        return nullptr;
}

bool SelectorQuery::match(const SelectorQuery::SelectorCheckingContext& context, SelectorQuery::MatchResult& result)
{
    STARFISH_ASSERT(context.selector.size() > 0);
    return matchSelector(context, result) == SelectorQuery::SelectorMatches;
}

bool SelectorQuery::match(const SelectorQuery::SelectorCheckingContext& context)
{
    SelectorQuery::MatchResult ignoreResult;
    return match(context, ignoreResult);
}

CSSSelector* SelectorQuery::selectorForIdLookup(std::vector<CSSSelector*, gc_allocator<CSSSelector*>>& selectors)
{
    int i = 0;
    for (CSSSelector* selector = selectors[i]; selector; selector = selectors[++i]) {
        if (selector->type() == CSSSelector::Id)
            return selector;
        if (selector->relation() != CSSSelector::SubSelector)
            break;
    }
    return nullptr;
}


void SelectorQuery::collectElementsById(Node& rootNode, AtomicString& id, std::vector<Node*, gc_allocator<Node*>>* collection)
{
    Traverse::getherDescendant(collection, &rootNode, [&](Node* child) {
        if (child->isElement() && child->asElement()->isHTMLElement() && child->asElement()->asHTMLElement()->id()->equals(id)) {
            return true;
        } else
            return false;
    });
}

bool SelectorQuery::selectorMatches(std::vector<CSSSelector*, gc_allocator<CSSSelector*> >& selector, Element& element, Node& rootNode)
{
    SelectorCheckingContext context(&element, VisitedMatchDisabled);
    context.selector = selector;
    context.scope = &rootNode;
    return match(context);
}

void SelectorQuery::execute(Node& rootNode, std::vector<Element*, gc_allocator<Element*>>* output, bool shouldOnlyMatchFirstElement)
{
    if (!m_selectors->size())
        return;

    std::vector<CSSSelector*, gc_allocator<CSSSelector*> > selectors = m_selectors->selectors();
    CSSSelector* firstSelector = selectors[0];

    // Fast path for querySelector*('#id'), querySelector*('tag#id').
    if (CSSSelector* idSelector = selectorForIdLookup(selectors)) {
        AtomicString idToMatch = AtomicString::createAtomicString(rootNode.document()->window()->starFish(), idSelector->selectorText());

        std::vector<Node*, gc_allocator<Node*>> elements;
        collectElementsById(rootNode, idToMatch, &elements);

        if (elements.size() > 0) {
            size_t count = elements.size();
            for (size_t i = 0; i < count; ++i) {
                Element* element = elements[i]->asElement();
                if (!(element->isDescendantOf(&rootNode)))
                    continue;
                if (selectorMatches(selectors, *element, rootNode)) {
                    output->push_back(element);

                    if (shouldOnlyMatchFirstElement)
                        return;
                }
            }
            return;
        }

        Element* element = rootNode.document()->getElementById(idToMatch);
        if (!element /* || !(isTreeScopeRoot(rootNode) */ || element->isDescendantOf(&rootNode))
            return;
        if (selectorMatches(selectors, *element, rootNode))
            output->push_back(element);
        return;
    }
}

bool SelectorQuery::checkPseudoClass(const SelectorCheckingContext& context, MatchResult& result)
{
    Element& element = *context.element;
    const std::vector<CSSSelector*, gc_allocator<CSSSelector*>>& selector = context.selector;

    switch (selector[0]->pseudoType()) {
    case CSSSelector::PseudoHover:
        /*if (m_mode == ResolvingStyle) {
            if (context.inRightmostCompound) {
                m_elementStyle->setAffectedByHover();
            } else {
                m_elementStyle->setUnique();
                element.setChildrenOrSiblingsAffectedByHover();
            }
        }

        if (!shouldMatchHoverOrActive(context))
            return false;
        if (InspectorInstrumentation::forcePseudoState(&element, CSSSelector::PseudoHover))
            return true;
        */
        return (element.state() >> 2) & 1;
    case CSSSelector::PseudoActive:
        /*if (m_mode == ResolvingStyle) {
            if (context.inRightmostCompound) {
                m_elementStyle->setAffectedByActive();
            } else {
                m_elementStyle->setUnique();
                element.setChildrenOrSiblingsAffectedByActive();
            }
        }

        if (!shouldMatchHoverOrActive(context))
            return false;
        if (InspectorInstrumentation::forcePseudoState(&element, CSSSelector::PseudoActive))
            return true;
        */
        return (element.state() >> 0) & 1;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    return false;
}

bool SelectorQuery::checkOne(const SelectorCheckingContext& context, MatchResult& result)
{
    STARFISH_ASSERT(context.element);
    Element& element = *context.element;
    STARFISH_ASSERT(context.selector.size() > 0);
    const std::vector<CSSSelector*, gc_allocator<CSSSelector*>>& selector = context.selector;

    switch (selector[0]->type()) {
    case CSSSelector::Tag:
        return element.tagName()->equals(selector[0]->selectorText()->toUpper());
    case CSSSelector::Class:
        return element.hasAttribute(element.document()->window()->starFish()->staticStrings()->m_class)
            && (std::find(element.classNames().begin(), element.classNames().end(), selector[0]->selectorText()) != element.classNames().end());
    case CSSSelector::Id:
        return element.hasAttribute(element.document()->window()->starFish()->staticStrings()->m_id)
            && element.id()->equals(selector[0]->selectorText());

    // Attribute selectors
    /*case CSSSelector::AttributeExact:
    case CSSSelector::AttributeSet:
    case CSSSelector::AttributeHyphen:
    case CSSSelector::AttributeList:
    case CSSSelector::AttributeContain:
    case CSSSelector::AttributeBegin:
    case CSSSelector::AttributeEnd:
        return anyAttributeMatches(element, selector.match(), selector);
    */
    case CSSSelector::PseudoClass:
        return checkPseudoClass(context, result);
    /*case CSSSelector::PseudoElement:
        return checkPseudoElement(context, result);

    case CSSSelector::PagePseudoClass:
        // FIXME: what?
        return true;
    */
    case CSSSelector::Universal:
    case CSSSelector::UnKnown:
        // FIXME: what?
        return true;
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return true;
}

SelectorQuery::Match SelectorQuery::matchForRelation(const SelectorCheckingContext& context, MatchResult& result)
{
    SelectorCheckingContext nextContext = prepareNextContextForRelation(context);

    CSSSelector::RelationType relation = context.selector[0]->relation();

    // Disable :visited matching when we see the first link or try to match anything else than an ancestors.
    if (!context.isSubSelector && (/*context.element->isLink() ||*/ (relation != CSSSelector::Descendant && relation != CSSSelector::Child)))
        nextContext.visitedMatchType = VisitedMatchDisabled;

    nextContext.inRightmostCompound = false;
    nextContext.isSubSelector = false;
    nextContext.previousElement = context.element;
    /* nextContext.pseudoId = PseudoIdNone; */

    switch (relation) {
    case CSSSelector::Descendant:
        /* if (context.selector->relationIsAffectedByPseudoContent()) {
            for (Element* element = context.element; element; element = element->parentElement()) {
                if (matchForPseudoContent(nextContext, *element, result) == SelectorMatches)
                    return SelectorMatches;
            }
            return SelectorFailsCompletely;
        }

        if (nextContext.selector->getPseudoType() == CSSSelector::PseudoShadow)
            return matchForPseudoShadow(nextContext, context.element->containingShadowRoot(), result);
        */
        for (nextContext.element = context.element->parentElement(); nextContext.element; nextContext.element = nextContext.element->parentElement()) {
            Match match = matchSelector(nextContext, result);
            if (match == SelectorMatches || match == SelectorFailsCompletely)
                return match;
            /*if (nextSelectorExceedsScope(nextContext))
                return SelectorFailsCompletely;
                */
        }
        return SelectorFailsCompletely;
    case CSSSelector::Child:
        /*{
            if (context.selector->relationIsAffectedByPseudoContent())
                return matchForPseudoContent(nextContext, *context.element, result);

            if (nextContext.selector->getPseudoType() == CSSSelector::PseudoShadow)
                return matchForPseudoShadow(nextContext, context.element->parentNode(), result);

            nextContext.element = parentElement(context);
            if (!nextContext.element)
                return SelectorFailsCompletely;
            return matchSelector(nextContext, result);
        }*/
    case CSSSelector::DirectAdjacent:
        /*// Shadow roots can't have sibling elements
        if (nextContext.selector->getPseudoType() == CSSSelector::PseudoShadow)
            return SelectorFailsCompletely;

        if (m_mode == ResolvingStyle) {
            if (ContainerNode* parent = context.element->parentElementOrShadowRoot())
                parent->setChildrenAffectedByDirectAdjacentRules();
        }
        nextContext.element = ElementTraversal::previousSibling(*context.element);
        if (!nextContext.element)
            return SelectorFailsAllSiblings;
        return matchSelector(nextContext, result);
         */
    case CSSSelector::IndirectAdjacent:
        /*// Shadow roots can't have sibling elements
        if (nextContext.selector->getPseudoType() == CSSSelector::PseudoShadow)
            return SelectorFailsCompletely;

        if (m_mode == ResolvingStyle) {
            if (ContainerNode* parent = context.element->parentElementOrShadowRoot())
                parent->setChildrenAffectedByIndirectAdjacentRules();
        }
        nextContext.element = ElementTraversal::previousSibling(*context.element);
        for (; nextContext.element; nextContext.element = ElementTraversal::previousSibling(*nextContext.element)) {
            Match match = matchSelector(nextContext, result);
            if (match == SelectorMatches || match == SelectorFailsAllSiblings || match == SelectorFailsCompletely)
                return match;
        }
        return SelectorFailsAllSiblings;
         */
    /*case CSSSelector::ShadowPseudo:
        {
            if (!m_isUARule && !m_isQuerySelector && context.selector->getPseudoType() == CSSSelector::PseudoShadow)
                Deprecation::countDeprecation(context.element->document(), UseCounter::CSSSelectorPseudoShadow);
            // If we're in the same tree-scope as the scoping element, then following a shadow descendant combinator would escape that and thus the scope.
            if (context.scope && context.scope->shadowHost() && context.scope->shadowHost()->treeScope() == context.element->treeScope())
                return SelectorFailsCompletely;

            Element* shadowHost = context.element->shadowHost();
            if (!shadowHost)
                return SelectorFailsCompletely;
            nextContext.element = shadowHost;
            return matchSelector(nextContext, result);
        }

    case CSSSelector::ShadowDeep:
        {
            if (!m_isUARule && !m_isQuerySelector)
                Deprecation::countDeprecation(context.element->document(), UseCounter::CSSDeepCombinator);
            if (ShadowRoot* root = context.element->containingShadowRoot()) {
                if (root->type() == ShadowRootType::UserAgent)
                    return SelectorFailsCompletely;
            }

            if (context.selector->relationIsAffectedByPseudoContent()) {
                // TODO(kochi): closed mode tree should be handled as well for ::content.
                for (Element* element = context.element; element; element = element->parentOrShadowHostElement()) {
                    if (matchForPseudoContent(nextContext, *element, result) == SelectorMatches)
                        return SelectorMatches;
                }
                return SelectorFailsCompletely;
            }

            for (nextContext.element = parentOrV0ShadowHostElement(*context.element); nextContext.element; nextContext.element = parentOrV0ShadowHostElement(*nextContext.element)) {
                Match match = matchSelector(nextContext, result);
                if (match == SelectorMatches || match == SelectorFailsCompletely)
                    return match;
                if (nextSelectorExceedsScope(nextContext))
                    return SelectorFailsCompletely;
            }
            return SelectorFailsCompletely;
        }

    case CSSSelector::ShadowSlot:
        {
            const HTMLSlotElement* slot = findSlotElementInScope(context);
            if (!slot)
                return SelectorFailsCompletely;

            nextContext.element = const_cast<HTMLSlotElement*>(slot);
            return matchSelector(nextContext, result);
        }
     */

    case CSSSelector::SubSelector:
    case CSSSelector::None:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    STARFISH_RELEASE_ASSERT_NOT_REACHED();
    return SelectorFailsCompletely;
}

// Recursive check of selectors and combinators
// It can return 4 different values:
// * SelectorMatches          - the selector matches the element e
// * SelectorFailsLocally     - the selector fails for the element e
// * SelectorFailsAllSiblings - the selector fails for e and any sibling of e
// * SelectorFailsCompletely  - the selector fails for e and any sibling or ancestor of e
SelectorQuery::Match SelectorQuery::matchSelector(const SelectorCheckingContext& context, MatchResult& result)
{
    MatchResult subResult;
    if (!checkOne(context, subResult))
        return SelectorFailsLocally;

    /* if (subResult.dynamicPseudo != PseudoIdNone)
        result.dynamicPseudo = subResult.dynamicPseudo;
    */
    if (context.selector[0]->isLastInTagHistory()) {
        result.specificity += subResult.specificity;
        return SelectorMatches;
    }

    Match match;
    if (context.selector[0]->relation() != CSSSelector::SubSelector) {
        /*if (nextSelectorExceedsScope(context))
            return SelectorFailsCompletely;

        if (context.pseudoId != PseudoIdNone && context.pseudoId != result.dynamicPseudo)
            return SelectorFailsCompletely;

        TemporaryChange<PseudoId> dynamicPseudoScope(result.dynamicPseudo, PseudoIdNone);
        */
        match = matchForRelation(context, result);
    } else {
        match = matchForSubSelector(context, result);
    }
    if (match == SelectorMatches)
        result.specificity += subResult.specificity;
    return match;
}

SelectorQuery::Match SelectorQuery::matchForSubSelector(const SelectorCheckingContext& context, MatchResult& result)
{
    SelectorCheckingContext nextContext = prepareNextContextForRelation(context);

    /* PseudoId dynamicPseudo = result.dynamicPseudo;
    nextContext.hasScrollbarPseudo = dynamicPseudo != PseudoIdNone && (m_scrollbar || dynamicPseudo == PseudoIdScrollbarCorner || dynamicPseudo == PseudoIdResizer);
    nextContext.hasSelectionPseudo = dynamicPseudo == PseudoIdSelection;
    */
    nextContext.isSubSelector = true;
    return matchSelector(nextContext, result);
}
}
