#ifndef __StarFishHTMLEntitySearch__
#define __StarFishHTMLEntitySearch__

#include "util/String.h"

namespace StarFish {

struct HTMLEntityTableEntry;

class HTMLEntitySearch {
public:
    HTMLEntitySearch();

    void advance(char32_t);

    bool isEntityPrefix() const { return !!m_first; }
    int currentLength() const { return m_currentLength; }

    const HTMLEntityTableEntry* mostRecentMatch() const { return m_mostRecentMatch; }

private:
    enum CompareResult {
        Before,
        Prefix,
        After,
    };

    CompareResult compare(const HTMLEntityTableEntry*, char32_t) const;
    const HTMLEntityTableEntry* findFirst(char32_t) const;
    const HTMLEntityTableEntry* findLast(char32_t) const;

    void fail()
    {
        m_first = 0;
        m_last = 0;
    }

    int m_currentLength;

    const HTMLEntityTableEntry* m_mostRecentMatch;
    const HTMLEntityTableEntry* m_first;
    const HTMLEntityTableEntry* m_last;
};

}

#endif
