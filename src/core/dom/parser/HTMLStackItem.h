/*
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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

#ifndef __StarFishHTMLStackItem__
#define __StarFishHTMLStackItem__

#include "StarFish.h"
#include "core/dom/Node.h"
#include "core/dom/Attribute.h"

namespace StarFish {

class AtomicHTMLToken;
class Element;
class Node;

class HTMLStackItem : public gc {
public:
    enum ItemType { ItemForContextElement, ItemForDocumentFragmentNode };

    HTMLStackItem(Node* node, ItemType type);

    HTMLStackItem(Node* node, AtomicHTMLToken* token,
                  const AtomicString& namespaceURI);

    Element* element() const;
    Node* node() const
    {
        return m_node;
    }

    bool isDocumentFragmentNode() const
    {
        return m_isDocumentFragmentNode;
    }
    bool isElementNode() const
    {
        return !m_isDocumentFragmentNode;
    }

    const AtomicString& namespaceURI() const
    {
        return m_namespaceURI;
    }
    const AtomicString& localName() const
    {
        return m_tokenLocalName;
    }

    const GCVector<Attribute>& attributes() const
    {
        STARFISH_ASSERT(m_tokenLocalName.string()->length());
        return m_tokenAttributes;
    }
    Attribute* getAttributeItem(const QualifiedName& attributeName)
    {
        STARFISH_ASSERT(m_tokenLocalName);
        return findAttributeInVector(m_tokenAttributes, attributeName);
    }
    bool hasLocalName(const AtomicString& name) const
    {
        return m_tokenLocalName == name;
    }
    bool hasTagName(const QualifiedName& name) const
    {
        return m_tokenLocalName == name.localNameAtomic() &&
               m_namespaceURI == name.namespaceURI();
    }

    bool matchesHTMLTag(const AtomicString& name) const;
    bool matchesHTMLTag(const QualifiedName& name) const;

    bool causesFosterParenting();

    bool isInHTMLNamespace() const
    {
        // A DocumentFragment takes the place of the document element when
        // parsing // fragments and should be considered in the HTML namespace.
        //
        // FIXME: Does this also apply to ShadowRoot?
        return namespaceURI() ==
                   m_node->starFish()->staticStrings()->m_xhtmlNamespaceURI ||
               isDocumentFragmentNode();
        return true;
    }

    bool isNumberedHeaderElement() const;

    bool isTableBodyContextElement() const;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/
    //        parsing.html#special
    bool isSpecialNode() const;

private:
    Node* m_node;
    const StaticStrings& staticStrings() const;

    AtomicString m_tokenLocalName;
    GCVector<Attribute> m_tokenAttributes;
    AtomicString m_namespaceURI;
    bool m_isDocumentFragmentNode;
};
}

#endif
