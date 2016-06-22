#ifndef __StarFishAtomicHTMLToken__
#define __StarFishAtomicHTMLToken__

#include "dom/parser/HTMLElementLookupTrie.h"
#include "dom/Attribute.h"
// #include "core/html/parser/CompactHTMLToken.h"
#include "dom/parser/HTMLToken.h"

namespace StarFish {

class AtomicHTMLToken : public gc {
public:
    bool forceQuirks() const
    {
        STARFISH_ASSERT(m_type == HTMLToken::DOCTYPE);
        return m_doctypeData->m_forceQuirks;
    }

    StarFish* starFish() { return m_starFish; }

    HTMLToken::Type type() const { return m_type; }

    const AtomicString& name() const
    {
        STARFISH_ASSERT(usesName());
        return m_name;
    }

    void setName(const AtomicString& name)
    {
        STARFISH_ASSERT(usesName());
        m_name = name;
    }

    bool selfClosing() const
    {
        STARFISH_ASSERT(m_type == HTMLToken::StartTag || m_type == HTMLToken::EndTag);
        return m_selfClosing;
    }

    Attribute* getAttributeItem(const QualifiedName& attributeName)
    {
        STARFISH_ASSERT(usesAttributes());
        return findAttributeInVector(m_attributes, attributeName);
    }

    AttributeVector& attributes()
    {
        ASSERT(usesAttributes());
        return m_attributes;
    }

    AttributeVector& attributes() const
    {
        ASSERT(usesAttributes());
        return const_cast<AtomicHTMLToken*>(this)->m_attributes;
    }

    String* characters() const
    {
        ASSERT(m_type == HTMLToken::Character);
        return m_data;
    }

    String* comment() const
    {
        ASSERT(m_type == HTMLToken::Comment);
        return m_data;
    }

    // FIXME: Distinguish between a missing public identifer and an empty one.
    String* publicIdentifier() const
    {
        ASSERT(m_type == HTMLToken::DOCTYPE);
        return m_doctypeData->m_publicIdentifier;
    }

    // FIXME: Distinguish between a missing system identifer and an empty one.
    String* systemIdentifier() const
    {
        ASSERT(m_type == HTMLToken::DOCTYPE);
        return m_doctypeData->m_systemIdentifier;
    }

