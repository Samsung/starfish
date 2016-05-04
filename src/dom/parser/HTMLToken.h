#ifndef _StarFishHTMLToken_
#define _StarFishHTMLToken_

#include "util/String.h"

namespace StarFish {

class DoctypeData : public gc {
public:
    DoctypeData()
        : m_hasPublicIdentifier(false)
        , m_hasSystemIdentifier(false)
        , m_publicIdentifier(String::emptyString)
        , m_systemIdentifier(String::emptyString)
        , m_forceQuirks(false)
    {
    }

    bool m_hasPublicIdentifier;
    bool m_hasSystemIdentifier;
    String* m_publicIdentifier;
    String* m_systemIdentifier;
    bool m_forceQuirks;
};

/*
static inline Attribute* findAttributeInVector(Vector<Attribute>& attributes, const QualifiedName& name)
{
    for (unsigned i = 0; i < attributes.size(); ++i) {
        if (attributes.at(i).name().matches(name))
            return &attributes.at(i);
    }
    return 0;
}
*/

class HTMLToken : public gc {
public:
    enum Type {
        Uninitialized,
        DOCTYPE,
        StartTag,
        EndTag,
        Comment,
        Character,
        EndOfFile,
    };

    class Attribute {
    public:
        Attribute()
        {
        }
        class Range {
        public:
            int start;
            int end;
        };

        Range nameRange;
        Range valueRange;
        UTF32String name;
        UTF32String value;
    };

    typedef std::vector<Attribute, gc_allocator<Attribute>> AttributeList;

    // By using an inline capacity of 256, we avoid spilling over into an malloced buffer
    // approximately 99% of the time based on a non-scientific browse around a number of
    // popular web sites on 23 May 2013.
    typedef std::vector<char32_t, gc_allocator<char32_t>> DataVector;

    HTMLToken()
    {
        m_doctypeData = nullptr;
        clear();
    }

    void clear()
    {
        m_type = Uninitialized;
        m_range.start = 0;
        m_range.end = 0;
        m_baseOffset = 0;
        m_data.clear();
        m_orAllData = 0;
    }

    bool isUninitialized() { return m_type == Uninitialized; }
    Type type() const { return m_type; }

    void makeEndOfFile()
    {
        STARFISH_ASSERT(m_type == Uninitialized);
        m_type = EndOfFile;
    }

    /* Range and offset methods exposed for HTMLSourceTracker and HTMLViewSourceParser */
    int startIndex() const { return m_range.start; }
    int endIndex() const { return m_range.end; }

    void setBaseOffset(int offset)
    {
        m_baseOffset = offset;
    }

    void end(int endOffset)
    {
        m_range.end = endOffset - m_baseOffset;
    }

    const DataVector& data() const
    {
        STARFISH_ASSERT(m_type == Character || m_type == Comment || m_type == StartTag || m_type == EndTag);
        return m_data;
    }

    bool isAll7BitData() const
    {
        return (m_orAllData < 128);
    }

    const DataVector& name() const
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag || m_type == DOCTYPE);
        return m_data;
    }

    void appendToName(char32_t character)
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag || m_type == DOCTYPE);
        STARFISH_ASSERT(character);
        m_data.push_back(character);
        m_orAllData |= character;
    }

    /* DOCTYPE Tokens */

    bool forceQuirks() const
    {
        STARFISH_ASSERT(m_type == DOCTYPE);
        return m_doctypeData->m_forceQuirks;
    }

    void setForceQuirks()
    {
        STARFISH_ASSERT(m_type == DOCTYPE);
        m_doctypeData->m_forceQuirks = true;
    }

    void beginDOCTYPE()
    {
        STARFISH_ASSERT(m_type == Uninitialized);
        m_type = DOCTYPE;
        m_doctypeData = new DoctypeData;
    }

    void beginDOCTYPE(char32_t character)
    {
        STARFISH_ASSERT(character);
        beginDOCTYPE();
        m_data.push_back(character);
        m_orAllData |= character;
    }

    String* publicIdentifier() const
    {
        STARFISH_ASSERT(m_type == DOCTYPE);
        return m_doctypeData->m_publicIdentifier;
    }

    String* systemIdentifier() const
    {
        STARFISH_ASSERT(m_type == DOCTYPE);
        return m_doctypeData->m_systemIdentifier;
    }

    void setPublicIdentifierToEmptyString()
    {
        STARFISH_ASSERT(m_type == DOCTYPE);
        m_doctypeData->m_hasPublicIdentifier = true;
        m_doctypeData->m_publicIdentifier = String::emptyString;
    }

    void setSystemIdentifierToEmptyString()
    {
        STARFISH_ASSERT(m_type == DOCTYPE);
        m_doctypeData->m_hasSystemIdentifier = true;
        m_doctypeData->m_systemIdentifier = String::emptyString;
    }

    void appendToPublicIdentifier(char32_t character)
    {
        STARFISH_ASSERT(character);
        STARFISH_ASSERT(m_type == DOCTYPE);
        STARFISH_ASSERT(m_doctypeData->m_hasPublicIdentifier);
        m_doctypeData->m_publicIdentifier = m_doctypeData->m_publicIdentifier->concat(String::createUTF32String(character));
    }

    void appendToSystemIdentifier(char32_t character)
    {
        STARFISH_ASSERT(character);
        STARFISH_ASSERT(m_type == DOCTYPE);
        STARFISH_ASSERT(m_doctypeData->m_hasSystemIdentifier);
        m_doctypeData->m_systemIdentifier = m_doctypeData->m_systemIdentifier->concat(String::createUTF32String(character));
    }

    DoctypeData* releaseDoctypeData()
    {
        auto data = m_doctypeData;
        m_doctypeData = nullptr;
        return data;
    }

    /* Start/End Tag Tokens */

    bool selfClosing() const
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        return m_selfClosing;
    }

    void setSelfClosing()
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        m_selfClosing = true;
    }

    void beginStartTag(char32_t character)
    {
        STARFISH_ASSERT(character);
        STARFISH_ASSERT(m_type == Uninitialized);
        m_type = StartTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.push_back(character);
        m_orAllData |= character;
    }

    void beginEndTag(char32_t character)
    {
        STARFISH_ASSERT(m_type == Uninitialized);
        m_type = EndTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.push_back(character);
    }

    void beginEndTag(const std::vector<char, gc_allocator<char>>& characters)
    {
        STARFISH_ASSERT(m_type == Uninitialized);
        m_type = EndTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        for (size_t i = 0; i < characters.size(); i++) {
            m_data.push_back(characters[i]);
        }
    }

    void addNewAttribute()
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        m_attributes.resize(m_attributes.size() + 1);
        m_currentAttribute = &m_attributes.back();
