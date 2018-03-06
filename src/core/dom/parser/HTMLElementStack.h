/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef __StarFishHTMLElementStack__
#define __StarFishHTMLElementStack__

namespace StarFish {

class DocumentFragment;
class Element;
class Node;
class HTMLStackItem;

// NOTE: The HTML5 spec uses a backwards (grows downward) stack. We're using
// more standard (grows upwards) stack terminology here.
class HTMLElementStack : public gc {
public:
    HTMLElementStack();
    ~HTMLElementStack();

    class ElementRecord : public gc {
    public:
        ~ElementRecord();
        Element* element() const;
        Node* node() const;
        HTMLStackItem* stackItem() const
        {
            return m_item;
        }
        void replaceElement(HTMLStackItem*);
        bool isAbove(ElementRecord*) const;
        ElementRecord* next() const
        {
            return m_next;
        }

    private:
        friend class HTMLElementStack;
        ElementRecord(HTMLStackItem*, ElementRecord*);
        ElementRecord* releaseNext()
        {
            ElementRecord* ret = m_next;
            m_next = nullptr;
            return ret;
        }
        void setNext(ElementRecord* next)
        {
            m_next = next;
        }

        HTMLStackItem* m_item;
        ElementRecord* m_next;
    };

    unsigned stackDepth() const
    {
        return m_stackDepth;
    }

    // Inlining this function is a (small) performance win on the parsing
    // benchmark.
    Element* top() const;
    Node* topNode() const;
    HTMLStackItem* topStackItem() const;
    HTMLStackItem* oneBelowTop() const;
    ElementRecord* topRecord() const;
    ElementRecord* find(Element*) const;
    ElementRecord* furthestBlockForFormattingElement(Element*) const;
    ElementRecord* topmost(const AtomicString& tagName) const;

    void insertAbove(HTMLStackItem*, ElementRecord*);

    void push(HTMLStackItem*);
    void pushRootNode(HTMLStackItem*);
    void pushHTMLHtmlElement(HTMLStackItem*);
    void pushHTMLHeadElement(HTMLStackItem*);
    void pushHTMLBodyElement(HTMLStackItem*);

    void pop();
    void popUntil(const AtomicString& tagName);
    void popUntil(Element*);
    void popUntilPopped(const AtomicString& tagName);
    void popUntilPopped(const QualifiedName& tagName)
    {
        popUntilPopped(tagName.localNameAtomic());
    }

    void popUntilPopped(Element*);
    void popUntilNumberedHeaderElementPopped();
    void popUntilTableScopeMarker();     // "clear the stack back to a table
                                         // context" in the spec.
    void popUntilTableBodyScopeMarker(); // "clear the stack back to a table
                                         // body context" in the spec.
    void popUntilTableRowScopeMarker();  // "clear the stack back to a table row
                                         // context" in the spec.
    void popUntilForeignContentScopeMarker();
    void popHTMLHeadElement();
    void popHTMLBodyElement();
    void popAll();

    static bool isMathMLTextIntegrationPoint(HTMLStackItem*);
    static bool isHTMLIntegrationPoint(HTMLStackItem*);

    void remove(Element*);
    void removeHTMLHeadElement(Element*);

    bool contains(Element*) const;
    bool contains(const AtomicString& tagName) const;

    bool inScope(Element*) const;
    bool inScope(const AtomicString& tagName) const;
    bool inScope(const QualifiedName&) const;
    bool inListItemScope(const AtomicString& tagName) const;
    bool inListItemScope(const QualifiedName&) const;
    bool inTableScope(const AtomicString& tagName) const;
    bool inTableScope(const QualifiedName&) const;
    bool inButtonScope(const AtomicString& tagName) const;
    bool inButtonScope(const QualifiedName&) const;
    bool inSelectScope(const AtomicString& tagName) const;
    bool inSelectScope(const QualifiedName&) const;

    bool hasNumberedHeaderElementInScope() const;

    bool hasOnlyOneElement() const;
    bool secondElementIsHTMLBodyElement() const;
    bool hasTemplateInHTMLScope() const;
    Element* htmlElement() const;
    Element* headElement() const;
    Element* bodyElement() const;
    Node* rootNode() const;

#ifndef NDEBUG
    void show();
#endif

private:
    void pushCommon(HTMLStackItem*);
    void pushRootNodeCommon(HTMLStackItem*);
    void popCommon();
    void removeNonTopCommon(Element*);

    ElementRecord* m_top;

    // We remember the root node, <head> and <body> as they are pushed. Their
    // ElementRecords keep them alive. The root node is never popped.
    // FIXME: We don't currently require type-specific information about
    // these elements so we haven't yet bothered to plumb the types all the
    // way down through createElement, etc.
    Node* m_rootNode;
    Element* m_headElement;
    Element* m_bodyElement;
    unsigned m_stackDepth;
};
}

#endif
