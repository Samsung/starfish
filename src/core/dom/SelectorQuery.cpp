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

#include "StarfishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/Node.h"
#include "core/dom/NodeList.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Traverse.h"

namespace Starfish {

static ALWAYS_INLINE bool contains(const GCVector<AtomicString>& vector,
                                   const AtomicString& string)
{
    size_t len = vector.size();
    for (size_t i = 0; i < len; i++) {
        if (vector[i] == string) {
            return true;
        }
    }
    return false;
}

enum ClassElementListBehavior { AllElements, OnlyRoots };
template <ClassElementListBehavior onlyRoots>
class ClassElementList : public gc {
public:
    ClassElementList(Node& rootNode, const AtomicString& className)
        : m_className(className)
        , m_rootNode(&rootNode)
        , m_currentElement(nullptr)
    {
        Node* child = m_rootNode->firstChild();
        while (child) {
            if (child->isElement()) {
                m_currentElement = child->asElement();
                break;
            }
            child = child->nextSibling();
        }
    }

    bool isEmpty() const
    {
        return !m_currentElement;
    }

    Element* next()
    {
        Element* current = m_currentElement;
        STARFISH_ASSERT(current);
        if (onlyRoots) {
            Node* n = nullptr;
            Node* prevN = m_currentElement;
            do {
                n = Traverse::nextSkippingChildren(prevN, m_rootNode);
                prevN = n;
            } while (n != nullptr && !n->isElement());

            if (n) {
                m_currentElement = nextInternal(n->asElement());
            } else {
                m_currentElement = nullptr;
            }
        } else {
            m_currentElement = nextInternal(
                (Element*)Traverse::nextElement(m_currentElement, m_rootNode));
        }
        return current;
    }

private:
    Element* nextInternal(Element* element)
    {
        for (; element;
             element = (Element*)Traverse::nextElement(element, m_rootNode)) {
            if (element->hasClass() &&
                contains(element->classNames(), m_className)) {
                return element;
            }
        }
        return nullptr;
    }

