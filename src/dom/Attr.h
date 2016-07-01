/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class Element;
class Attr : public Node {
public:
    Attr(Document* document, ScriptBindingInstance* instance, Element* element, QualifiedName name)
        : Node(document), m_element(element), m_name(name), m_standAloneValue(String::emptyString)
    {
    }

    Attr(Document* document, ScriptBindingInstance* instance, QualifiedName name)
        : Node(document), m_element(nullptr), m_name(name), m_standAloneValue(String::emptyString)
    {
    }

    Attr(Document* document, ScriptBindingInstance* instance, QualifiedName name, String* value)
        : Node(document), m_element(nullptr), m_name(name), m_standAloneValue(value)
    {

    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    QualifiedName name()
    {
        return m_name;
    }

    String* value()
    {
        if (m_element)
            return m_element->getAttribute(m_name);
        return m_standAloneValue;
    }

    void setValue(String* value)
    {
        if (m_element)
            m_element->setAttribute(m_name, value);
        else
            m_standAloneValue = value;
    }

    Element* ownerElement() { return m_element; }

    /* 4.4 Interface Node */

    virtual NodeType nodeType()
    {
        return ATTRIBUTE_NODE;
    }

    virtual String* nodeName()
    {
        return m_name.localName();
    }

    virtual String* localName()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    virtual void setNodeValue(String* val) { }

    virtual String* textContent()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    virtual void setTextContent(String* val) { }

    virtual Node* clone()
    {
        return (Node*)(new Attr(document(), document()->scriptBindingInstance(), m_name, value()));
    }

    virtual bool isAttr() const
    {
        return true;
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
