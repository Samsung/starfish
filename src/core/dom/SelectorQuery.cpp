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

#include "StarFishConfig.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/Node.h"
#include "core/dom/NodeList.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Traverse.h"

namespace StarFish {

static bool contains(const GCVector<AtomicString>& vector, const String* string)
{
    return std::any_of(
        vector.begin(), vector.end(),
        [&string](AtomicString elm) { return string->equals(elm.string()); });
}

enum ClassElementListBehavior { AllElements, OnlyRoots };
template <ClassElementListBehavior onlyRoots>
class ClassElementList : public gc {
public:
    ClassElementList(Node& rootNode, String* className)
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
            m_currentElement =
                nextInternal((Element*)Traverse::nextSkippingChildren(
                    m_currentElement, m_rootNode));
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

    const String* m_className;
    Node* m_rootNode;
    Element* m_currentElement;
};

Element* SelectorQuery::queryFirst(Node& rootNode)
{
    GCVector<Element*> matchedElement;
    execute(rootNode, matchedElement, true);

    STARFISH_ASSERT(matchedElement.size() <= 1);

    if (matchedElement.size() > 0) {
        return matchedElement[0];
    }
    return nullptr;
}

NodeList* SelectorQuery::queryAll(Node& rootNode)
{
    GCVector<Element*> matchedElement;
    execute(rootNode, matchedElement, false);

    NodeList* list = new NodeList(&rootNode, true);
    if (matchedElement.size() > 0) {
        list->getNodeListImpl().setItems(matchedElement);
    }
    return list;
}

inline bool ancestorHasClassName(Node& rootNode, const String* className)
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

CSSSelector* SelectorQuery::selectorForIdLookup(
    GCDeque<CSSSelector*>& selectors)
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

void SelectorQuery::collectElementsById(Node& rootNode, const String* id,
                                        GCVector<Element*>& collection,
                                        bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, &rootNode,
                               [&](Element* element) {
                                   return element->hasId() &&
                                          element->id()->equals(id);
                               },
                               shouldOnlyMatchFirstElement);
}

void SelectorQuery::collectElementsByClassName(Node& rootNode,
                                               const String* className,
                                               GCVector<Element*>& collection,
                                               bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, &rootNode,
                               [&](Element* element) {
                                   return element->hasClass() &&
                                          contains(element->classNames(),
                                                   className);
                               },
                               shouldOnlyMatchFirstElement);
}

void SelectorQuery::collectElementsByTagName(Node& rootNode,
                                             const String* tagName,
                                             GCVector<Element*>& collection,
                                             bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, &rootNode,
                               [&](Element* element) {
                                   return tagName->equals(
                                              String::fromUTF8("*")) ||
                                          element->tagName()->equals(tagName);
                               },
                               shouldOnlyMatchFirstElement);
}

void SelectorQuery::traverseDescendants(GCDeque<CSSSelector*>& selectors,
                                        Node* traverseRoot, Node& rootNode,
                                        GCVector<Element*>& collection,
                                        bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(collection, traverseRoot,
                               [&](Element* element) {
                                   return selectorMatches(selectors, element,
                                                          rootNode);
                               },
                               shouldOnlyMatchFirstElement);
}

bool SelectorQuery::selectorMatches(GCDeque<CSSSelector*>& selector,
                                    Element* element, Node& rootNode)
{
    StyleResolver& resolver = element->document()->styleResolver();
    StyleResolver::MatchResult result;
    return resolver.matchSelector(element, &selector, 0, result, true) ==
           StyleResolver::Match::SelectorMatches;
}

void SelectorQuery::executeForTraverseRoot(
    GCDeque<CSSSelector*>& selectors, Node* traverseRoot,
    MatchTraverseRootState matchTraverseRoot, Node& rootNode,
    GCVector<Element*>& output, bool shouldOnlyMatchFirstElement)
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
    GCDeque<CSSSelector*>& selectors, SimpleElementListType& traverseRoots,
    MatchTraverseRootState matchTraverseRoots, Node& rootNode,
    GCVector<Element*>& output, bool shouldOnlyMatchFirstElement)
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
    Node& rootNode, GCVector<Element*>& output,
    bool shouldOnlyMatchFirstElement)
{
    // We need to return the matches in document order. To use id lookup while
    // there is possibility of multiple matches
    // we would need to sort the results. For now, just traverse the document in
    // that case.
    STARFISH_ASSERT(m_selectorListContainer.size() == 1);

    bool isRightmostSelector = true;
    bool startFromParent = false;

    GCDeque<CSSSelector*> selectors = *m_selectorListContainer[0];
    for (auto it = selectors.begin(); it != selectors.end(); ++it) {
        GCVector<Element*> elements;
        collectElementsById(rootNode, (*it)->selectorText().string(), elements,
                            shouldOnlyMatchFirstElement);
        if ((*it)->type() == CSSSelector::Id && elements.size() == 1) {
            Element* element = rootNode.document()->getElementById(
                (*it)->selectorText().string());

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
                    rootNode, (*it)->selectorText().string());
                executeForTraverseRoots(selectors, traverseRoots,
                                        MatchesTraverseRoots, rootNode, output,
                                        shouldOnlyMatchFirstElement);
                return;
            }
            // Since there exists some ancestor element which has the class
            // name, we need to see all children of rootNode.
            if (ancestorHasClassName(rootNode,
                                     (*it)->selectorText().string())) {
                executeForTraverseRoot(selectors, &rootNode,
                                       DoesNotMatchTraverseRoots, rootNode,
                                       output, shouldOnlyMatchFirstElement);
                return;
            }

            ClassElementList<OnlyRoots> traverseRoots(
                rootNode, (*it)->selectorText().string());
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
    return std::any_of(
        m_selectorListContainer.begin(), m_selectorListContainer.end(),
        [this, &element, &rootNode](GCDeque<CSSSelector*>* selectors) {
            return selectorMatches(*selectors, element, rootNode);
        });
}

void SelectorQuery::executeSlow(Node& rootNode, GCVector<Element*>& collection,
                                bool shouldOnlyMatchFirstElement)
{
    Traverse::getherDescendant(
        collection, &rootNode,
        [&](Node* child) {
            if (child->isElement() &&
                selectorListMatches(rootNode, child->asElement())) {
                return true;
            }
            return false;
        },
        shouldOnlyMatchFirstElement);
}

void SelectorQuery::execute(Node& rootNode, GCVector<Element*>& output,
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

    GCDeque<CSSSelector*> selectors = *m_selectorListContainer[0];
    CSSSelector* firstSelector = selectors[0];

    // Fast path for querySelector*('#id'), querySelector*('tag#id').
    if (CSSSelector* idSelector = selectorForIdLookup(selectors)) {
        GCVector<Element*> elements;
        collectElementsById(rootNode, idSelector->selectorText().string(),
                            elements, shouldOnlyMatchFirstElement);
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

        Element* element = rootNode.document()->getElementById(
            idSelector->selectorText().string());
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
        firstSelector->pseudoType() == CSSSelector::PseudoNone) {
        switch (firstSelector->type()) {
        case CSSSelector::Class:
            collectElementsByClassName(rootNode,
                                       firstSelector->selectorText().string(),
                                       output, shouldOnlyMatchFirstElement);
            return;
        case CSSSelector::Tag:
            collectElementsByTagName(
                rootNode, firstSelector->selectorText().string()->toUpper(),
                output, shouldOnlyMatchFirstElement);
            return;
        default:
            break; // If we need another fast path, add here.
        }
    }

    findTraverseRootsAndExecute(rootNode, output, shouldOnlyMatchFirstElement);
}
}
