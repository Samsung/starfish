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

#ifndef __StarFishHTMLElementStack__
#define __StarFishHTMLElementStack__

#include "dom/parser/HTMLStackItem.h"

namespace StarFish {

class DocumentFragment;
class Element;
class QualifiedName;

// NOTE: The HTML5 spec uses a backwards (grows downward) stack. We're using
// more standard (grows upwards) stack terminology here.
class HTMLElementStack : public gc {
public:
    HTMLElementStack();
    ~HTMLElementStack();

    class ElementRecord : public gc {
    public:
        ~ElementRecord();
        Element* element() const { return m_item->element(); }
        Node* node() const { return m_item->node(); }
        HTMLStackItem* stackItem() const { return m_item; }
        void replaceElement(HTMLStackItem*);
        bool isAbove(ElementRecord*) const;
        ElementRecord* next() const { return m_next; }
    private:
        friend class HTMLElementStack;
        ElementRecord(HTMLStackItem*, ElementRecord*);
        ElementRecord* releaseNext()
        {
            ElementRecord* ret = m_next;
            m_next = nullptr;
            return ret;
        }
        void setNext(ElementRecord* next) { m_next = next; }

        HTMLStackItem* m_item;
        ElementRecord* m_next;
    };

    unsigned stackDepth() const { return m_stackDepth; }

    // Inlining this function is a (small) performance win on the parsing
    // benchmark.
    Element* top() const
    {
        STARFISH_ASSERT(m_top->element());
        return m_top->element();
    }

    Node* topNode() const
    {
        ASSERT(m_top->node());
        return m_top->node();
    }

    HTMLStackItem* topStackItem() const
    {
        ASSERT(m_top->stackItem());
        return m_top->stackItem();
    }

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
    void popUntilPopped(const QualifiedName& tagName) { popUntilPopped(tagName.localNameAtomic()); }

    void popUntilPopped(Element*);
    void popUntilNumberedHeaderElementPopped();
    void popUntilTableScopeMarker(); // "clear the stack back to a table context" in the spec.
    void popUntilTableBodyScopeMarker(); // "clear the stack back to a table body context" in the spec.
    void popUntilTableRowScopeMarker(); // "clear the stack back to a table row context" in the spec.
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
