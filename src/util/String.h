#ifndef __StarFishString__
#define __StarFishString__

namespace StarFish {

typedef std::basic_string<char, std::char_traits<char>, gc_allocator<char> > ASCIIString;
typedef std::basic_string<char32_t, std::char_traits<char32_t>, gc_allocator<char32_t> > UTF32String;
class StringDataASCII;
class String;

class String {
public:
    typedef std::vector<String*, gc_allocator<String*> > Vector;

    static String* emptyString;
    static String* spaceString;

    static String* fromUTF8(const char* src);
    static String* fromUTF8(const char* src, size_t len);
    static String* createASCIIString(const char* src);
    static String* createUTF32String(const char32_t* c);
    static String* createUTF32String(char32_t c);

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

    const char* utf8Data();

    bool equals(const String* str) const;
    bool equalsWithoutCase(const String* str) const;
    bool equals(const char* str)
    {
#ifndef NDEBUG
        {
            const char* c = str;
            while (*c) {
                STARFISH_ASSERT(*c > 0);
                c++;
            }
        }
#endif
        if (*asASCIIString() == str) {
            return true;
        }
        return false;
    }

    size_t length() const
    {
        if (m_isASCIIString) {
            return asASCIIString()->length();
        } else {
            return asUTF32String()->length();
        }
    }

    char32_t charAt(size_t idx)
    {
        if (m_isASCIIString) {
            return (*asASCIIString())[idx];
        } else {
            return (*asUTF32String())[idx];
        }
    }

    char32_t operator[](size_t idx)
    {
        return charAt(idx);
    }

    size_t indexOf(char32_t ch)
    {
        for (size_t i = 0; i < length(); i ++) {
            if (charAt(i) == ch) {
                return i;
            }
        }
        return SIZE_MAX;
    }

    size_t lastIndexOf(char32_t ch)
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
    String* replaceAll(const std::string& from, const std::string& to);
    bool isASCIIString() const
    {
        return m_isASCIIString;
    }
    String* concat(String* str);
    void split(char delim, Vector& tokens);
    String* trim();

    // token is only 1-byte char now.
    std::vector<String*, gc_allocator<String*>> tokenize(const char* tokens, size_t tokensLength);

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

    UTF32String toUTF32String();
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

}
#endif
