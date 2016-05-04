#ifndef __StarFishString__
#define __StarFishString__

namespace StarFish {

typedef std::basic_string<char, std::char_traits<char>, gc_allocator<char> > ASCIIString;
typedef std::basic_string<char16_t, std::char_traits<char16_t>, gc_allocator<char16_t> > UTF16String;
typedef std::basic_string<char32_t, std::char_traits<char32_t>, gc_allocator<char32_t> > UTF32String;

class StringDataASCII;
class String;

template<typename CharType> inline bool isASCIIUpper(CharType c)
{
    return c >= 'A' && c <= 'Z';
}

template<typename CharType> inline bool isASCIILower(CharType c)
{
    return c >= 'a' && c <= 'z';
}

template<typename CharType> inline CharType toASCIILower(CharType c)
{
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER == 170060610
    // Make a workaround for VS2012 update 3 optimizer bug, remove once VS2012 fix it.
    return (c >= 'A' && c <= 'Z') ? c + 0x20 : c;
#else
    return c | ((c >= 'A' && c <= 'Z') << 5);
#endif
}

class String {
public:
    static const unsigned defaultLengthLimit = 1 << 16;
    typedef std::vector<String*, gc_allocator<String*> > Vector;

    static String* emptyString;
    static String* spaceString;

    static String* fromUTF8(const char* src);
    static String* fromUTF8(const char* src, size_t len);
    static String* createASCIIString(const char* src);
    static String* createUTF32String(const UTF32String& src);
    static String* createUTF32String(char32_t c);
    static String* createASCIIStringFromUTF32Source(const UTF32String& src);
    static String* createASCIIStringFromUTF32SourceIfPossible(const UTF32String& src);

    ASCIIString* asASCIIString() const
    {
        STARFISH_ASSERT(m_isASCIIString);
        return (ASCIIString*)((size_t)this + sizeof(size_t));
    }

    UTF32String* asUTF32String() const
    {
        STARFISH_ASSERT(!m_isASCIIString);
        return (UTF32String*)((size_t)this + sizeof(size_t));
    }

    UTF16String toUTF16String() const;

    const char* utf8Data();

    bool equals(const String* str) const;
    bool equalsWithoutCase(const String* str) const;
    bool equals(const char* str)
    {
#ifndef NDEBUG
        {
            const char* c = str;
            while (*c) {
                STARFISH_ASSERT(!(*c & 0x80));
                c++;
            }
        }
#endif
        size_t srcLen = strlen(str);
        if (srcLen != length())
            return false;
        for (size_t i = 0; i < length() ; i ++) {
            if (charAt(i) != (char32_t)str[i]) {
                return false;
            }
        }
        return true;
    }

    bool equals(const char32_t* str)
    {
        size_t srcLen = 0;
        for (; str[srcLen] ; srcLen ++) { }

        if (srcLen != length())
            return false;
        for (size_t i = 0; i < length() ; i ++) {
            if (charAt(i) != str[i]) {
                return false;
            }
        }
        return true;
    }

    size_t length() const
    {
        if (m_isASCIIString) {
            return asASCIIString()->length();
        } else {
            return asUTF32String()->length();
        }
    }

    char32_t charAt(size_t idx) const
    {
        if (m_isASCIIString) {
            return (*asASCIIString())[idx];
        } else {
            return (*asUTF32String())[idx];
        }
    }

    char32_t operator[](size_t idx) const
    {
        return charAt(idx);
    }

    size_t indexOf(char32_t ch) const
    {
        for (size_t i = 0; i < length(); i ++) {
            if (charAt(i) == ch) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    size_t lastIndexOf(char32_t ch) const
    {
        for (size_t i = length(); i > 0; i --) {
            if (charAt(i - 1) == ch) {
                return i - 1;
            }
        }
        return SIZE_MAX;
    }

    static inline bool isASCIISpace(char32_t c) { return c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9)); }
    static inline bool isSpaceOrNewline(char32_t c)
    {
        // Use isASCIISpace() for basic Latin-1.
        // This will include newlines, which aren't included in Unicode DirWS.
        return c <= 0x7F ? isASCIISpace(c) : false; /* : u_charDirection(c) == U_WHITE_SPACE_NEUTRAL;*/
    }

