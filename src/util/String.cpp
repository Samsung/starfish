#include "StarFishConfig.h"
#include "String.h"
#include <sstream>

namespace StarFish {

String* String::emptyString = String::createASCIIString("");
String* String::spaceString = String::createASCIIString(" ");

size_t utf8ToUtf32(const char* UTF8, char32_t& uc)
{
    size_t tRequiredSize = 0;

    uc = 0x00000000;

    // ASCII byte
    if (0 == (UTF8[0] & 0x80)) {
        uc = UTF8[0];
        tRequiredSize = 1;
    } else // Start byte for 2byte
        if (0xC0 == (UTF8[0] & 0xE0)
            && 0x80 == (UTF8[1] & 0xC0)) {
            uc += (UTF8[0] & 0x1F) << 6;
            uc += (UTF8[1] & 0x3F) << 0;
            tRequiredSize = 2;
        } else // Start byte for 3byte
            if (0xE0 == (UTF8[0] & 0xF0)
                && 0x80 == (UTF8[1] & 0xC0)
                && 0x80 == (UTF8[2] & 0xC0)) {
                uc += (UTF8[0] & 0x0F) << 12;
                uc += (UTF8[1] & 0x3F) << 6;
                uc += (UTF8[2] & 0x3F) << 0;
                tRequiredSize = 3;
            } else // Start byte for 4byte
                if (0xF0 == (UTF8[0] & 0xF8)
                    && 0x80 == (UTF8[1] & 0xC0)
                    && 0x80 == (UTF8[2] & 0xC0)
                    && 0x80 == (UTF8[3] & 0xC0)) {
                    uc += (UTF8[0] & 0x07) << 18;
                    uc += (UTF8[1] & 0x3F) << 12;
                    uc += (UTF8[2] & 0x3F) << 6;
                    uc += (UTF8[3] & 0x3F) << 0;
                    tRequiredSize = 4;
                } else // Start byte for 5byte
                    if (0xF8 == (UTF8[0] & 0xFC)
                        && 0x80 == (UTF8[1] & 0xC0)
                        && 0x80 == (UTF8[2] & 0xC0)
                        && 0x80 == (UTF8[3] & 0xC0)
                        && 0x80 == (UTF8[4] & 0xC0)) {
                        uc += (UTF8[0] & 0x03) << 24;
                        uc += (UTF8[1] & 0x3F) << 18;
                        uc += (UTF8[2] & 0x3F) << 12;
                        uc += (UTF8[3] & 0x3F) << 6;
                        uc += (UTF8[4] & 0x3F) << 0;
                        tRequiredSize = 5;
                    } else // Start byte for 6byte
                        if (0xFC == (UTF8[0] & 0xFE)
                            && 0x80 == (UTF8[1] & 0xC0)
                            && 0x80 == (UTF8[2] & 0xC0)
                            && 0x80 == (UTF8[3] & 0xC0)
                            && 0x80 == (UTF8[4] & 0xC0)
                            && 0x80 == (UTF8[5] & 0xC0)) {
                            uc += (UTF8[0] & 0x01) << 30;
                            uc += (UTF8[1] & 0x3F) << 24;
                            uc += (UTF8[2] & 0x3F) << 18;
                            uc += (UTF8[3] & 0x3F) << 12;
                            uc += (UTF8[4] & 0x3F) << 6;
                            uc += (UTF8[5] & 0x3F) << 0;
                            tRequiredSize = 6;
                        } else {
                            tRequiredSize = 1;
                            uc = 0xFFFD;
                        }

    return tRequiredSize;
}
size_t utf32ToUtf8(char32_t uc, char* UTF8)
{
    size_t tRequiredSize = 0;

    if (uc <= 0x7f) {
        if (NULL != UTF8) {
            UTF8[0] = (char)uc;
            UTF8[1] = (char)'\0';
        }
        tRequiredSize = 1;
    } else if (uc <= 0x7ff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xc0 + uc / (0x01 << 6));
            UTF8[1] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[2] = (char)'\0';
        }
        tRequiredSize = 2;
    } else if (uc <= 0xffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xe0 + uc / (0x01 << 12));
            UTF8[1] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[2] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[3] = (char)'\0';
        }
        tRequiredSize = 3;
    } else if (uc <= 0x1fffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xf0 + uc / (0x01 <<18));
            UTF8[1] = (char)(0x80 + uc / (0x01 <<12) % (0x01 <<12));
            UTF8[2] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[3] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[4] = (char) '\0';
        }
        tRequiredSize = 4;
    } else if (uc <= 0x3ffffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xf8 + uc / (0x01 <<24));
            UTF8[1] = (char)(0x80 + uc / (0x01 <<18) % (0x01 <<18));
            UTF8[2] = (char)(0x80 + uc / (0x01 <<12) % (0x01 <<12));
            UTF8[3] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[4] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[5] = (char) '\0';
        }
        tRequiredSize = 5;
    } else if (uc <= 0x7fffffff) {
        if (NULL != UTF8) {
            UTF8[0] = (char)(0xfc + uc / (0x01 <<30));
            UTF8[1] = (char)(0x80 + uc / (0x01 <<24) % (0x01 <<24));
            UTF8[2] = (char)(0x80 + uc / (0x01 <<18) % (0x01 <<18));
            UTF8[3] = (char)(0x80 + uc / (0x01 <<12) % (0x01 <<12));
            UTF8[4] = (char)(0x80 + uc / (0x01 << 6) % (0x01 << 6));
            UTF8[5] = (char)(0x80 + uc % (0x01 << 6));
            UTF8[6] = (char) '\0';
        }
        tRequiredSize = 6;
    } else {
        tRequiredSize = 1;
        uc = 0xFFFD;
    }

    return tRequiredSize;
}