#ifndef NDEBUG
        m_currentAttribute->nameRange.start = 0;
        m_currentAttribute->nameRange.end = 0;
        m_currentAttribute->valueRange.start = 0;
        m_currentAttribute->valueRange.end = 0;
#endif
    }

    void beginAttributeName(int offset)
    {
        m_currentAttribute->nameRange.start = offset - m_baseOffset;
    }

    void endAttributeName(int offset)
    {
        int index = offset - m_baseOffset;
        m_currentAttribute->nameRange.end = index;
        m_currentAttribute->valueRange.start = index;
        m_currentAttribute->valueRange.end = index;
    }

    void beginAttributeValue(int offset)
    {
        m_currentAttribute->valueRange.start = offset - m_baseOffset;
#ifndef NDEBUG
        m_currentAttribute->valueRange.end = 0;
#endif
    }

    void endAttributeValue(int offset)
    {
        m_currentAttribute->valueRange.end = offset - m_baseOffset;
    }

    void appendToAttributeName(char32_t character)
    {
        STARFISH_ASSERT(character);
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        m_currentAttribute->name.push_back(character);
    }

    void appendToAttributeValue(char32_t character)
    {
        STARFISH_ASSERT(character);
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        STARFISH_ASSERT(m_currentAttribute->valueRange.start);
        m_currentAttribute->value.push_back(character);
    }

    void appendToAttributeValue(size_t i, String* value)
    {
        STARFISH_ASSERT(value->length() != 0);
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        for (size_t j = 0; j < value->length(); j++) {
            m_attributes[i].value.push_back(value->charAt(j));
        }
    }

    const AttributeList& attributes() const
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        return m_attributes;
    }

    const Attribute* getAttributeItem(const QualifiedName& name) const
    {
        for (unsigned i = 0; i < m_attributes.size(); ++i) {
            // if (AtomicString(m_attributes.at(i).name) == name.localName())
            if (name.string()->equals(m_attributes.at(i).name.data()))
                return &m_attributes.at(i);
        }
        return 0;
    }

    // Used by the XSSAuditor to nuke XSS-laden attributes.
    void eraseValueOfAttribute(size_t i)
    {
        STARFISH_ASSERT(m_type == StartTag || m_type == EndTag);
        m_attributes[i].value.clear();
    }

    /* Character Tokens */

    // Starting a character token works slightly differently than starting
    // other types of tokens because we want to save a per-character branch.
    void ensureIsCharacterToken()
    {
        STARFISH_ASSERT(m_type == Uninitialized || m_type == Character);
        m_type = Character;
    }

    const DataVector& characters() const
    {
        STARFISH_ASSERT(m_type == Character);
        return m_data;
    }

    void appendToCharacter(char character)
    {
        STARFISH_ASSERT(m_type == Character);
        m_data.push_back(character);
    }

    void appendToCharacter(char32_t character)
    {
        STARFISH_ASSERT(m_type == Character);
        m_data.push_back(character);
        m_orAllData |= character;
    }

    void appendToCharacter(const std::vector<char, gc_allocator<char>>& characters)
    {
        STARFISH_ASSERT(m_type == Character);


        for (size_t i = 0; i < characters.size(); i++) {
            m_data.push_back(characters[i]);
        }
    }

    /* Comment Tokens */

    const DataVector& comment() const
    {
        STARFISH_ASSERT(m_type == Comment);
        return m_data;
    }

    void beginComment()
    {
        STARFISH_ASSERT(m_type == Uninitialized);
        m_type = Comment;
    }

    void appendToComment(char32_t character)
    {
        STARFISH_ASSERT(character);
        STARFISH_ASSERT(m_type == Comment);
        m_data.push_back(character);
        m_orAllData |= character;
    }

    void eraseCharacters()
    {
        STARFISH_ASSERT(m_type == Character);
        m_data.clear();
        m_orAllData = 0;
    }

private:
    Type m_type;
    Attribute::Range m_range; // Always starts at zero.
    int m_baseOffset;
    DataVector m_data;
    char32_t m_orAllData;

    // For StartTag and EndTag
    bool m_selfClosing;
    AttributeList m_attributes;

    // A pointer into m_attributes used during lexing.
    Attribute* m_currentAttribute;

    // For DOCTYPE
    DoctypeData* m_doctypeData;
};

}

#endif