    bool containsOnlyWhitespace()
    {
        for (size_t i = 0; i < length(); i ++) {
            if (!isASCIISpace(charAt(i))) {
                return false;
            }
        }
        return true;
    }

    static String* fromFloat(float f);
    static String* fromInt(int i);
    static int parseInt(String* s)
    {
        return atoi(s->utf8Data());
    }

    String* substring(size_t pos, size_t len);

    String* toUpper();
    String* toLower();
    String* replaceAll(String* from, String* to);
    bool isASCIIString() const
    {
        return m_isASCIIString;
    }
    String* concat(String* str);
    void split(char delim, Vector& tokens);
    String* trim();

    // token is only 1-byte char now.
    std::vector<String*, gc_allocator<String*>> tokenize(const char* tokens, size_t tokensLength);

    icu::UnicodeString toUnicodeString() const
    {
        if (isASCIIString()) {
            return icu::UnicodeString(asASCIIString()->data(), asASCIIString()->length(), US_INV);
        } else {
            return icu::UnicodeString::fromUTF32((const UChar32*)asUTF32String()->data(), asUTF32String()->length());
        }
    }

    icu::UnicodeString toUnicodeString(size_t start, size_t end) const
    {
        size_t len = end - start;
        if (isASCIIString()) {
            return icu::UnicodeString(asASCIIString()->data() + start, len, US_INV);
        } else {
            return icu::UnicodeString::fromUTF32((const UChar32*)asUTF32String()->data() + start, len);
        }
    }

    UTF32String toUTF32String();

    bool startsWith(const char* str, bool caseSensitive = true);
    bool startsWith(String* str, bool caseSensitive = true);

protected:
    template <typename T>
    static bool stringEqual(const T* s, const T* s1, const size_t& len)
    {
        return memcmp(s, s1, sizeof(T) * len) == 0;
    }

    static bool stringEqual(const char32_t* s, const char* s1, const size_t& len)
    {
        for (size_t i = 0; i < len ; i ++) {
            if (s[i] != (unsigned char)s1[i]) {
                return false;
            }
        }
        return true;
    }

    const char* utf8DataSlowCase();
    String()
    {
        m_isASCIIString = true;
    }

    size_t m_isASCIIString;
};

class StringDataASCII : public String, public ASCIIString, public gc {
public:
    StringDataASCII(ASCIIString&& str)
        : ASCIIString(str)
    {
    }

    StringDataASCII(const char* str)
        : ASCIIString(str)
    {
    }

    StringDataASCII(const char* str, size_t len)
        : ASCIIString(str, &str[len])
    {
    }
};

class StringDataUTF32 : public String, public UTF32String, public gc {
public:
    StringDataUTF32(const UTF32String& str)
        : UTF32String(str)
    {
        m_isASCIIString = false;
    }
    StringDataUTF32(UTF32String&& str)
        : UTF32String(str)
    {
        m_isASCIIString = false;
    }
    StringDataUTF32(const char* src, size_t len);
    StringDataUTF32(const char32_t* str)
        : UTF32String(str)
    {
        m_isASCIIString = false;
    }
};


class SegmentedString;

class SegmentedSubstring {
public:
    SegmentedSubstring()
        : m_length(0)
        , m_doNotExcludeLineNumbers(true)
        , m_is8Bit(false)
        , m_string(String::emptyString)
    {
        m_data.string32Ptr = 0;
    }

    SegmentedSubstring(String* str)
        : m_length(str->length())
        , m_doNotExcludeLineNumbers(true)
        , m_string(str)
    {
        if (m_length) {
            if (m_string->isASCIIString()) {
                m_is8Bit = true;
                m_data.string8Ptr = m_string->asASCIIString()->data();
            } else {
                m_is8Bit = false;
                m_data.string32Ptr = m_string->asUTF32String()->data();
            }
        } else {
            m_is8Bit = false;
        }
    }

    void clear() { m_length = 0; m_data.string32Ptr = 0; m_is8Bit = false;}

    bool is8Bit() { return m_is8Bit; }

    bool excludeLineNumbers() const { return !m_doNotExcludeLineNumbers; }
    bool doNotExcludeLineNumbers() const { return m_doNotExcludeLineNumbers; }

    void setExcludeLineNumbers() { m_doNotExcludeLineNumbers = false; }