    AtomicString m_className;
    Node* m_rootNode;
    Element* m_currentElement;
};

Element* SelectorQuery::queryFirst(Node& rootNode)
{
    std::vector<Element*> matchedElement;
    execute(rootNode, matchedElement, true);

    STARFISH_ASSERT(matchedElement.size() <= 1);

    if (matchedElement.size() > 0) {
        return matchedElement[0];
    }
    return nullptr;
}

NodeList* SelectorQuery::queryAll(Node& rootNode)
{
    std::vector<Element*> matchedElement;
    execute(rootNode, matchedElement, false);

    NodeList* list = new NodeList(&rootNode, true);
    if (matchedElement.size() > 0) {
        GCVector<Element*> gcMatchedElement;
        size_t len = matchedElement.size();
        gcMatchedElement.reserve(len);
        for (size_t i = 0; i < len; i++) {
            gcMatchedElement.push_back(matchedElement[i]);
        }
        list->getNodeListImpl().setItems(gcMatchedElement);
    }
    return list;
}

bool SelectorQuery::matches(Element& element)
{
    return selectorListMatches(element, &element);
}

void SelectorQuery::invalidateStyleOfMatchedElement(Node& rootNode)
{
    m_inInvalidateStyleOfMatchedElement = true;
    std::vector<Element*> matchedElement;
    execute(rootNode, matchedElement, false);
    m_inInvalidateStyleOfMatchedElement = false;

    for (size_t i = 0; i < matchedElement.size(); i++) {
        matchedElement[i]->setNeedsStyleRecalc();
    }
}

inline bool ancestorHasClassName(Node& rootNode, const AtomicString& className)
{
    if (!rootNode.isElement()) {
        return false;
    }

    for (Element* element = rootNode.asElement(); element;
         element = element->parentElement()) {
        if (element->hasClass() && contains(element->classNames(), className)) {
            return true;
        }
    }
    return false;
}

CSSSelector* SelectorQuery::selectorForIdLookup(CSSSelectorList& selectors)
{
    int i = 0;
    for (auto it = selectors.begin(); it != selectors.end(); ++it) {
        if ((*it)->type() == CSSSelector::Id) {
            return *it;
        }
        if ((*it)->relation() != CSSSelector::SubSelector) {
            break;
        }
    }
    return nullptr;
}

void SelectorQuery::collectElementsById(Node& rootNode, const AtomicString& id,
                                        std::vector<Element*>& collection,
                                        bool shouldOnlyMatchFirstElement)
{
    Traverse::findMatchedDescendants(
        &rootNode,
        [](Element* element, void* data) -> bool {
            AtomicString* id = (AtomicString*)data;
            return element->hasId() && element->atomicId() == *id;
        },
        (void*)&id,
        [](Element* e, void* data) {
            std::vector<Element*>* collection = (std::vector<Element*>*)data;
            collection->push_back(e);
        },
        &collection, shouldOnlyMatchFirstElement);
}

void SelectorQuery::collectElementsByClassName(
    Node& rootNode, const AtomicString& className,
    std::vector<Element*>& collection, bool shouldOnlyMatchFirstElement)
{
    Traverse::findMatchedDescendants(
        &rootNode,
        [](Element* element, void* data) -> bool {
            AtomicString* className = (AtomicString*)data;
            return element->hasClass() &&
                   contains(element->classNames(), *className);
        },
        (void*)&className,
        [](Element* e, void* data) {
            std::vector<Element*>* collection = (std::vector<Element*>*)data;
            collection->push_back(e);
        },
        &collection, shouldOnlyMatchFirstElement);
}

void SelectorQuery::collectElementsByTagName(Node& rootNode,
                                             const AtomicString& tagName,
                                             std::vector<Element*>& collection,
                                             bool shouldOnlyMatchFirstElement)
{
    Traverse::findMatchedDescendants(
        &rootNode,
        [](Element* element, void* data) -> bool {
            AtomicString* tagName = (AtomicString*)data;
            return element->name().localNameAtomic() == *tagName;
        },
        (void*)&tagName,
        [](Element* e, void* data) {
            std::vector<Element*>* collection = (std::vector<Element*>*)data;
            collection->push_back(e);
        },
        &collection, shouldOnlyMatchFirstElement);
}

void SelectorQuery::traverseDescendants(CSSSelectorList& selectors,
                                        Node* traverseRoot, Node& rootNode,
                                        std::vector<Element*>& collection,
                                        bool shouldOnlyMatchFirstElement)
{
    struct Data {
        SelectorQuery* selectorQuery;
        CSSSelectorList& selectors;
        Node& rootNode;
        Data(SelectorQuery* q, CSSSelectorList& a, Node& b)
            : selectorQuery(q)
            , selectors(a)
            , rootNode(b)
        {
        }
    };

    Data d(this, selectors, rootNode);
    Traverse::findMatchedDescendants(
        &rootNode,
        [](Element* element, void* data) -> bool {
            Data* d = (Data*)data;
            return d->selectorQuery->selectorMatches(d->selectors, element,
                                                     d->rootNode);
        },
        &d,
        [](Element* e, void* data) {
            std::vector<Element*>* collection = (std::vector<Element*>*)data;
            collection->push_back(e);
        },
        &collection, shouldOnlyMatchFirstElement);
}

bool SelectorQuery::selectorMatches(CSSSelectorList& selector, Element* element,
                                    Node& rootNode)
{
    StyleResolver& resolver = element->document()->styleResolver();
    StyleResolver::MatchResult result;
    AtomicString elementName = element->name().localNameAtomic();
    AtomicString elementId = element->atomicId();
    const GCVector<AtomicString>& elementClasses = element->classNames();

    return resolver.matchSelector(element, elementName, elementId,
                                  elementClasses, selector, 0, result,
                                  !m_inInvalidateStyleOfMatchedElement) ==
           StyleResolver::Match::SelectorMatches;
}

void SelectorQuery::executeForTraverseRoot(
    CSSSelectorList& selectors, Node* traverseRoot,
    MatchTraverseRootState matchTraverseRoot, Node& rootNode,
    std::vector<Element*>& output, bool shouldOnlyMatchFirstElement)
{
    if (!traverseRoot) {
        return;
    }

    if (matchTraverseRoot) {
        if (!traverseRoot->isElement()) {
            return;
        }
        if (selectorMatches(selectors, traverseRoot->asElement(), rootNode)) {
            output.push_back(traverseRoot->asElement());
        }
        return;
    }

    traverseDescendants(selectors, traverseRoot, rootNode, output,
                        shouldOnlyMatchFirstElement);
}

template <typename SimpleElementListType>
void SelectorQuery::executeForTraverseRoots(
    CSSSelectorList& selectors, SimpleElementListType& traverseRoots,
    MatchTraverseRootState matchTraverseRoots, Node& rootNode,
    std::vector<Element*>& output, bool shouldOnlyMatchFirstElement)
{
    if (traverseRoots.isEmpty()) {
        return;
    }

    if (matchTraverseRoots) {
        while (!traverseRoots.isEmpty()) {
            Element* element = traverseRoots.next();
            if (selectorMatches(selectors, element, rootNode)) {
                output.push_back(element);
                if (shouldOnlyMatchFirstElement) {
                    return;
                }
            }
        }
        return;
    }

    while (!traverseRoots.isEmpty()) {
        traverseDescendants(selectors, traverseRoots.next(), rootNode, output,
                            shouldOnlyMatchFirstElement);
    }
}

void SelectorQuery::findTraverseRootsAndExecute(
    Node& rootNode, std::vector<Element*>& output,
    bool shouldOnlyMatchFirstElement)
{
    // We need to return the matches in document order. To use id lookup while
    // there is possibility of multiple matches
    // we would need to sort the results. For now, just traverse the document in
    // that case.
    STARFISH_ASSERT(m_selectorListContainer.size() == 1);

    bool isRightmostSelector = true;
    bool startFromParent = false;

    CSSSelectorList selectors = *m_selectorListContainer[0];
    for (auto it = selectors.begin(); it != selectors.end(); ++it) {
        std::vector<Element*> elements;
        collectElementsById(rootNode, (*it)->selectorText(), elements,
                            shouldOnlyMatchFirstElement);
        if ((*it)->type() == CSSSelector::Id && elements.size() == 1) {
            Element* element =
                rootNode.document()->getElementById((*it)->selectorText());

            Node* adjustedNode = &rootNode;
            if (element &&
                (rootNode.isDocument() || element->isDescendantOf(&rootNode))) {
                adjustedNode = element;
            } else if (!element || isRightmostSelector) {
                adjustedNode = nullptr;
            }
            if (isRightmostSelector) {
                executeForTraverseRoot(selectors, adjustedNode,
                                       MatchesTraverseRoots, rootNode, output,
                                       shouldOnlyMatchFirstElement);

                return;
            }

            if (startFromParent && adjustedNode) {
                adjustedNode = adjustedNode->parentNode();
            }

            executeForTraverseRoot(selectors, adjustedNode,
                                   DoesNotMatchTraverseRoots, rootNode, output,
                                   shouldOnlyMatchFirstElement);
            return;
        }

        // If we have both CSSSelector::Id and CSSSelector::Class at the same
        // time, we should use Id
        // to find traverse root.
        if (!shouldOnlyMatchFirstElement && !startFromParent &&
            (*it)->type() == CSSSelector::Class) {
            if (isRightmostSelector) {
                ClassElementList<AllElements> traverseRoots(
                    rootNode, (*it)->selectorText());
                executeForTraverseRoots(selectors, traverseRoots,
                                        MatchesTraverseRoots, rootNode, output,
                                        shouldOnlyMatchFirstElement);
                return;
            }
            // Since there exists some ancestor element which has the class
            // name, we need to see all children of rootNode.
            if (ancestorHasClassName(rootNode, (*it)->selectorText())) {
                executeForTraverseRoot(selectors, &rootNode,
                                       DoesNotMatchTraverseRoots, rootNode,
                                       output, shouldOnlyMatchFirstElement);
                return;
            }

            ClassElementList<OnlyRoots> traverseRoots(rootNode,
                                                      (*it)->selectorText());
            executeForTraverseRoots(selectors, traverseRoots,
                                    DoesNotMatchTraverseRoots, rootNode, output,
                                    shouldOnlyMatchFirstElement);
            return;
        }

        if ((*it)->relation() == CSSSelector::RelationType::SubSelector) {
            continue;
        }
        isRightmostSelector = false;
        if ((*it)->relation() == CSSSelector::RelationType::AdjacentSibling ||
            (*it)->relation() == CSSSelector::RelationType::GeneralSibling) {
            startFromParent = true;
        } else {
            startFromParent = false;
        }
    }

    executeForTraverseRoot(selectors, &rootNode, DoesNotMatchTraverseRoots,
                           rootNode, output, shouldOnlyMatchFirstElement);
}

inline bool SelectorQuery::canUseFastQuery(const Node& rootNode)
{
    /*if (m_needsUpdatedDistribution) {
        return false;
    }
    */
    if (!rootNode.isDocument()) {
        return false;
    }
    return m_selectorListContainer.size() == 1;
}

bool SelectorQuery::selectorListMatches(Node& rootNode, Element* element)
{
    size_t len = m_selectorListContainer.size();

    for (size_t i = 0; i < len; i++) {
        if (selectorMatches(*m_selectorListContainer[i], element, rootNode)) {
            return true;
        }
    }
    return false;
}

void SelectorQuery::executeSlow(Node& rootNode,
                                std::vector<Element*>& collection,
                                bool shouldOnlyMatchFirstElement)
{
    struct Data {
        SelectorQuery* selectorQuery;
        Node& rootNode;
        Data(SelectorQuery* q, Node& b)
            : selectorQuery(q)
            , rootNode(b)
        {
        }
    };
    Data d(this, rootNode);

    Traverse::findMatchedDescendants(
        &rootNode,
        [](Element* element, void* data) -> bool {
            Data* d = (Data*)data;
            if (d->selectorQuery->selectorListMatches(d->rootNode, element)) {
                return true;
            }
            return false;
        },
        &d,
        [](Element* e, void* data) {
            std::vector<Element*>* collection = (std::vector<Element*>*)data;
            collection->push_back(e);
        },
        &collection, shouldOnlyMatchFirstElement);
}

void SelectorQuery::execute(Node& rootNode, std::vector<Element*>& output,
                            bool shouldOnlyMatchFirstElement)
{
    if (!m_selectorListContainer.size()) {
        return;
    }

    if (!canUseFastQuery(rootNode)) {
        // TODO: We should investigate below code's purpose
        // if (m_needsUpdatedDistribution)
        //     rootNode.updateDistribution();
        executeSlow(rootNode, output, shouldOnlyMatchFirstElement);
        return;
    }

    STARFISH_ASSERT(m_selectorListContainer.size() == 1);

    CSSSelectorList selectors = *m_selectorListContainer[0];
    CSSSelector* firstSelector = selectors[0];

    // Fast path for querySelector*('#id'), querySelector*('tag#id').
    if (CSSSelector* idSelector = selectorForIdLookup(selectors)) {
        std::vector<Element*> elements;
        collectElementsById(rootNode, idSelector->selectorText(), elements,
                            shouldOnlyMatchFirstElement);
        if (elements.size() > 1) {
            size_t count = elements.size();
            for (size_t i = 0; i < count; ++i) {
                Element* element = elements[i];
                if (!(rootNode.isDocument() ||
                      element->isDescendantOf(&rootNode))) {
                    continue;
                }
                if (selectorMatches(selectors, element, rootNode)) {
                    output.push_back(element);

                    if (shouldOnlyMatchFirstElement) {
                        return;
                    }
                }
            }
            return;
        }

        Element* element =
            rootNode.document()->getElementById(idSelector->selectorText());
        if (!element ||
            !(rootNode.isDocument() || element->isDescendantOf(&rootNode))) {
            return;
        }
        if (selectorMatches(selectors, element, rootNode)) {
            output.push_back(element);
        }
        return;
    }

    // Fast path for querySelector*('.foo'), and querySelector*('div').
    if (firstSelector->isLastInTagHistory() &&
        (!firstSelector->isPseudoSelector() ||
         firstSelector->asCSSPseudoSelector()->pseudoType() ==
             CSSSelector::PseudoNone)) {
        switch (firstSelector->type()) {
        case CSSSelector::Class:
            collectElementsByClassName(rootNode, firstSelector->selectorText(),
                                       output, shouldOnlyMatchFirstElement);
            return;
        case CSSSelector::Tag:
            collectElementsByTagName(rootNode, firstSelector->selectorText(),
                                     output, shouldOnlyMatchFirstElement);
            return;
        default:
            break; // If we need another fast path, add here.
        }
    }

    findTraverseRootsAndExecute(rootNode, output, shouldOnlyMatchFirstElement);
}
}
