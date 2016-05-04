#ifndef __StarFishHTMLEntityTable__
#define __StarFishHTMLEntityTable__

#include "util/String.h"

namespace StarFish {

struct HTMLEntityTableEntry {
    char lastCharacter() const { return entity[length - 1]; }

    const char* entity;
    int length;
    char32_t firstValue;
    char32_t secondValue;
};

class HTMLEntityTable {
public:
    static const HTMLEntityTableEntry* firstEntry();
    static const HTMLEntityTableEntry* lastEntry();

    static const HTMLEntityTableEntry* firstEntryStartingWith(char32_t);
    static const HTMLEntityTableEntry* lastEntryStartingWith(char32_t);
};

}

#endif