    int numberOfCharactersConsumed() const { return m_string->length() - m_length; }

    void appendTo(UTF32String& builder) const
    {
        int offset = (int)m_string->length() - m_length;

        if (!offset) {
            if (m_length) {
                for (size_t i = 0; i < m_string->length(); i ++) {
                    builder.push_back(m_string->charAt(i));
                }
            }
        } else {
            for (int i = offset; i < (offset + m_length); i ++) {
                builder.push_back(m_string->charAt(i));
            }
        }
    }

    char32_t getCurrentChar8()
    {
        return *m_data.string8Ptr;
    }

    char32_t getCurrentChar32()
    {
        return m_data.string32Ptr ? *m_data.string32Ptr : 0;
    }

    char32_t incrementAndGetCurrentChar8()
    {
        STARFISH_ASSERT(m_data.string8Ptr);
        return *++m_data.string8Ptr;
    }

    char32_t incrementAndGetCurrentChar32()
    {
        STARFISH_ASSERT(m_data.string32Ptr);
        return *++m_data.string32Ptr;
    }

    String* currentSubString(unsigned length)
    {
        int offset = m_string->length() - m_length;
        return m_string->substring(offset, length);
    }

    ALWAYS_INLINE char32_t getCurrentChar()
    {
        STARFISH_ASSERT(m_length);
        if (is8Bit())
            return getCurrentChar8();
        return getCurrentChar32();
    }

    ALWAYS_INLINE char32_t incrementAndGetCurrentChar()
    {
        STARFISH_ASSERT(m_length);
        if (is8Bit())
            return incrementAndGetCurrentChar8();
        return incrementAndGetCurrentChar32();
    }

public:
    union {
        const char* string8Ptr;
        const char32_t* string32Ptr;
    } m_data;
    int m_length;

private:
    bool m_doNotExcludeLineNumbers;
    bool m_is8Bit;
    String* m_string;
};

// An abstract number of element in a sequence. The sequence has a first element.
// This type should be used instead of integer because 2 contradicting traditions can
// call a first element '0' or '1' which makes integer type ambiguous.
class OrdinalNumber {
public:
    static OrdinalNumber fromZeroBasedInt(int zeroBasedInt) { return OrdinalNumber(zeroBasedInt); }
    static OrdinalNumber fromOneBasedInt(int oneBasedInt) { return OrdinalNumber(oneBasedInt - 1); }
    OrdinalNumber()
        : m_zeroBasedValue(0)
        { }

    int zeroBasedInt() const { return m_zeroBasedValue; }
    int oneBasedInt() const { return m_zeroBasedValue + 1; }

    bool operator==(OrdinalNumber other) { return m_zeroBasedValue == other.m_zeroBasedValue; }
    bool operator!=(OrdinalNumber other) { return !((*this) == other); }

    static OrdinalNumber first() { return OrdinalNumber(0); }
    static OrdinalNumber beforeFirst() { return OrdinalNumber(-1); }

private:
    OrdinalNumber(int zeroBasedInt)
        : m_zeroBasedValue(zeroBasedInt)
        { }
    int m_zeroBasedValue;
};

class SegmentedString {
public:
    SegmentedString()
        : m_pushedChar1(0)
        , m_pushedChar2(0)
        , m_currentChar(0)
        , m_numberOfCharactersConsumedPriorToCurrentString(0)
        , m_numberOfCharactersConsumedPriorToCurrentLine(0)
        , m_currentLine(0)
        , m_closed(false)
        , m_empty(true)
        , m_fastPathFlags(NoFastPath)
        , m_advanceFunc(&SegmentedString::advanceEmpty)
        , m_advanceAndUpdateLineNumberFunc(&SegmentedString::advanceEmpty)
    {
    }

    SegmentedString(String* str)
        : m_pushedChar1(0)
        , m_pushedChar2(0)
        , m_currentString(str)
        , m_currentChar(0)
        , m_numberOfCharactersConsumedPriorToCurrentString(0)
        , m_numberOfCharactersConsumedPriorToCurrentLine(0)
        , m_currentLine(0)
        , m_closed(false)
        , m_empty(!str->length())
        , m_fastPathFlags(NoFastPath)
    {
        if (m_currentString.m_length)
            m_currentChar = m_currentString.getCurrentChar();
        updateAdvanceFunctionPointers();
    }