    explicit AtomicHTMLToken(StarFish* sf, HTMLToken& token)
        : m_starFish(sf)
        , m_type(token.type())
        , m_name(AtomicString::emptyAtomicString())
    {
        m_data = String::emptyString;
        m_doctypeData = nullptr;
        switch (m_type) {
        case HTMLToken::Uninitialized:
            STARFISH_ASSERT_NOT_REACHED();
            break;
        case HTMLToken::DOCTYPE:
            m_name = AtomicString::createAttrAtomicString(m_starFish, new StringDataUTF32(UTF32String(token.name().begin(), token.name().end())));
            m_doctypeData = token.releaseDoctypeData();
            break;
        case HTMLToken::EndOfFile:
            break;
        case HTMLToken::StartTag:
        case HTMLToken::EndTag: {
            m_selfClosing = token.selfClosing();
            QualifiedName tagName = lookupHTMLTag(*sf->staticStrings(), token.name().data(), token.name().size());
            if (tagName.localName()->length())
                m_name = tagName.localNameAtomic();
            else
                m_name = AtomicString::createAttrAtomicString(m_starFish, new StringDataUTF32(UTF32String(token.name().begin(), token.name().end())));
            initializeAttributes(token.attributes());
            break;
        }
        case HTMLToken::Character:
        case HTMLToken::Comment:
            if (token.isAll7BitData())
                m_data = String::createASCIIStringFromUTF32Source(UTF32String(token.data().begin(), token.data().end()));
            else
                m_data = String::createASCIIStringFromUTF32SourceIfPossible(UTF32String(token.data().begin(), token.data().end()));
            break;
        }
    }
/*
    explicit AtomicHTMLToken(const CompactHTMLToken& token)
        : m_type(token.type())
    {
        m_name = String::emptyString;
        m_doctypeData = nullptr;
        switch (m_type) {
        case HTMLToken::Uninitialized:
            ASSERT_NOT_REACHED();
            break;
        case HTMLToken::DOCTYPE:
            m_name = AtomicString(token.data());
            m_doctypeData = adoptPtr(new DoctypeData());
            m_doctypeData->m_hasPublicIdentifier = true;
            append(m_doctypeData->m_publicIdentifier, token.publicIdentifier());
            m_doctypeData->m_hasSystemIdentifier = true;
            append(m_doctypeData->m_systemIdentifier, token.systemIdentifier());
            m_doctypeData->m_forceQuirks = token.doctypeForcesQuirks();
            break;
        case HTMLToken::EndOfFile:
            break;
        case HTMLToken::StartTag:
            m_attributes.reserveInitialCapacity(token.attributes().size());
            for (Vector<CompactHTMLToken::Attribute>::const_iterator it = token.attributes().begin(); it != token.attributes().end(); ++it) {
                QualifiedName name(nullAtom, AtomicString(it->name), nullAtom);
                // FIXME: This is N^2 for the number of attributes.
                if (!findAttributeInVector(m_attributes, name))
                    m_attributes.append(Attribute(name, AtomicString(it->value)));
            }
            // Fall through!
        case HTMLToken::EndTag:
            m_selfClosing = token.selfClosing();
            m_name = AtomicString(token.data());
            break;
        case HTMLToken::Character:
        case HTMLToken::Comment:
            m_data = token.data();
            break;
        }
    }
*/
    explicit AtomicHTMLToken(StarFish* sf, HTMLToken::Type type)
        : m_starFish(sf)
        , m_type(type)
        , m_name(AtomicString::emptyAtomicString())
        , m_selfClosing(false)
    {
        m_data = String::emptyString;
        m_doctypeData = nullptr;
    }

    AtomicHTMLToken(StarFish* sf, HTMLToken::Type type, AtomicString name, const AttributeVector& attributes = AttributeVector())
        : m_starFish(sf)
        , m_type(type)
        , m_name(name)
        , m_selfClosing(false)
        , m_attributes(attributes)
    {
        m_data = String::emptyString;
        m_doctypeData = nullptr;
        ASSERT(usesName());
    }

private:
    StarFish* m_starFish;
    HTMLToken::Type m_type;

    void initializeAttributes(const HTMLToken::AttributeList& attributes);
    QualifiedName nameForAttribute(const HTMLToken::Attribute&) const;

    bool usesName() const;

    bool usesAttributes() const;

    // "name" for DOCTYPE, StartTag, and EndTag
    AtomicString m_name;

    // "data" for Comment, "characters" for Character
    String* m_data;

    // For DOCTYPE
    DoctypeData* m_doctypeData;

    // For StartTag and EndTag
    bool m_selfClosing;

    AttributeVector m_attributes;
};

inline void AtomicHTMLToken::initializeAttributes(const HTMLToken::AttributeList& attributes)
{
    size_t size = attributes.size();
    if (!size)
        return;

    m_attributes.clear();
    // m_attributes.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i) {
        const HTMLToken::Attribute& attribute = attributes[i];
        if (attribute.name.size() == 0)
            continue;

        // FIXME: We should be able to add the following ASSERT once we fix
        // https://bugs.webkit.org/show_bug.cgi?id=62971
        //   ASSERT(attribute.nameRange.start);
        ASSERT(attribute.nameRange.end);
        ASSERT(attribute.valueRange.start);
        ASSERT(attribute.valueRange.end);

        // AtomicString value(attribute.value);
        const QualifiedName& name = nameForAttribute(attribute);
        // FIXME: This is N^2 for the number of attributes.
        if (!findAttributeInVector(m_attributes, name))
            m_attributes.push_back(Attribute(name, String::createASCIIStringFromUTF32SourceIfPossible(attribute.value)));
    }
}

}

#endif
