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

#ifndef __StarFishAttr__
#define __StarFishAttr__

#include "core/dom/Node.h"

namespace StarFish {

class Element;
class Attr : public Node {
public:
    Attr(Document* document, Element* element, QualifiedName name)
        : Node(document)
        , m_element(element)
        , m_name(name)
        , m_standAloneValue(String::emptyString)
    {
    }

    Attr(Document* document, QualifiedName name)
        : Node(document)
        , m_element(nullptr)
        , m_name(name)
        , m_standAloneValue(String::emptyString)
    {
    }

    Attr(Document* document, QualifiedName name, String* value)
        : Node(document)
        , m_element(nullptr)
        , m_name(name)
        , m_standAloneValue(value)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isAttr() const override;

    String* name() const
    {
        return m_name.toString();
    }

    QualifiedName qname()
    {
        return m_name;
    }

    Nullable<String*> namespaceURI()
    {
        auto v = m_name.namespaceURI();
        if (v.hasValue() && !v.getValue().isEmptyAtomicString()) {
            return v.getValue().string();
        } else {
            return Nullable<String*>();
        }
    }

    Nullable<String*> prefix() override
    {
        auto v = m_name.prefix();
        if (v.hasValue()) {
            return v.getValue().string();
        } else {
            return Nullable<String*>();
        }
    }

    String* value() const;

    void setValue(String* value);

    Element* ownerElement()
    {
        return m_element;
    }

    bool specified() const
    {
        return true;
    }

    /* 4.4 Interface Node */

    virtual NodeType nodeType() const override
    {
        return ATTRIBUTE_NODE;
    }

    virtual String* nodeName() override
    {
        return name();
    }

    virtual String* localName() override
    {
        return m_name.localName();
    }

    virtual Node* clone() override
    {
        return new Attr(document(), m_name, value());
    }

    void detachFromElement(String* value)
    {
        STARFISH_ASSERT(m_element);
        m_standAloneValue = value;
        m_element = nullptr;
    }

    void attachToElement(Element* element, String* attachedLocalName)
    {
        m_element = element;
        m_standAloneValue = attachedLocalName;
    }

    bool isContainerNode() override
    {
        return false;
    }

    bool operator==(const Attr& attr);
    bool operator!=(const Attr& attr);

private:
    Element* m_element;
    QualifiedName m_name;
    String* m_standAloneValue;
};
}

#endif
