#ifndef __StarFishHTMLParserIdioms__
#define __StarFishHTMLParserIdioms__

#include "util/String.h"

namespace StarFish {

// This class represents decimal base floating point number.
//
// FIXME: Once all C++ compiler support decimal type, we should replace this
// class to compiler supported one. See below URI for current status of decimal
// type for C++: // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2006/n1977.html
class Decimal : public gc {
public:
    enum Sign {
        Positive,
        Negative,
    };

    // You should not use EncodedData other than unit testing.
    class EncodedData {
        // For accessing FormatClass.
        friend class Decimal;
        // friend class DecimalPrivate::SpecialValueHandler;
    public:
        EncodedData(Sign, int exponent, uint64_t coefficient);

        bool operator==(const EncodedData&) const;
        bool operator!=(const EncodedData& another) const { return !operator==(another); }

        uint64_t coefficient() const { return m_coefficient; }
        int countDigits() const;
        int exponent() const { return m_exponent; }
        bool isFinite() const { return !isSpecial(); }
        bool isInfinity() const { return m_formatClass == ClassInfinity; }
        bool isNaN() const { return m_formatClass == ClassNaN; }
        bool isSpecial() const { return m_formatClass == ClassInfinity || m_formatClass == ClassNaN; }
        bool isZero() const { return m_formatClass == ClassZero; }
        Sign sign() const { return m_sign; }
        void setSign(Sign sign) { m_sign = sign; }

    private:
        enum FormatClass {
            ClassInfinity,
            ClassNormal,
            ClassNaN,
            ClassZero,
        };

        EncodedData(Sign, FormatClass);
        FormatClass formatClass() const { return m_formatClass; }

        uint64_t m_coefficient;
        int16_t m_exponent;
        FormatClass m_formatClass;
        Sign m_sign;
    };

    Decimal(int32_t = 0);
    Decimal(Sign, int exponent, uint64_t coefficient);
    Decimal(const Decimal&);

    Decimal& operator=(const Decimal&);
    Decimal& operator+=(const Decimal&);
    Decimal& operator-=(const Decimal&);
    Decimal& operator*=(const Decimal&);
    Decimal& operator/=(const Decimal&);

    Decimal operator-() const;

    bool operator==(const Decimal&) const;
    bool operator!=(const Decimal&) const;
    bool operator<(const Decimal&) const;
    bool operator<=(const Decimal&) const;
    bool operator>(const Decimal&) const;
    bool operator>=(const Decimal&) const;

    Decimal operator+(const Decimal&) const;
    Decimal operator-(const Decimal&) const;
    Decimal operator*(const Decimal&) const;
    Decimal operator/(const Decimal&) const;

    int exponent() const
    {
        STARFISH_ASSERT(isFinite());
        return m_data.exponent();
    }

    bool isFinite() const { return m_data.isFinite(); }
    bool isInfinity() const { return m_data.isInfinity(); }
    bool isNaN() const { return m_data.isNaN(); }
    bool isNegative() const { return sign() == Negative; }
    bool isPositive() const { return sign() == Positive; }
    bool isSpecial() const { return m_data.isSpecial(); }
    bool isZero() const { return m_data.isZero(); }

    Decimal abs() const;
    Decimal ceiling() const;
    Decimal floor() const;
    Decimal remainder(const Decimal&) const;
    Decimal round() const;

    double toDouble() const;
    // Note: toString method supports infinity and nan but fromString not.
    String toString() const;

    static Decimal fromDouble(double);
    // fromString supports following syntax EBNF:
    //  number ::= sign? digit+ ('.' digit*) (exponent-marker sign? digit+)?
    //          | sign? '.' digit+ (exponent-marker sign? digit+)?
    //  sign ::= '+' | '-'
    //  exponent-marker ::= 'e' | 'E'
    //  digit ::= '0' | '1' | ... | '9'
    // Note: fromString doesn't support "infinity" and "nan".
    static Decimal fromString(const String&);
    static Decimal infinity(Sign);
    static Decimal nan();
    static Decimal zero(Sign);

    // You should not use below methods. We expose them for unit testing.
    explicit Decimal(const EncodedData&);
    const EncodedData& value() const { return m_data; }

private:
    struct AlignedOperands {
        uint64_t lhsCoefficient;
        uint64_t rhsCoefficient;
        int exponent;
    };

