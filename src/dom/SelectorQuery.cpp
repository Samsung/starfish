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
#include "dom/NodeList.h"

namespace StarFish {

static inline SelectorQuery::SelectorCheckingContext prepareNextContextForRelation(const SelectorQuery::SelectorCheckingContext& context)
{
    SelectorQuery::SelectorCheckingContext nextContext(context);
    STARFISH_ASSERT(context.selector[1]);
    nextContext.selector.assign(context.selector.begin() + 1, context.selector.end());
    return nextContext;
}

static bool contains(const std::vector<String*, gc_allocator_ignore_off_page<String*> >& vector, const String* string)
{
    size_t size = vector.size();
    for (size_t i = 0; i < size; ++i) {
        if (string->equals(vector[i]))
            return true;
    }
    return false;
}

enum ClassElementListBehavior { AllElements, OnlyRoots };
template <ClassElementListBehavior onlyRoots>
class ClassElementList : public gc {
public:
    ClassElementList(Node& rootNode, String* className)
        : m_className(className)
        , m_rootNode(&rootNode)
        , m_currentElement(nextInternal((Element*)Traverse::firstChild(&rootNode, [&](Node* child) {
            if (child->isElement()) {
                return true;
            } else
                return false;
        })))
    { }

    bool isEmpty() const { return !m_currentElement; }

    Element* next()
    {
        Element* current = m_currentElement;
        STARFISH_ASSERT(current);
        if (onlyRoots)
            m_currentElement = nextInternal((Element*)Traverse::nextSkippingChildren(m_currentElement, m_rootNode));
        else
            m_currentElement = nextInternal((Element*)Traverse::nextElement(m_currentElement, m_rootNode));
        return current;
    }

private:
    Element* nextInternal(Element* element)
    {
        for (; element; element = (Element*)Traverse::nextElement(element, m_rootNode)) {
            if (element->isElement() && element->hasClass() && contains(element->classNames(), m_className))
                return element;
        }
        return nullptr;
    }

    const String* m_className;
    Node* m_rootNode;
    Element* m_currentElement;
};

Element* SelectorQuery::queryFirst(Node& rootNode)
{
    std::vector<Element*, gc_allocator_ignore_off_page<Element*>> matchedElement;
    execute(rootNode, matchedElement, true);

    if (matchedElement.size() > 0)
        return matchedElement[0];
    return nullptr;
}

NodeList* SelectorQuery::queryAll(Node& rootNode)
{
    std::vector<Element*, gc_allocator_ignore_off_page<Element*>> matchedElement;
    execute(rootNode, matchedElement, false);

    NodeList* list = new NodeList(rootNode.document()->scriptBindingInstance(), &rootNode, true);
    if (matchedElement.size() > 0)
        list->getNodeListImpl().setItems(matchedElement);
    return list;
}

inline bool ancestorHasClassName(Node& rootNode, const String* className)
{
    if (!rootNode.isElement())
        return false;

    for (Element* element = rootNode.asElement(); element; element = element->parentElement()) {
        if (element->hasClass() && contains(element->classNames(), className))
            return true;
    }
    return false;
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

CSSSelector* SelectorQuery::selectorForIdLookup(std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*>>& selectors)
{
    int i = 0;
    for (std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*>>::iterator it = selectors.begin(); it != selectors.end(); ++it) {
        if ((*it)->type() == CSSSelector::Id)
            return *it;
        if ((*it)->relation() != CSSSelector::SubSelector)
            break;
    }
    return nullptr;
}


void SelectorQuery::collectElementsById(Node& rootNode, String* id, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection)
{
    Traverse::getherDescendant(collection, &rootNode, [&](Node* child) {
        if (child->isElement() && child->asElement()->hasId() && child->asElement()->id()->equals(id)) {
            return true;
        } else
            return false;
    });
}

void SelectorQuery::collectElementsByClassName(Node& rootNode, const String* className,  std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, &rootNode, [&](Node* child) {
        if (child->isElement() && child->asElement()->hasClass() && contains(child->asElement()->classNames(), className)) {
            return true;
        } else
            return false;
    }, shouldOnlyMatchFirstElement);
}

void SelectorQuery::collectElementsByTagName(Node& rootNode, const String* tagName, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, &rootNode, [&](Node* child) {
        if (child->isElement() && (tagName->equals(String::fromUTF8("*")) || child->asElement()->tagName()->equals(tagName))) {
            return true;
        } else
            return false;
    }, shouldOnlyMatchFirstElement);
}

void SelectorQuery::traverseDescendants(std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selectors, Node* traverseRoot, Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, traverseRoot, [&](Node* child) {
        if (child->isElement() && selectorMatches(selectors, child->asElement(), rootNode))
            return true;
        return false;
    }, shouldOnlyMatchFirstElement);
}

