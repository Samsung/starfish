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
        , m_qname(name)
        , m_standAloneValue(String::emptyString)
    {
    }

    Attr(Document* document, QualifiedName name)
        : Node(document)
        , m_element(nullptr)
        , m_qname(name)
        , m_standAloneValue(String::emptyString)
    {
    }

    Attr(Document* document, QualifiedName name, String* value)
        : Node(document)
        , m_element(nullptr)
        , m_qname(name)
        , m_standAloneValue(value)
    {
    }

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isAttr() const override;

    QualifiedName qname() const
    {
        return m_qname;
    }

    String* name() const
    {
        // FIXME: If we support legacy xml, then we have to implement this
        // to return with namespace
        return m_qname.localName();
    }

    String* value() const;

    void setValue(String* value);

    Element* ownerElement() const
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
        return m_qname.localName();
    }

    virtual Node* clone() override
    {
        return new Attr(document(), m_qname, value());
    }

    void detachFromElement(String* value)
    {
        m_standAloneValue = value;
        m_element = nullptr;
    }

private:
    Element* m_element;
    QualifiedName m_qname;
    String* m_standAloneValue;
};
}

#endif
