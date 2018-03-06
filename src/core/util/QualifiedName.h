/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishQualifiedName__
#define __StarFishQualifiedName__

#include "core/util/AtomicString.h"

namespace StarFish {

class QualifiedName : public gc {
    friend class StaticStrings;
    QualifiedName()
        : m_prefix(nullptr)
        , m_namespaceURI(AtomicString::emptyAtomicString())
        , m_localName(AtomicString::emptyAtomicString())
    {
    }

public:
    QualifiedName(const AtomicString& localName)
        : m_prefix(nullptr)
        , m_namespaceURI(nullptr)
        , m_localName(localName)
    {
    }

    QualifiedName(const AtomicString& nsURI, const AtomicString& localName)
        : m_prefix(nullptr)
        , m_namespaceURI(nsURI)
        , m_localName(localName)
    {
    }

    QualifiedName(const AtomicString& prefix, const AtomicString& nsURI,
                  const AtomicString& localName)
        : m_prefix(prefix)
        , m_namespaceURI(nsURI)
        , m_localName(localName)
    {
    }

    static bool checkNameProductionRule(String* str);
    bool operator==(const QualifiedName& src) const
    {
        return m_prefix == src.m_prefix &&
               m_namespaceURI == src.m_namespaceURI &&
               m_localName == src.m_localName;
    }

    bool operator!=(const QualifiedName& src) const
    {
        return !operator==(src);
    }

    bool equalsLocalName(const QualifiedName& src) const
    {
        return m_localName == src.m_localName;
    }

    bool equalsNamespace(const QualifiedName& src) const
    {
        return m_namespaceURI == src.m_namespaceURI;
    }

    AtomicString localNameAtomic() const
    {
        return m_localName;
    }

    String* localName() const
    {
        return m_localName;
    }

    bool hasPrefix() const
    {
        return m_prefix.string() != nullptr;
    }

    Nullable<AtomicString> prefix() const
    {
        if (m_prefix.string() == nullptr) {
            return Nullable<AtomicString>();
        }
        return m_prefix;
    }

    Nullable<String*> prefixString() const
    {
        if (m_prefix.string() == nullptr) {
            return Nullable<String*>();
        }
        return m_prefix.string();
    }

    void copyPrefixFrom(const QualifiedName& other)
    {
        m_prefix = other.m_prefix;
    }

    Nullable<AtomicString> namespaceURI() const
    {
        if (m_namespaceURI.string() == nullptr) {
            return Nullable<AtomicString>();
        }
        return m_namespaceURI;
    }

    bool hasSameNamespaceURI(const char* str) const
    {
        if (m_namespaceURI.string()) {
            return m_namespaceURI.string()->equals(str);
        }
        return false;
    }

    bool hasSameNamespaceURI(String* str) const
    {
        if (m_namespaceURI.string()) {
            return m_namespaceURI.string()->equals(str);
        }
        return false;
    }

    bool hasSameNamespaceURI(Nullable<String*> str) const
    {
        if (m_namespaceURI.string()) {
            if (str.hasValue()) {
                return m_namespaceURI.string()->equals(str.getValue());
            } else {
                return false;
            }
        } else {
            if (str.hasValue()) {
                return false;
            }
            return true;
        }
    }

    String* toString() const
    {
        if (!prefix().hasValue())
            return localName();

        StringBuilder sb;
        sb.appendString(prefix().getValue().string());
        sb.appendChar(':');
        sb.appendString(localName());
        return sb.finalize();
    }

private:
    AtomicString m_prefix;
    AtomicString m_namespaceURI;
    AtomicString m_localName;
};

inline bool operator==(const AtomicString& a, const QualifiedName& q)
{
    return a == q.localNameAtomic();
}
inline bool operator!=(const AtomicString& a, const QualifiedName& q)
{
    return a != q.localNameAtomic();
}
inline bool operator==(const QualifiedName& q, const AtomicString& a)
{
    return a == q.localNameAtomic();
}
inline bool operator!=(const QualifiedName& q, const AtomicString& a)
{
    return a != q.localNameAtomic();
}
}
#endif