bool SelectorQuery::selectorMatches(std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selector, Element* element, Node& rootNode)
{
    SelectorCheckingContext context(element, VisitedMatchDisabled);
    context.selector = selector;
    context.scope = &rootNode;
    return match(context);
}

void SelectorQuery::executeForTraverseRoot(std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selectors, Node* traverseRoot, MatchTraverseRootState matchTraverseRoot, Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement)
{
    if (!traverseRoot)
        return;

    if (matchTraverseRoot) {
        if (!traverseRoot->isElement())
            return;
        if (selectorMatches(selectors, traverseRoot->asElement(), rootNode))
            output.push_back(traverseRoot->asElement());
        return;
    }

    traverseDescendants(selectors, traverseRoot, rootNode, output, shouldOnlyMatchFirstElement);
}

template <typename SimpleElementListType>
void SelectorQuery::executeForTraverseRoots(std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> >& selectors, SimpleElementListType& traverseRoots, MatchTraverseRootState matchTraverseRoots, Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement)
{
    if (traverseRoots.isEmpty())
        return;

    if (matchTraverseRoots) {
        while (!traverseRoots.isEmpty()) {
            Element* element = traverseRoots.next();
            if (selectorMatches(selectors, element, rootNode)) {
                output.push_back(element);
                if (shouldOnlyMatchFirstElement)
                    return;
            }
        }
        return;
    }

    while (!traverseRoots.isEmpty()) {
        traverseDescendants(selectors, traverseRoots.next(), rootNode, output, shouldOnlyMatchFirstElement);
    }
}

void SelectorQuery::findTraverseRootsAndExecute(Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement)
{
    // We need to return the matches in document order. To use id lookup while there is possibility of multiple matches
    // we would need to sort the results. For now, just traverse the document in that case.
    STARFISH_ASSERT(m_selectorListContainer.size() == 1);

    bool isRightmostSelector = true;
    bool startFromParent = false;

    std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> > selectors = m_selectorListContainer[0]->selectors();
    for (std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*>>::iterator it = selectors.begin(); it != selectors.end(); ++it) {
        std::vector<Element*, gc_allocator_ignore_off_page<Element*>> elements;
        collectElementsById(rootNode, (*it)->selectorText(), elements);
        if ((*it)->type() == CSSSelector::Id && elements.size() == 1) {
            Element* element = rootNode.document()->getElementById((*it)->selectorText());

            Node* adjustedNode = &rootNode;
            if (element && (rootNode.isDocument() || element->isDescendantOf(&rootNode)))
                adjustedNode = element;
            else if (!element || isRightmostSelector)
                adjustedNode = nullptr;
            if (isRightmostSelector) {
                executeForTraverseRoot(selectors, adjustedNode, MatchesTraverseRoots, rootNode, output, shouldOnlyMatchFirstElement);

                return;
            }

            if (startFromParent && adjustedNode)
                adjustedNode = adjustedNode->parentNode();

            executeForTraverseRoot(selectors, adjustedNode, DoesNotMatchTraverseRoots, rootNode, output, shouldOnlyMatchFirstElement);
            return;
        }

        // If we have both CSSSelector::Id and CSSSelector::Class at the same time, we should use Id
        // to find traverse root.
        if (!shouldOnlyMatchFirstElement && !startFromParent && (*it)->type() == CSSSelector::Class) {
            if (isRightmostSelector) {
                ClassElementList<AllElements> traverseRoots(rootNode, (*it)->selectorText());
                executeForTraverseRoots(selectors, traverseRoots, MatchesTraverseRoots, rootNode, output, shouldOnlyMatchFirstElement);
                return;
            }
            // Since there exists some ancestor element which has the class name, we need to see all children of rootNode.
            if (ancestorHasClassName(rootNode, (*it)->selectorText())) {
                executeForTraverseRoot(selectors, &rootNode, DoesNotMatchTraverseRoots, rootNode, output, shouldOnlyMatchFirstElement);
                return;
            }

            ClassElementList<OnlyRoots> traverseRoots(rootNode, (*it)->selectorText());
            executeForTraverseRoots(selectors, traverseRoots, DoesNotMatchTraverseRoots, rootNode, output, shouldOnlyMatchFirstElement);
            return;
        }

        if ((*it)->relation() == CSSSelector::SubSelector)
            continue;
        isRightmostSelector = false;
        if ((*it)->relation() == CSSSelector::DirectAdjacent || (*it)->relation() == CSSSelector::IndirectAdjacent)
            startFromParent = true;
        else
            startFromParent = false;
    }

    executeForTraverseRoot(selectors, &rootNode, DoesNotMatchTraverseRoots, rootNode, output, shouldOnlyMatchFirstElement);
}

