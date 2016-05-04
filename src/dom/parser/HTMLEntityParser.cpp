#include "StarFishConfig.h"
#include "HTMLEntityParser.h"
#include "HTMLEntitySearch.h"
#include "HTMLEntityTable.h"

namespace StarFish {

static const char32_t windowsLatin1ExtensionArray[32] = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, // 80-87
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F, // 88-8F
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, // 90-97
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178, // 98-9F
};

static bool isAlphaNumeric(char32_t cc)
{
    return (cc >= '0' && cc <= '9') || (cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z');
}
/*
static UChar adjustEntity(UChar32 value)
{
    if ((value & ~0x1F) != 0x0080)
        return value;
    return windowsLatin1ExtensionArray[value - 0x80];
}*/

static void appendLegalEntityFor(char32_t c, DecodedHTMLEntity& decodedEntity)
{
    /*
    // FIXME: A number of specific entity values generate parse errors.
    if (c <= 0 || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) {
        decodedEntity.append(0xFFFD);
        return;
    }
    if (U_IS_BMP(c)) {
        decodedEntity.append(adjustEntity(c));
        return;
    }*/
    decodedEntity.append(c);
}

static const char32_t kInvalidUnicode = -1;

static bool isHexDigit(char32_t cc)
{
    return (cc >= '0' && cc <= '9') || (cc >= 'a' && cc <= 'f') || (cc >= 'A' && cc <= 'F');
}

static char32_t asHexDigit(char32_t cc)
{
    if (cc >= '0' && cc <= '9')
        return cc - '0';
    if (cc >= 'a' && cc <= 'z')
        return 10 + cc - 'a';
    if (cc >= 'A' && cc <= 'Z')
        return 10 + cc - 'A';
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

typedef UTF32String ConsumedCharacterBuffer;

static void unconsumeCharacters(SegmentedString& source, ConsumedCharacterBuffer& consumedCharacters)
{
    if (consumedCharacters.size() == 1)
        source.push(consumedCharacters[0]);
    else if (consumedCharacters.size() == 2) {
        source.push(consumedCharacters[0]);
        source.push(consumedCharacters[1]);
    } else
        source.prepend(SegmentedString(String::createUTF32String(consumedCharacters.data())));
}

static bool consumeNamedEntity(SegmentedString& source, DecodedHTMLEntity& decodedEntity, bool& notEnoughCharacters, char32_t additionalAllowedCharacter, char32_t& cc)
{
    ConsumedCharacterBuffer consumedCharacters;
    HTMLEntitySearch entitySearch;
    while (!source.isEmpty()) {
        cc = source.currentChar();
        entitySearch.advance(cc);
        if (!entitySearch.isEntityPrefix())
            break;
        consumedCharacters.push_back(cc);
        source.advanceAndASSERT(cc);
    }
    notEnoughCharacters = source.isEmpty();
    if (notEnoughCharacters) {
        // We can't an entity because there might be a longer entity
        // that we could match if we had more data.
        unconsumeCharacters(source, consumedCharacters);
        return false;
    }
    if (!entitySearch.mostRecentMatch()) {
        unconsumeCharacters(source, consumedCharacters);
        return false;
    }
    if (entitySearch.mostRecentMatch()->length != entitySearch.currentLength()) {
        // We've consumed too many characters. We need to walk the
        // source back to the point at which we had consumed an
        // actual entity.
        unconsumeCharacters(source, consumedCharacters);
        consumedCharacters.clear();
        const int length = entitySearch.mostRecentMatch()->length;
        // const char* reference = entitySearch.mostRecentMatch()->entity;
        for (int i = 0; i < length; ++i) {
            cc = source.currentChar();
            // ASSERT_UNUSED(reference, cc == *reference++);
            consumedCharacters.push_back(cc);
            source.advanceAndASSERT(cc);
            STARFISH_ASSERT(!(source.length() == 0));
        }
        cc = source.currentChar();
    }
    if (entitySearch.mostRecentMatch()->lastCharacter() == ';'
        || !additionalAllowedCharacter
        || !(isAlphaNumeric(cc) || cc == '=')) {
        decodedEntity.append(entitySearch.mostRecentMatch()->firstValue);
        if (UChar32 second = entitySearch.mostRecentMatch()->secondValue)
            decodedEntity.append(second);
        return true;
    }
    unconsumeCharacters(source, consumedCharacters);
    return false;
}


bool consumeHTMLEntity(SegmentedString& source, DecodedHTMLEntity& decodedEntity, bool& notEnoughCharacters, char32_t additionalAllowedCharacter)
{
    STARFISH_ASSERT(!additionalAllowedCharacter || additionalAllowedCharacter == '"' || additionalAllowedCharacter == '\'' || additionalAllowedCharacter == '>');
    STARFISH_ASSERT(!notEnoughCharacters);
    STARFISH_ASSERT(decodedEntity.isEmpty());

    enum EntityState {
        Initial,
        Number,
        MaybeHexLowerCaseX,
        MaybeHexUpperCaseX,
        Hex,
        Decimal,
        Named
    };
    EntityState entityState = Initial;
    char32_t result = 0;
    ConsumedCharacterBuffer consumedCharacters;

    while (!source.isEmpty()) {
        char32_t cc = source.currentChar();
        switch (entityState) {
        case Initial: {
            if (cc == '\x09' || cc == '\x0A' || cc == '\x0C' || cc == ' ' || cc == '<' || cc == '&')
                return false;
            if (additionalAllowedCharacter && cc == additionalAllowedCharacter)
                return false;
            if (cc == '#') {
                entityState = Number;
                break;
            }
            if ((cc >= 'a' && cc <= 'z') || (cc >= 'A' && cc <= 'Z')) {
                entityState = Named;
                continue;
            }
            return false;
        }
        case Number: {
            if (cc == 'x') {
                entityState = MaybeHexLowerCaseX;
                break;
            }
            if (cc == 'X') {
                entityState = MaybeHexUpperCaseX;
                break;
            }
            if (cc >= '0' && cc <= '9') {
                entityState = Decimal;
                continue;
            }
            source.push('#');
            return false;
        }
        case MaybeHexLowerCaseX: {
            if (isHexDigit(cc)) {
                entityState = Hex;
                continue;
            }
            source.push('#');
            source.push('x');
            return false;
        }
        case MaybeHexUpperCaseX: {
            if (isHexDigit(cc)) {
                entityState = Hex;
                continue;
            }
            source.push('#');
            source.push('X');
            return false;
        }
        case Hex: {
            if (isHexDigit(cc)) {
                if (result != kInvalidUnicode)
                    result = result * 16 + asHexDigit(cc);
            } else if (cc == ';') {
                source.advanceAndASSERT(cc);
                appendLegalEntityFor(result, decodedEntity);
                return true;
            } else {
                appendLegalEntityFor(result, decodedEntity);
                return true;
            }
            break;
        }
        case Decimal: {
            if (cc >= '0' && cc <= '9') {
                if (result != kInvalidUnicode)
                    result = result * 10 + cc - '0';
            } else if (cc == ';') {
                source.advanceAndASSERT(cc);
                appendLegalEntityFor(result, decodedEntity);
                return true;
            } else {
                appendLegalEntityFor(result, decodedEntity);
                return true;
            }
            break;
        }
        case Named: {
            return consumeNamedEntity(source, decodedEntity, notEnoughCharacters, additionalAllowedCharacter, cc);
        }
        }

        if (result > UCHAR_MAX_VALUE)
            result = kInvalidUnicode;

        consumedCharacters.push_back(cc);
        source.advanceAndASSERT(cc);
    }
    STARFISH_ASSERT(source.length() == 0);
    notEnoughCharacters = true;
    unconsumeCharacters(source, consumedCharacters);
    return false;
}
/*
static size_t appendUChar32ToUCharArray(char32_t value, UChar* result)
{
    if (U_IS_BMP(value)) {
        char character = static_cast<char>(value);
        STARFISH_ASSERT(character == value);
        result[0] = character;
        return 1;
    }

    result[0] = U16_LEAD(value);
    result[1] = U16_TRAIL(value);
    return 2;
}
*/

size_t decodeNamedEntityToUCharArray(const char* name, char32_t result[4])
{
    HTMLEntitySearch search;
    while (*name) {
        search.advance(*name++);
        if (!search.isEntityPrefix())
            return 0;
    }
    search.advance(';');
    if (!search.isEntityPrefix())
        return 0;

    result[0] = search.mostRecentMatch()->firstValue;
    size_t numberOfCodePoints = 1;
    if (!search.mostRecentMatch()->secondValue)
        return numberOfCodePoints;
    result[1] = search.mostRecentMatch()->secondValue;
    return numberOfCodePoints + 1;
}

}