const char* utf32ToUtf8(const char32_t* t, const size_t& len, size_t* bufferSize = NULL)
{
    unsigned strLength = 0;
    char buffer[8];
    for (size_t i = 0; i < len; i++) {
        int length = utf32ToUtf8(t[i], buffer);
        strLength += length;
    }

    char* result = (char*)GC_MALLOC_ATOMIC(strLength + 1);
    if (bufferSize)
        *bufferSize = strLength + 1;
    unsigned currentPosition = 0;

    for (size_t i = 0; i < len; i++) {
        int length = utf32ToUtf8(t[i], buffer);
        memcpy(&result[currentPosition], buffer, length);
        currentPosition += length;
    }
    result[strLength] = 0;

    return result;
}

StringDataUTF32::StringDataUTF32(const char* src, size_t len)
{
    m_isASCIIString = false;
    const char* end = src + len;
    while (end != src) {
        char32_t c;
        src += utf8ToUtf32(src, c);
        UTF32String::operator+=(c);
    }
    putDebugInfo();
}

const char* String::utf8DataSlowCase()
{
    STARFISH_ASSERT(!m_isASCIIString);
    return utf32ToUtf8(asUTF32String()->data(), asUTF32String()->length());
}

String* String::fromUTF8(const char* src, size_t len)
{
    for (unsigned i = 0; i < len; i ++) {
        if (src[i] & 0x80) {
            return new StringDataUTF32(src, len);
        }
    }
    return new StringDataASCII(src, len);
}

String* String::fromUTF8(const char* str)
{
    const char* p = str;
    while (*p) {
        if (*p & 0x80) {
            return new StringDataUTF32(str, strlen(str));
        }
        p++;
    }

    return new StringDataASCII(str);
}

String* String::createASCIIString(const char* str)
{
    return new StringDataASCII(str);
}

String* String::createUTF32String(const UTF32String& src)
{
    return new StringDataUTF32(src);
}

String* String::createUTF32String(char32_t c)
{
    if (c < 128) {
        char s[2] = {(char)c, '\0'};
        return new StringDataASCII(s);
    }
    char32_t s[2] = {c, '\0'};
    return new StringDataUTF32(s);
}

String* String::createASCIIStringFromUTF32Source(const UTF32String& src)
{
#ifndef NDEBUG
    for (size_t i = 0; i < src.length(); i ++) {
        const char32_t c = src[i];
        STARFISH_ASSERT(c < 128);
    }
#endif
    ASCIIString ascii;
    for (size_t i = 0; i < src.length(); i ++) {
        ascii.push_back(src[i]);
    }
    return new StringDataASCII(std::move(ascii));
}

String* String::createASCIIStringFromUTF32SourceIfPossible(const UTF32String& src)
{
    for (size_t i = 0; i < src.length(); i ++) {
        const char32_t c = src[i];
        if (c > 127) {
            return String::createUTF32String(src);
        }
    }
    ASCIIString ascii;
    for (size_t i = 0; i < src.length(); i ++) {
        ascii.push_back(src[i]);
    }
    return new StringDataASCII(std::move(ascii));
}

