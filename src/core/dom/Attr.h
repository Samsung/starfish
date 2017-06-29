/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
        auto v = m_name.prefix();
        if (v.hasValue()) {
            return v.getValue().string();
        } else {
            return Nullable<String*>();
        }
    }

    Nullable<String*> prefix()
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

    virtual String* nodeName()
    {
        return name();
    }

    virtual String* localName()
    {
        return m_name.localName();
    }

    virtual Node* clone() override
    {
        return new Attr(document(), m_name, value());
    }

    void detachFromElement(String* value)
    {
        m_standAloneValue = value;
        m_element = nullptr;
    }

private:
    Element* m_element;
    QualifiedName m_name;
    String* m_standAloneValue;
};
}

#endif
