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
#include "HTMLFormattingElementList.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

namespace StarFish {

// Biblically, Noah's Ark only had room for two of each animal, but in the
// Book of Hixie (aka http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#list-of-active-formatting-elements),
// Noah's Ark of Formatting Elements can fit three of each element.
static const size_t kNoahsArkCapacity = 3;

HTMLFormattingElementList::HTMLFormattingElementList()
{
}

HTMLFormattingElementList::~HTMLFormattingElementList()
{
}

Element* HTMLFormattingElementList::closestElementInScopeWithName(const AtomicString& targetName)
{
    for (unsigned i = 1; i <= m_entries.size(); ++i) {
        const Entry& entry = m_entries[m_entries.size() - i];
        if (entry.isMarker())
            return 0;
        if (entry.stackItem()->matchesHTMLTag(targetName))
            return entry.element();
    }
    return 0;
}

bool HTMLFormattingElementList::contains(Element* element)
{
    return !!find(element);
}

HTMLFormattingElementList::Entry* HTMLFormattingElementList::find(Element* element)
{
    auto iter = m_entries.rbegin();
    for (; iter != m_entries.rend(); iter++) {
        if (iter->element() == element) {
            return &(*iter);
        }
    }
    return 0;
}

HTMLFormattingElementList::Bookmark HTMLFormattingElementList::bookmarkFor(Element* element)
{
    auto iter = m_entries.rbegin();
    for (; iter != m_entries.rend(); iter++) {
        if (iter->element() == element) {
            return Bookmark(&(*iter));
        }
    }
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void HTMLFormattingElementList::swapTo(Element* oldElement, HTMLStackItem* newItem, const Bookmark& bookmark)
{
    STARFISH_ASSERT(contains(oldElement));
    STARFISH_ASSERT(!contains(newItem->element()));
    if (!bookmark.hasBeenMoved()) {
        STARFISH_ASSERT(bookmark.mark()->element() == oldElement);
        bookmark.mark()->replaceElement(newItem);
        return;
    }
    size_t index = bookmark.mark() - first();
    STARFISH_ASSERT(index < size());
    m_entries.insert(m_entries.begin() + (index + 1), Entry(newItem));
    remove(oldElement);
}

void HTMLFormattingElementList::append(HTMLStackItem* item)
{
    ensureNoahsArkCondition(item);
    m_entries.push_back(Entry(item));
}

void HTMLFormattingElementList::remove(Element* element)
{
    auto iter = m_entries.rbegin();
    for (; iter != m_entries.rend(); iter++) {
        if (iter->element() == element) {
            m_entries.erase(--(iter.base()));
            return;
        }
    }
}

void HTMLFormattingElementList::appendMarker()
{
    m_entries.push_back(Entry(Entry::MarkerEntry));
}

void HTMLFormattingElementList::clearToLastMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#clear-the-list-of-active-formatting-elements-up-to-the-last-marker
    while (m_entries.size()) {
        bool shouldStop = m_entries.back().isMarker();
        m_entries.erase(m_entries.end() - 1);
        if (shouldStop)
            break;
    }
}

void HTMLFormattingElementList::tryToEnsureNoahsArkConditionQuickly(HTMLStackItem* newItem, std::vector<HTMLStackItem*, gc_allocator<HTMLStackItem*>>& remainingCandidates)
{
    STARFISH_ASSERT(remainingCandidates.size() == 0);

    if (m_entries.size() < kNoahsArkCapacity)
        return;

    // Use a vector with inline capacity to avoid a malloc in the common case
    // of a quickly ensuring the condition.
    std::vector<HTMLStackItem*, gc_allocator<HTMLStackItem*>> candidates;

    size_t newItemAttributeCount = newItem->attributes().size();

    for (size_t i = m_entries.size(); i; ) {
        --i;
        Entry& entry = m_entries[i];
        if (entry.isMarker())
            break;

        // Quickly reject obviously non-matching candidates.
        HTMLStackItem* candidate = entry.stackItem();
        // if (newItem->localName() != candidate->localName() || newItem->namespaceURI() != candidate->namespaceURI())
        if (newItem->localName() != candidate->localName())
            continue;
        if (candidate->attributes().size() != newItemAttributeCount)
            continue;

        candidates.push_back(candidate);
    }

    if (candidates.size() < kNoahsArkCapacity)
        return; // There's room for the new element in the ark. There's no need to copy out the remainingCandidates.

    for (size_t i = 0; i < candidates.size(); i ++) {
        remainingCandidates.push_back(candidates[i]);
    }
}

void HTMLFormattingElementList::ensureNoahsArkCondition(HTMLStackItem* newItem)
{
    std::vector<HTMLStackItem*, gc_allocator<HTMLStackItem*>> candidates;
    tryToEnsureNoahsArkConditionQuickly(newItem, candidates);
    if (candidates.size() == 0)
        return;

    // We pre-allocate and re-use this second vector to save one malloc per
    // attribute that we verify.
    std::vector<HTMLStackItem*, gc_allocator<HTMLStackItem*>> remainingCandidates;
    // remainingCandidates.reserveInitialCapacity(candidates.size());

    const AttributeVector& attributes = newItem->attributes();
    for (size_t i = 0; i < attributes.size(); ++i) {
        const Attribute& attribute = attributes[i];

        for (size_t j = 0; j < candidates.size(); ++j) {
            HTMLStackItem* candidate = candidates[j];

            // These properties should already have been checked by tryToEnsureNoahsArkConditionQuickly.
            STARFISH_ASSERT(newItem->attributes().size() == candidate->attributes().size());
            // STARFISH_ASSERT(newItem->localName() == candidate->localName() && newItem->namespaceURI() == candidate->namespaceURI());
            STARFISH_ASSERT(newItem->localName() == candidate->localName());

            Attribute* candidateAttribute = candidate->getAttributeItem(attribute.name());
            if (candidateAttribute && candidateAttribute->value() == attribute.value())
                remainingCandidates.push_back(candidate);
        }

        if (remainingCandidates.size() < kNoahsArkCapacity)
            return;

        candidates.swap(remainingCandidates);
        // remainingCandidates.shrink(0);
        remainingCandidates.resize(0);
    }

    // Inductively, we shouldn't spin this loop very many times. It's possible,
    // however, that we wil spin the loop more than once because of how the
    // formatting element list gets permuted.
    for (size_t i = kNoahsArkCapacity - 1; i < candidates.size(); ++i)
        remove(candidates[i]->element());
}

#ifndef NDEBUG

void HTMLFormattingElementList::show()
{
    for (unsigned i = 1; i <= m_entries.size(); ++i) {
        const Entry& entry = m_entries[m_entries.size() - i];
        if (entry.isMarker())
            fprintf(stderr, "marker\n");
        else {
            fprintf(stderr, "not impl");
            // entry.element()->showNode();
        }
    }
}

#endif

}
