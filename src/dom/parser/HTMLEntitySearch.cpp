#include "StarFishConfig.h"
#include "HTMLEntitySearch.h"
#include "HTMLEntityTable.h"

namespace StarFish {

static const HTMLEntityTableEntry* halfway(const HTMLEntityTableEntry* left, const HTMLEntityTableEntry* right)
{
    return &left[(right - left) / 2];
}

HTMLEntitySearch::HTMLEntitySearch()
    : m_currentLength(0)
    , m_mostRecentMatch(0)
    , m_first(HTMLEntityTable::firstEntry())
    , m_last(HTMLEntityTable::lastEntry())
{
}

HTMLEntitySearch::CompareResult HTMLEntitySearch::compare(const HTMLEntityTableEntry* entry, char32_t nextCharacter) const
{
    if (entry->length < m_currentLength + 1)
        return Before;
    char32_t entryNextCharacter = (char32_t) entry->entity[m_currentLength];
    if (entryNextCharacter == nextCharacter)
        return Prefix;
    return entryNextCharacter < nextCharacter ? Before : After;
}

const HTMLEntityTableEntry* HTMLEntitySearch::findFirst(char32_t nextCharacter) const
{
    const HTMLEntityTableEntry* left = m_first;
    const HTMLEntityTableEntry* right = m_last;
    if (left == right)
        return left;
    CompareResult result = compare(left, nextCharacter);
    if (result == Prefix)
        return left;
    if (result == After)
        return right;
    while (left + 1 < right) {
        const HTMLEntityTableEntry* probe = halfway(left, right);
        result = compare(probe, nextCharacter);
        if (result == Before)
            left = probe;
        else {
            STARFISH_ASSERT(result == After || result == Prefix);
            right = probe;
        }
    }
    STARFISH_ASSERT(left + 1 == right);
    return right;
}

const HTMLEntityTableEntry* HTMLEntitySearch::findLast(char32_t nextCharacter) const
{
    const HTMLEntityTableEntry* left = m_first;
    const HTMLEntityTableEntry* right = m_last;
    if (left == right)
        return right;
    CompareResult result = compare(right, nextCharacter);
    if (result == Prefix)
        return right;
    if (result == Before)
        return left;
    while (left + 1 < right) {
        const HTMLEntityTableEntry* probe = halfway(left, right);
        result = compare(probe, nextCharacter);
        if (result == After)
            right = probe;
        else {
            STARFISH_ASSERT(result == Before || result == Prefix);
            left = probe;
        }
    }
    STARFISH_ASSERT(left + 1 == right);
    return left;
}

void HTMLEntitySearch::advance(char32_t nextCharacter)
{
    STARFISH_ASSERT(isEntityPrefix());
    if (!m_currentLength) {
        m_first = HTMLEntityTable::firstEntryStartingWith(nextCharacter);
        m_last = HTMLEntityTable::lastEntryStartingWith(nextCharacter);
        if (!m_first || !m_last)
            return fail();
    } else {
        m_first = findFirst(nextCharacter);
        m_last = findLast(nextCharacter);
        if (m_first == m_last && compare(m_first, nextCharacter) != Prefix)
            return fail();
    }
    ++m_currentLength;
    if (m_first->length != m_currentLength) {
        return;
    }
    m_mostRecentMatch = m_first;
}

}