const char* String::utf8Data()
{
    if (m_isASCIIString) {
        return asASCIIString()->data();
    } else {
        return utf8DataSlowCase();
    }
}

String* String::substring(size_t pos, size_t len)
{
    if (m_isASCIIString) {
        return new StringDataASCII(std::move(asASCIIString()->substr(pos, len)));
    } else {
        return new StringDataUTF32(std::move(asUTF32String()->substr(pos, len)));
    }
}

String* String::toUpper()
{
    if (m_isASCIIString) {
        ASCIIString str = *asASCIIString();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return new StringDataASCII(std::move(str));
    } else {
        UTF32String str = *asUTF32String();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return new StringDataUTF32(std::move(str));
    }
}

String* String::toLower()
{
    if (m_isASCIIString) {
        ASCIIString str = *asASCIIString();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return new StringDataASCII(std::move(str));
    } else {
        UTF32String str = *asUTF32String();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return new StringDataUTF32(std::move(str));
    }
}

String* String::concat(String* str)
{
    if (isASCIIString() && str->isASCIIString()) {
        ASCIIString s = *asASCIIString() + *(str->asASCIIString());
        return new StringDataASCII(std::move(s));
    } else {
        UTF32String a = toUTF32String();
        UTF32String b = str->toUTF32String();
        a = a + b;
        return new StringDataUTF32(std::move(a));
    }
}

String* String::replaceAll(String* from, String* to)
{
    if (m_isASCIIString) {
        std::string str = std::string(utf8Data());
        std::string from_str = std::string(from->utf8Data());
        std::string to_str = std::string(to->utf8Data());

        size_t start_pos = 0;
        while ((start_pos = str.find(from_str, start_pos)) != std::string::npos) {
            str.replace(start_pos, from_str.length(), to_str);
            start_pos += to_str.length(); // Handles case where 'to' is a substring of 'from'
        }
        return createASCIIString(str.c_str());
    } else {
        std::basic_string<char32_t> str(toUTF32String().data());
        std::basic_string<char32_t> from_str(from->toUTF32String().data());
        std::basic_string<char32_t> to_str(to->toUTF32String().data());

        size_t start_pos = 0;
        while ((start_pos = str.find(from_str, start_pos)) != std::string::npos) {
            str.replace(start_pos, from_str.length(), to_str);
            start_pos += to_str.length(); // Handles case where 'to' is a substring of 'from'
        }
        return createUTF32String(UTF32String(str.begin(), str.end()));
    }
}

String* String::fromFloat(float f)
{
    if (f == std::floor(f))
        return String::fromUTF8(std::to_string((int)std::floor(f)).c_str());

    return String::fromUTF8(std::to_string(f).c_str());
}

String* String::fromInt(int i)
{
    return String::fromUTF8(std::to_string(i).c_str());
}

void String::split(char delim, Vector& tokens)
{
    if (m_isASCIIString) {
        std::stringstream ss;
        ss << utf8Data();
        std::string item;
        while (std::getline(ss, item, delim)) {
            tokens.push_back(String::fromUTF8(item.c_str()));
        }
    } else {
        std::basic_stringstream<char32_t> ss;
        ss << toUTF32String().data();
        std::basic_string<char32_t> item;
        while (std::getline(ss, item, (char32_t)delim)) {
            tokens.push_back(String::createUTF32String(UTF32String(item.begin(), item.end())));
        }
    }
}

String* String::trim()
{
    size_t first = 0;
    size_t last = 0;
    if (length()) {
        last = length() - 1;

        for (size_t i = 0; i < length(); i ++) {
            if (!String::isSpaceOrNewline(charAt(i))) {
                first = i;
                break;
            }
        }

        do {
            if (!String::isSpaceOrNewline(charAt(last)))
                break;
        } while (last--);
    }

    return substring(first, (last - first + 1));
}

std::vector<String*, gc_allocator<String*> > String::tokenize(const char* tokens, size_t tokensLength)
{
    std::vector<String*, gc_allocator<String*> > result;
    const char* data = utf8Data();
    size_t length = strlen(data);

    std::string str;
    for (size_t i = 0; i < length; i++) {
        char c = data[i];
        bool isToken = false;
        for (size_t j = 0; j < tokensLength; j++) {
            if (c == tokens[j]) {
                isToken = true;
                break;
            }
        }

        if (isToken) {
            result.push_back(String::fromUTF8(str.data(), str.length()));
            str.clear();
        } else {
            str += c;
        }
    }

    if (str.length()) {
        result.push_back(String::fromUTF8(str.data(), str.length()));
    }

    return result;
}