    void clear();
    void close();

    void append(const SegmentedString&);
    void prepend(const SegmentedString&);

    bool excludeLineNumbers() const { return m_currentString.excludeLineNumbers(); }
    void setExcludeLineNumbers();

    void push(char32_t c)
    {
        if (!m_pushedChar1) {
            m_pushedChar1 = c;
            m_currentChar = m_pushedChar1 ? m_pushedChar1 : m_currentString.getCurrentChar();
            updateSlowCaseFunctionPointers();
        } else {
            STARFISH_ASSERT(!m_pushedChar2);
            m_pushedChar2 = c;
        }
    }

    bool isEmpty() const { return m_empty; }
    unsigned length() const;

    bool isClosed() const { return m_closed; }

    enum LookAheadResult {
        DidNotMatch,
        DidMatch,
        NotEnoughCharacters,
    };

    LookAheadResult lookAhead(String* string) { return lookAheadInline(string, true); }
    LookAheadResult lookAheadIgnoringCase(String* string) { return lookAheadInline(string, false); }

    void advance()
    {
        if (m_fastPathFlags & Use8BitAdvance) {
            STARFISH_ASSERT(!m_pushedChar1);
            bool haveOneCharacterLeft = (--m_currentString.m_length == 1);
            m_currentChar = m_currentString.incrementAndGetCurrentChar8();

            if (!haveOneCharacterLeft)
                return;

            updateSlowCaseFunctionPointers();

            return;
        }

        (this->*m_advanceFunc)();
    }

    inline void advanceAndUpdateLineNumber()
    {
        if (m_fastPathFlags & Use8BitAdvance) {
            STARFISH_ASSERT(!m_pushedChar1);

            bool haveNewLine = (m_currentChar == '\n') & !!(m_fastPathFlags & Use8BitAdvanceAndUpdateLineNumbers);
            bool haveOneCharacterLeft = (--m_currentString.m_length == 1);

            m_currentChar = m_currentString.incrementAndGetCurrentChar8();

            if (!(haveNewLine | haveOneCharacterLeft))
                return;

            if (haveNewLine) {
                ++m_currentLine;
                m_numberOfCharactersConsumedPriorToCurrentLine =  m_numberOfCharactersConsumedPriorToCurrentString + m_currentString.numberOfCharactersConsumed();
            }

            if (haveOneCharacterLeft)
                updateSlowCaseFunctionPointers();

            return;
        }

        (this->*m_advanceAndUpdateLineNumberFunc)();
    }

    void advanceAndASSERT(UChar expectedCharacter)
    {
        // ASSERT_UNUSED(expectedCharacter, currentChar() == expectedCharacter);
        advance();
    }

    void advanceAndASSERTIgnoringCase(UChar expectedCharacter)
    {
        // ASSERT_UNUSED(expectedCharacter, WTF::Unicode::foldCase(currentChar()) == WTF::Unicode::foldCase(expectedCharacter));
        advance();
    }

    void advancePastNonNewline()
    {
        STARFISH_ASSERT(currentChar() != '\n');
        advance();
    }

    void advancePastNewlineAndUpdateLineNumber()
    {
        STARFISH_ASSERT(currentChar() == '\n');
        if (!m_pushedChar1 && m_currentString.m_length > 1) {
            int newLineFlag = m_currentString.doNotExcludeLineNumbers();
            m_currentLine += newLineFlag;
            if (newLineFlag)
                m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + 1;
            decrementAndCheckLength();
            m_currentChar = m_currentString.incrementAndGetCurrentChar();
            return;
        }
        advanceAndUpdateLineNumberSlowCase();
    }

    // Writes the consumed characters into consumedCharacters, which must
    // have space for at least |count| characters.
    void advance(unsigned count, char32_t* consumedCharacters);

    bool escaped() const { return m_pushedChar1; }

    int numberOfCharactersConsumed() const
    {
        int numberOfPushedCharacters = 0;
        if (m_pushedChar1) {
            ++numberOfPushedCharacters;
            if (m_pushedChar2)
                ++numberOfPushedCharacters;
        }
        return m_numberOfCharactersConsumedPriorToCurrentString + m_currentString.numberOfCharactersConsumed() - numberOfPushedCharacters;
    }

