#ifndef __StarFishQualifiedName__
#define __StarFishQualifiedName__

#include "util/AtomicString.h"

namespace StarFish {

class QualifiedName : public gc {
    friend class StaticStrings;
    QualifiedName()
        : m_namespaceURI(AtomicString::emptyAtomicString())
        , m_localName(AtomicString::emptyAtomicString())
    {
    }
public:
    QualifiedName(const AtomicString& nsURI, const AtomicString& localName)
        : m_namespaceURI(nsURI)
        , m_localName(localName)
    {
    }

    static bool checkNameProductionRule(String* str, unsigned length);
    bool operator==(const QualifiedName& src) const
    {
        return m_namespaceURI == src.m_namespaceURI && m_localName == src.m_localName;
    }

    AtomicString localNameAtomic() const
    {
        return m_localName;
    }

    AtomicString namespaceURIAtomic() const
    {
        return m_namespaceURI;
    }

    String* localName() const
    {
        return m_localName;
    }

    String* namespaceURI() const
    {
        return m_namespaceURI;
    }

private:
    AtomicString m_namespaceURI;
    AtomicString m_localName;
};

inline bool operator==(const AtomicString& a, const QualifiedName& q) { return a == q.localNameAtomic(); }
inline bool operator!=(const AtomicString& a, const QualifiedName& q) { return a != q.localNameAtomic(); }
inline bool operator==(const QualifiedName& q, const AtomicString& a) { return a == q.localNameAtomic(); }
inline bool operator!=(const QualifiedName& q, const AtomicString& a) { return a != q.localNameAtomic(); }

}
#endif