static int utf32ToUtf16(char32_t i, char16_t *u)
{
    if (i < 0xffff) {
        *u= (char16_t)(i & 0xffff);
        return 1;
    } else if (i < 0x10ffff) {
        i-= 0x10000;
        *u++= 0xd800 | (i >> 10);
        *u= 0xdc00 | (i & 0x3ff);
        return 2;
    } else {
        // produce error char
        // U+FFFD
        *u = 0xFFFD;
        return 1;
    }
}

UTF32String String::toUTF32String()
{
    if (m_isASCIIString) {
        UTF32String str;
        const ASCIIString& src = *asASCIIString();
        size_t len = src.length();
        for (size_t i = 0; i < len; i ++) {
            str.push_back(src[i]);
        }
        return str;
    } else {
        return *asUTF32String();
    }
}

UTF16String String::toUTF16String() const
{
    UTF16String out;
    for (size_t i = 0; i < length(); i++) {
        char32_t src = charAt(i);
        char16_t dst[2];
        int ret = utf32ToUtf16(src, dst);

        if (ret == 1) {
            out.push_back(src);
        } else if (ret == 2) {
            out.push_back(dst[0]);
            out.push_back(dst[1]);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return out;
}

bool String::equals(const String* str) const
{
    size_t lenA = length();
    size_t lenB = str->length();
    if (lenA == lenB) {
        bool aa = isASCIIString();
        bool bb = str->isASCIIString();
        if (aa && bb) {
            return stringEqual(asASCIIString()->data(), str->asASCIIString()->data(), lenA);
        } else if (aa && !bb) {
            return stringEqual(str->asUTF32String()->data(), asASCIIString()->data(), lenA);
        } else if (!aa && bb) {
            return stringEqual(asUTF32String()->data(), str->asASCIIString()->data(), lenA);
        } else {
            return stringEqual(asUTF32String()->data(), str->asUTF32String()->data(), lenA);
        }
    }
    return false;
}

template <typename T>
bool stringEqualWithoutCase(const T* s, const T* s1, const size_t& len)
{
    for (size_t i = 0; i < len; i ++) {
        if (tolower(s[i]) != tolower(s1[i]))
            return false;
    }
    return true;
}

bool stringEqualWithoutCase(const char32_t* s, const char* s1, const size_t& len)
{
    for (size_t i = 0; i < len ; i ++) {
        if (towlower(s[i]) != towlower((unsigned char)s1[i])) {
            return false;
        }
    }
    return true;
}

bool String::equalsWithoutCase(const String* str) const
{
    size_t lenA = length();
    size_t lenB = str->length();
    if (lenA == lenB) {
        bool aa = isASCIIString();
        bool bb = str->isASCIIString();
        if (aa && bb) {
            return stringEqualWithoutCase(asASCIIString()->data(), str->asASCIIString()->data(), lenA);
        } else if (aa && !bb) {
            return stringEqualWithoutCase(str->asUTF32String()->data(), asASCIIString()->data(), lenA);
        } else if (!aa && bb) {
            return stringEqualWithoutCase(asUTF32String()->data(), str->asASCIIString()->data(), lenA);
        } else {
            return stringEqualWithoutCase(asUTF32String()->data(), str->asUTF32String()->data(), lenA);
        }
    }
    return false;
}

bool String::startsWith(const char* str, bool caseSensitive)
{
    // is string is all ascii??
    STARFISH_ASSERT(String::createASCIIString(str));
    bool result = true;
    size_t len = length();
    size_t strLen = strlen(str);
    if (strLen > len)
        return false;

    if (caseSensitive) {
        for (size_t i = 0; i < strLen; i++) {
            if ((char32_t)str[i] != charAt(i)) {
                result = false;
                break;
            }
        }
    } else {
        for (size_t i = 0; i < strLen; i++) {
            if (tolower(str[i]) != tolower(charAt(i))) {
                result = false;
                break;
            }
        }
    }

    return result;
}

bool String::startsWith(String* str, bool caseSensitive)
{
    bool result = true;
    size_t len = length();
    size_t strLen = str->length();
    if (strLen > len)
        return false;

    if (caseSensitive) {
        for (size_t i = 0; i < strLen; i++) {
            if (str->charAt(i) != charAt(i)) {
                result = false;
                break;
            }
        }
    } else {
        for (size_t i = 0; i < strLen; i++) {
            if (tolower(str->charAt(i)) != tolower(charAt(i))) {
                result = false;
                break;
            }
        }
    }

    return result;
}

size_t String::find(String* str, size_t pos)
{
    const size_t srcStrLen = str->length();
    const size_t size = length();

    if (srcStrLen == 0)
        return pos <= size ? pos : -1;

    if (srcStrLen <= size) {
        char32_t src0 = str->charAt(0);
        for (; pos <= size - srcStrLen; ++pos) {
            if (charAt(pos) == src0) {
                bool same = true;
                for (size_t k = 1; k < srcStrLen; k++) {
                    if (charAt(pos + k) != str->charAt(k)) {
                        same = false;
                        break;
                    }
                }
                if (same)
                    return pos;
            }
        }
    }
    return -1;
}

size_t String::find(const char* str, size_t pos)
{
    const size_t srcStrLen = strlen(str);
    const size_t size = length();

    if (srcStrLen == 0)
        return pos <= size ? pos : -1;

    if (srcStrLen <= size) {
        char32_t src0 = str[0];
        for (; pos <= size - srcStrLen; ++pos) {
            if (charAt(pos) == src0) {
                bool same = true;
                for (size_t k = 1; k < srcStrLen; k++) {
                    if (charAt(pos + k) != (char32_t)str[k]) {
                        same = false;
                        break;
                    }
                }
                if (same)
                    return pos;
            }
        }
    }
    return -1;
}

bool String::contains(const char* str, bool caseSensitive)
{
    size_t len = length();
    size_t strLen = strlen(str);

    if (strLen == 0)
        return true;

    if (caseSensitive) {
        if (strLen <= len) {
            char32_t src0 = str[0];
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (charAt(pos + k) != (char32_t)str[k]) {
                            same = false;
                            break;
                        }
                    }
                    if (same)
                        return true;
                }
            }
        }
    } else {
        if (strLen <= len) {
            char32_t src0 = str[0];
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (tolower(charAt(pos + k)) != tolower(str[k])) {
                            same = false;
                            break;
                        }
                    }
                    if (same)
                        return true;
                }
            }
        }
    }

    return false;
}