    String* toString() const;

    char32_t currentChar() const { return m_currentChar; }

    // The method is moderately slow, comparing to currentLine method.
    OrdinalNumber currentColumn() const;
    OrdinalNumber currentLine() const;
    // Sets value of line/column variables. Column is specified indirectly by a parameter columnAftreProlog
    // which is a value of column that we should get after a prolog (first prologLength characters) has been consumed.
    void setCurrentPosition(OrdinalNumber line, OrdinalNumber columnAftreProlog, int prologLength);

private:
    enum FastPathFlags {
        NoFastPath = 0,
        Use8BitAdvanceAndUpdateLineNumbers = 1 << 0,
        Use8BitAdvance = 1 << 1,
    };

    void append(const SegmentedSubstring&);
    void prepend(const SegmentedSubstring&);

    void advance8();
    void advance16();
    void advanceAndUpdateLineNumber8();
    void advanceAndUpdateLineNumber16();
    void advanceSlowCase();
    void advanceAndUpdateLineNumberSlowCase();
    void advanceEmpty();
    void advanceSubstring();

    void updateSlowCaseFunctionPointers();

    void decrementAndCheckLength()
    {
        STARFISH_ASSERT(m_currentString.m_length > 1);
        if (--m_currentString.m_length == 1)
            updateSlowCaseFunctionPointers();
    }

    void updateAdvanceFunctionPointers()
    {
        if ((m_currentString.m_length > 1) && !m_pushedChar1) {
            if (m_currentString.is8Bit()) {
                m_advanceFunc = &SegmentedString::advance8;
                m_fastPathFlags = Use8BitAdvance;
                if (m_currentString.doNotExcludeLineNumbers()) {
                    m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceAndUpdateLineNumber8;
                    m_fastPathFlags |= Use8BitAdvanceAndUpdateLineNumbers;
                } else {
                    m_advanceAndUpdateLineNumberFunc = &SegmentedString::advance8;
                }
                return;
            }

            m_advanceFunc = &SegmentedString::advance16;
            m_fastPathFlags = NoFastPath;
            if (m_currentString.doNotExcludeLineNumbers())
                m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceAndUpdateLineNumber16;
            else
                m_advanceAndUpdateLineNumberFunc = &SegmentedString::advance16;
            return;
        }

        if (!m_currentString.m_length && !isComposite()) {
            m_advanceFunc = &SegmentedString::advanceEmpty;
            m_fastPathFlags = NoFastPath;
            m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
        }

        updateSlowCaseFunctionPointers();
    }

    inline LookAheadResult lookAheadInline(String* string, bool caseSensitive)
    {
        if (!m_pushedChar1 && string->length() <= static_cast<unsigned>(m_currentString.m_length)) {
            String* currentSubstring = m_currentString.currentSubString(string->length());
            if (currentSubstring->startsWith(string, caseSensitive))
                return DidMatch;
            return DidNotMatch;
        }
        return lookAheadSlowCase(string, caseSensitive);
    }

    LookAheadResult lookAheadSlowCase(String* string, bool caseSensitive)
    {
        unsigned count = string->length();
        if (count > length())
            return NotEnoughCharacters;
        UTF32String consumedCharacters;
        consumedCharacters.resize(count);
        advance(count, (char32_t*)consumedCharacters.data());
        LookAheadResult result = DidNotMatch;
        String* consumedString = new StringDataUTF32(std::move(consumedCharacters));
        if (consumedString->startsWith(string, caseSensitive))
            result = DidMatch;
        prepend(SegmentedString(consumedString));
        return result;
    }

    bool isComposite() const { return !(m_substrings.size() == 0); }

    char32_t m_pushedChar1;
    char32_t m_pushedChar2;
    SegmentedSubstring m_currentString;
    char32_t m_currentChar;
    int m_numberOfCharactersConsumedPriorToCurrentString;
    int m_numberOfCharactersConsumedPriorToCurrentLine;
    int m_currentLine;
    std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>> m_substrings;
    bool m_closed;
    bool m_empty;
    unsigned char m_fastPathFlags;
    void (SegmentedString::*m_advanceFunc)();
    void (SegmentedString::*m_advanceAndUpdateLineNumberFunc)();
};


}
#endif