inline bool SelectorQuery::canUseFastQuery(const Node& rootNode)
{
    /*if (m_needsUpdatedDistribution)
        return false;
    */
    if (!rootNode.isDocument())
        return false;
    return m_selectorListContainer.size() == 1;
}

bool SelectorQuery::selectorListMatches(Node& rootNode, Element* element, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output)
{
    for (unsigned i = 0; i < m_selectorListContainer.size(); ++i) {
        if (selectorMatches(m_selectorListContainer[i]->selectors(), element, rootNode)) {
            return true;
        }
    }
    return false;
}

void SelectorQuery::executeSlow(Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& collection, bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, &rootNode, [&](Node* child) {
        if (child->isElement() && selectorListMatches(rootNode, child->asElement(), collection))
            return true;
        return false;
    }, shouldOnlyMatchFirstElement);
}

void SelectorQuery::execute(Node& rootNode, std::vector<Element*, gc_allocator_ignore_off_page<Element*>>& output, bool shouldOnlyMatchFirstElement)
{
    if (!m_selectorListContainer.size())
        return;

    if (!canUseFastQuery(rootNode)) {
        // TODO: We should investigate below code's purpose
        // if (m_needsUpdatedDistribution)
        //     rootNode.updateDistribution();
        executeSlow(rootNode, output, shouldOnlyMatchFirstElement);
        return;
    }

    STARFISH_ASSERT(m_selectorListContainer.size() == 1);

    std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*> > selectors = m_selectorListContainer[0]->selectors();
    CSSSelector* firstSelector = selectors[0];

    // Fast path for querySelector*('#id'), querySelector*('tag#id').
    if (CSSSelector* idSelector = selectorForIdLookup(selectors)) {
        std::vector<Element*, gc_allocator_ignore_off_page<Element*>> elements;
        collectElementsById(rootNode, idSelector->selectorText(), elements);
        if (elements.size() > 1) {
            size_t count = elements.size();
            for (size_t i = 0; i < count; ++i) {
                Element* element = elements[i];
                if (!(rootNode.isDocument() || element->isDescendantOf(&rootNode)))
                    continue;
                if (selectorMatches(selectors, element, rootNode)) {
                    output.push_back(element);

                    if (shouldOnlyMatchFirstElement)
                        return;
                }
            }
            return;
        }

        Element* element = rootNode.document()->getElementById(idSelector->selectorText());
        if (!element || !(rootNode.isDocument() || element->isDescendantOf(&rootNode)))
            return;
        if (selectorMatches(selectors, element, rootNode))
            output.push_back(element);
        return;
    }

    // Fast path for querySelector*('.foo'), and querySelector*('div').
    if (firstSelector->isLastInTagHistory() && firstSelector->pseudoType() == CSSSelector::PseudoNone) {
        switch (firstSelector->type()) {
        case CSSSelector::Class:
            collectElementsByClassName(rootNode, firstSelector->selectorText(), output, shouldOnlyMatchFirstElement);
            return;
        case CSSSelector::Tag:
            collectElementsByTagName(rootNode, firstSelector->selectorText()->toUpper(), output, shouldOnlyMatchFirstElement);
            return;
        default:
            break; // If we need another fast path, add here.
        }
    }

    findTraverseRootsAndExecute(rootNode, output, shouldOnlyMatchFirstElement);
}

bool SelectorQuery::checkPseudoClass(const SelectorCheckingContext& context, MatchResult& result)
{
    Element& element = *context.element;
    const std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*>>& selector = context.selector;

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
    const std::vector<CSSSelector*, gc_allocator_ignore_off_page<CSSSelector*>>& selector = context.selector;

    switch (selector[0]->type()) {
    case CSSSelector::Tag:
        if (element.tagName()->equals(selector[0]->selectorText()->toUpper())) {
            if (selector[0]->pseudoType() == CSSSelector::PseudoNone) {
                return true;
            } else {
                return checkPseudoClass(context, result);
            }
        }
        return false;
    case CSSSelector::Class:
        return element.hasClass() && contains(element.classNames(), selector[0]->selectorText());
    case CSSSelector::Id:
        return element.hasId() && element.id()->equals(selector[0]->selectorText());

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
    case CSSSelector::PseudoElement:
        return true;
    /*    return checkPseudoElement(context, result);

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

    switch (relation) {
    case CSSSelector::Descendant:
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
    case CSSSelector::IndirectAdjacent:
    case CSSSelector::SubSelector:
    case CSSSelector::None:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        break;
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

    if (context.selector[0]->isLastInTagHistory()) {
        result.specificity += subResult.specificity;
        return SelectorMatches;
    }

    Match match;
    if (context.selector[0]->relation() != CSSSelector::SubSelector) {
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
    nextContext.isSubSelector = true;
    return matchSelector(nextContext, result);
}
}