bool String::contains(String* str, bool caseSensitive)
{
    size_t len = length();
    size_t strLen = str->length();

    if (strLen == 0)
        return true;

    if (caseSensitive) {
        if (strLen <= len) {
            char32_t src0 = str->charAt(0);
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (charAt(pos + k) != str->charAt(k)) {
                            same = false;
                            break;
                        }
                    }
                    if (same)
                        return true;
                }
            }
        }
    } else {
        if (strLen <= len) {
            char32_t src0 = str->charAt(0);
            size_t pos = 0;
            for (; pos <= len - strLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < strLen; k++) {
                        if (tolower(charAt(pos + k)) != tolower(str->charAt(k))) {
                            same = false;
                            break;
                        }
                    }
                    if (same)
                        return true;
                }
            }
        }
    }

    return false;
}

unsigned SegmentedString::length() const
{
    unsigned length = m_currentString.m_length;
    if (m_pushedChar1) {
        ++length;
        if (m_pushedChar2)
            ++length;
    }
    if (isComposite()) {
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_iterator it = m_substrings.begin();
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_iterator e = m_substrings.end();
        for (; it != e; ++it)
            length += it->m_length;
    }
    return length;
}

void SegmentedString::setExcludeLineNumbers()
{
    m_currentString.setExcludeLineNumbers();
    if (isComposite()) {
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::iterator it = m_substrings.begin();
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::iterator e = m_substrings.end();
        for (; it != e; ++it)
            it->setExcludeLineNumbers();
    }
}

void SegmentedString::clear()
{
    m_pushedChar1 = 0;
    m_pushedChar2 = 0;
    m_currentChar = 0;
    m_currentString.clear();
    m_numberOfCharactersConsumedPriorToCurrentString = 0;
    m_numberOfCharactersConsumedPriorToCurrentLine = 0;
    m_currentLine = 0;
    m_substrings.clear();
    m_closed = false;
    m_empty = true;
    m_fastPathFlags = NoFastPath;
    m_advanceFunc = &SegmentedString::advanceEmpty;
    m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
}

