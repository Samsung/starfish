#ifndef __StarFishHTMLEntityParser__
#define __StarFishHTMLEntityParser__

#include "util/String.h"

namespace StarFish {

class DecodedHTMLEntity {
private:
    // HTML entities contain at most four UTF-16 code units.
    static const unsigned kMaxLength = 4;

public:
    DecodedHTMLEntity()
        : length(0)
    { }

    bool isEmpty() const { return !length; }

    void append(char32_t c)
    {
        STARFISH_RELEASE_ASSERT(length < kMaxLength);
        data[length++] = c;
    }

    unsigned length;
    char32_t data[kMaxLength];
};

bool consumeHTMLEntity(SegmentedString&, DecodedHTMLEntity& decodedEntity, bool& notEnoughCharacters, char32_t additionalAllowedCharacter = '\0');

}

#endif