    Decimal(double);
    Decimal compareTo(const Decimal&) const;

    static AlignedOperands alignOperands(const Decimal& lhs, const Decimal& rhs);
    static inline Sign invertSign(Sign sign) { return sign == Negative ? Positive : Negative; }

    Sign sign() const { return m_data.sign(); }

    EncodedData m_data;
};

// Space characters as defined by the HTML specification.
// bool isHTMLSpace(char32_t);
bool isHTMLLineBreak(char32_t);
bool isNotHTMLSpace(char32_t);

// Strip leading and trailing whitespace as defined by the HTML specification.
String* stripLeadingAndTrailingHTMLSpaces(String*);
inline String* stripLeadingAndTrailingHTMLSpaces(const UTF32String& vector)
{
    return stripLeadingAndTrailingHTMLSpaces(String::createASCIIStringFromUTF32SourceIfPossible(vector.data()));
}

// An implementation of the HTML specification's algorithm to convert a number to a string for number and range types.
String serializeForNumberType(const Decimal&);
String serializeForNumberType(double);

// Convert the specified string to a decimal/double. If the conversion fails, the return value is fallback value or NaN if not specified.
// Leading or trailing illegal characters cause failure, as does passing an empty string.
// The double* parameter may be 0 to check if the string can be parsed without getting the result.
Decimal parseToDecimalForNumberType(String*, const Decimal& fallbackValue = Decimal::nan());
double parseToDoubleForNumberType(String*, double fallbackValue = std::numeric_limits<double>::quiet_NaN());

// http://www.whatwg.org/specs/web-apps/current-work/#rules-for-parsing-integers
bool parseHTMLInteger(String*, int&);

// http://www.whatwg.org/specs/web-apps/current-work/#rules-for-parsing-non-negative-integers
bool parseHTMLNonNegativeInteger(String*, unsigned int&);

typedef std::vector<std::pair<String*, String*>, gc_allocator<std::pair<String*, String*>>> HTMLAttributeList;
// The returned encoding might not be valid.
// WTF::TextEncoding encodingFromMetaAttributes(const HTMLAttributeList&);

// Inline implementations of some of the functions declared above.

template<typename CharType = char32_t>
inline bool isHTMLSpace(CharType character)
{
    // Histogram from Apple's page load test combined with some ad hoc browsing some other test suites.
    //
    // 82%: 216330 non-space characters, all > U+0020
    // 11%: 30017 plain space characters, U+0020
    // 5%: 12099 newline characters, U+000A
    // 2%: 5346 tab characters, U+0009
    //
    // No other characters seen. No U+000C or U+000D, and no other control characters.
    // Accordingly, we check for non-spaces first, then space, then newline, then tab, then the other characters.

    return character <= ' ' && (character == ' ' || character == '\n' || character == '\t' || character == '\r' || character == '\f');
}

template<typename CharType>
inline bool isHTMLSpaceOrComma(CharType character)
{
    return isHTMLSpace<CharType>(character) || character == ',';
}

inline bool isHTMLLineBreak(UChar character)
{
    return character <= '\r' && (character == '\n' || character == '\r');
}

template<typename CharType>
inline bool isNotHTMLSpace(CharType character)
{
    return !isHTMLSpace<CharType>(character);
}

bool threadSafeMatch(const QualifiedName&, const QualifiedName&);
bool threadSafeMatch(const String&, const QualifiedName&);

// StringImpl* findStringIfStatic(const char32_t* characters, unsigned length);

enum CharacterWidth {
    Likely8Bit,
    Force8Bit,
    Force16Bit
};

inline String* attemptStaticStringCreation(const UTF32String& vector, CharacterWidth width)
{
    /*
    String string(findStringIfStatic(vector.data(), vector.size()));
    if (string.impl())
        return string;
    if (width == Likely8Bit)
        string = StringImpl::create8BitIfPossible(vector);
    else if (width == Force8Bit)
        string = String::make8BitFrom16BitSource(vector);
    else
        string = String(vector);
     */
    String* string;
    if (width == Likely8Bit)
        string = String::createASCIIStringFromUTF32SourceIfPossible(vector.data());
    else if (width == Force8Bit)
        string = String::createASCIIStringFromUTF32Source(vector.data());
    else
        string = new StringDataUTF32(vector);
    return string;
}

}
#endif