void SegmentedString::append(const SegmentedSubstring& s)
{
    STARFISH_ASSERT(!m_closed);
    if (!s.m_length)
        return;

    if (!m_currentString.m_length) {
        m_numberOfCharactersConsumedPriorToCurrentString += m_currentString.numberOfCharactersConsumed();
        m_currentString = s;
        updateAdvanceFunctionPointers();
    } else {
        m_substrings.push_back(s);
    }
    m_empty = false;
}

void SegmentedString::prepend(const SegmentedSubstring& s)
{
    STARFISH_ASSERT(!escaped());
    STARFISH_ASSERT(!s.numberOfCharactersConsumed());
    if (!s.m_length)
        return;

    // FIXME: We're assuming that the prepend were originally consumed by
    //        this SegmentedString. We're also ASSERTing that s is a fresh
    //        SegmentedSubstring. These assumptions are sufficient for our
    //        current use, but we might need to handle the more elaborate
    //        cases in the future.
    m_numberOfCharactersConsumedPriorToCurrentString += m_currentString.numberOfCharactersConsumed();
    m_numberOfCharactersConsumedPriorToCurrentString -= s.m_length;
    if (!m_currentString.m_length) {
        m_currentString = s;
        updateAdvanceFunctionPointers();
    } else {
        // Shift our m_currentString into our list.
        m_substrings.insert(m_substrings.begin(), m_currentString);
        m_currentString = s;
        updateAdvanceFunctionPointers();
    }
    m_empty = false;
}

void SegmentedString::close()
{
    // Closing a stream twice is likely a coding mistake.
    STARFISH_ASSERT(!m_closed);
    m_closed = true;
}

void SegmentedString::append(const SegmentedString& s)
{
    STARFISH_ASSERT(!m_closed);
    STARFISH_ASSERT(!s.escaped());
    append(s.m_currentString);
    if (s.isComposite()) {
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_iterator it = s.m_substrings.begin();
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_iterator e = s.m_substrings.end();
        for (; it != e; ++it)
            append(*it);
    }
    m_currentChar = m_pushedChar1 ? m_pushedChar1 : (m_currentString.m_length ? m_currentString.getCurrentChar() : 0);
}

void SegmentedString::prepend(const SegmentedString& s)
{
    STARFISH_ASSERT(!escaped());
    STARFISH_ASSERT(!s.escaped());
    if (s.isComposite()) {
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_reverse_iterator it = s.m_substrings.rbegin();
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_reverse_iterator e = s.m_substrings.rend();
        for (; it != e; ++it)
            prepend(*it);
    }
    prepend(s.m_currentString);
    m_currentChar = m_pushedChar1 ? m_pushedChar1 : (m_currentString.m_length ? m_currentString.getCurrentChar() : 0);
}

void SegmentedString::advanceSubstring()
{
    if (isComposite()) {
        m_numberOfCharactersConsumedPriorToCurrentString += m_currentString.numberOfCharactersConsumed();
        m_currentString = m_substrings.front();
        m_substrings.pop_front();
        // If we've previously consumed some characters of the non-current
        // string, we now account for those characters as part of the current
        // string, not as part of "prior to current string."
        m_numberOfCharactersConsumedPriorToCurrentString -= m_currentString.numberOfCharactersConsumed();
        updateAdvanceFunctionPointers();
    } else {
        m_currentString.clear();
        m_empty = true;
        m_fastPathFlags = NoFastPath;
        m_advanceFunc = &SegmentedString::advanceEmpty;
        m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
    }
}

String* SegmentedString::toString() const
{
    UTF32String result;
    if (m_pushedChar1) {
        result.push_back(m_pushedChar1);
        if (m_pushedChar2)
            result.push_back(m_pushedChar2);
    }
    m_currentString.appendTo(result);
    if (isComposite()) {
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_iterator it = m_substrings.begin();
        std::deque<SegmentedSubstring, gc_allocator<SegmentedSubstring>>::const_iterator e = m_substrings.end();
        for (; it != e; ++it)
            it->appendTo(result);
    }
    return new StringDataUTF32(std::move(result));
}

void SegmentedString::advance(unsigned count, char32_t* consumedCharacters)
{
    STARFISH_ASSERT(count <= length());
    for (unsigned i = 0; i < count; ++i) {
        consumedCharacters[i] = currentChar();
        advance();
    }
}

void SegmentedString::advance8()
{
    STARFISH_ASSERT(!m_pushedChar1);
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar8();
}

void SegmentedString::advance16()
{
    STARFISH_ASSERT(!m_pushedChar1);
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar32();
}

void SegmentedString::advanceAndUpdateLineNumber8()
{
    STARFISH_ASSERT(!m_pushedChar1);
    STARFISH_ASSERT(m_currentString.getCurrentChar() == m_currentChar);
    if (m_currentChar == '\n') {
        ++m_currentLine;
        m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + 1;
    }
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar8();
}

void SegmentedString::advanceAndUpdateLineNumber16()
{
    STARFISH_ASSERT(!m_pushedChar1);
    STARFISH_ASSERT(m_currentString.getCurrentChar() == m_currentChar);
    if (m_currentChar == '\n') {
        ++m_currentLine;
        m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + 1;
    }
    decrementAndCheckLength();
    m_currentChar = m_currentString.incrementAndGetCurrentChar32();
}

void SegmentedString::advanceSlowCase()
{
    if (m_pushedChar1) {
        m_pushedChar1 = m_pushedChar2;
        m_pushedChar2 = 0;

        if (m_pushedChar1) {
            m_currentChar = m_pushedChar1;
            return;
        }

        updateAdvanceFunctionPointers();
    } else if (m_currentString.m_length) {
        if (!--m_currentString.m_length)
            advanceSubstring();
    } else if (!isComposite()) {
        m_currentString.clear();
        m_empty = true;
        m_fastPathFlags = NoFastPath;
        m_advanceFunc = &SegmentedString::advanceEmpty;
        m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
    }
    m_currentChar = m_currentString.m_length ? m_currentString.getCurrentChar() : 0;
}

void SegmentedString::advanceAndUpdateLineNumberSlowCase()
{
    if (m_pushedChar1) {
        m_pushedChar1 = m_pushedChar2;
        m_pushedChar2 = 0;

        if (m_pushedChar1) {
            m_currentChar = m_pushedChar1;
            return;
        }

        updateAdvanceFunctionPointers();
    } else if (m_currentString.m_length) {
        if (m_currentString.getCurrentChar() == '\n' && m_currentString.doNotExcludeLineNumbers()) {
            ++m_currentLine;
            // Plus 1 because numberOfCharactersConsumed value hasn't incremented yet; it does with m_length decrement below.
            m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + 1;
        }
        if (!--m_currentString.m_length)
            advanceSubstring();
        else
            m_currentString.incrementAndGetCurrentChar(); // Only need the ++
    } else if (!isComposite()) {
        m_currentString.clear();
        m_empty = true;
        m_fastPathFlags = NoFastPath;
        m_advanceFunc = &SegmentedString::advanceEmpty;
        m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceEmpty;
    }

    m_currentChar = m_currentString.m_length ? m_currentString.getCurrentChar() : 0;
}

void SegmentedString::advanceEmpty()
{
    STARFISH_ASSERT(!m_currentString.m_length && !isComposite());
    m_currentChar = 0;
}

void SegmentedString::updateSlowCaseFunctionPointers()
{
    m_fastPathFlags = NoFastPath;
    m_advanceFunc = &SegmentedString::advanceSlowCase;
    m_advanceAndUpdateLineNumberFunc = &SegmentedString::advanceAndUpdateLineNumberSlowCase;
}

OrdinalNumber SegmentedString::currentLine() const
{
    return OrdinalNumber::fromZeroBasedInt(m_currentLine);
}

OrdinalNumber SegmentedString::currentColumn() const
{
    int zeroBasedColumn = numberOfCharactersConsumed() - m_numberOfCharactersConsumedPriorToCurrentLine;
    return OrdinalNumber::fromZeroBasedInt(zeroBasedColumn);
}

void SegmentedString::setCurrentPosition(OrdinalNumber line, OrdinalNumber columnAftreProlog, int prologLength)
{
    m_currentLine = line.zeroBasedInt();
    m_numberOfCharactersConsumedPriorToCurrentLine = numberOfCharactersConsumed() + prologLength - columnAftreProlog.zeroBasedInt();
}

}
