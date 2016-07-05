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

#ifndef __StarFishHTMLConstructionSite__
#define __StarFishHTMLConstructionSite__

#include "dom/parser/HTMLElementStack.h"
#include "dom/parser/HTMLFormattingElementList.h"
#include "dom/Document.h"

namespace StarFish {

struct HTMLConstructionSiteTask {
    enum Operation {
        Insert,
        InsertText, // Handles possible merging of text nodes.
        InsertAlreadyParsedChild, // Insert w/o calling begin/end parsing.
        Reparent,
        TakeAllChildren,
    };

    explicit HTMLConstructionSiteTask(Operation op)
        : operation(op)
        , parent(nullptr)
        , nextChild(nullptr)
        , child(nullptr)
        , selfClosing(false)
    {
    }

    Node* oldParent()
    {
        // It's sort of ugly, but we store the |oldParent| in the |child| field
        // of the task so that we don't bloat the HTMLConstructionSiteTask
        // object in the common case of the Insert operation.
        return child;
    }

    Operation operation;
    Node* parent;
    Node* nextChild;
    Node* child;
    bool selfClosing;
};

// Note: These are intentionally ordered so that when we concatonate
// strings and whitespaces the resulting whitespace is ws = min(ws1, ws2).
enum WhitespaceMode {
    WhitespaceUnknown,
    NotAllWhitespace,
    AllWhitespace,
};

class AtomicHTMLToken;
class Document;
class Element;
typedef HTMLElement HTMLFormElement;

class HTMLConstructionSite : public gc {
public:
    HTMLConstructionSite(Document*);
    HTMLConstructionSite(DocumentFragment*);
    ~HTMLConstructionSite();

    void detach();

    // executeQueuedTasks empties the queue but does not flush pending text.
    // NOTE: Possible reentrancy via JavaScript execution.
    void executeQueuedTasks();

    // flushPendingText turns pending text into queued Text insertions, but does not execute them.
    void flushPendingText();

    // Called before every token in HTMLTreeBuilder::processToken, thus inlined:
    void flush()
    {
        if (!hasPendingTasks())
            return;
        flushPendingText();
        executeQueuedTasks(); // NOTE: Possible reentrancy via JavaScript execution.
        ASSERT(!hasPendingTasks());
    }

    bool hasPendingTasks()
    {
        return !m_pendingText.isEmpty() || !(m_taskQueue.size() == 0);
    }

    void setDefaultCompatibilityMode();
    void processEndOfFile();
    void finishedParsing();

    void insertDoctype(AtomicHTMLToken*);
    void insertComment(AtomicHTMLToken*);
    void insertCommentOnDocument(AtomicHTMLToken*);
    void insertCommentOnHTMLHtmlElement(AtomicHTMLToken*);
    void insertHTMLElement(AtomicHTMLToken*);
    void insertSelfClosingHTMLElement(AtomicHTMLToken*);
    void insertFormattingElement(AtomicHTMLToken*);
    void insertHTMLHeadElement(AtomicHTMLToken*);
    void insertHTMLBodyElement(AtomicHTMLToken*);
    void insertHTMLFormElement(AtomicHTMLToken*, bool isDemoted = false);
    void insertScriptElement(AtomicHTMLToken*);
    void insertTextNode(String*, WhitespaceMode = WhitespaceUnknown);
    void insertForeignElement(AtomicHTMLToken* token, const AtomicString& namespaceURI);

    void insertHTMLHtmlStartTagBeforeHTML(AtomicHTMLToken*);
    void insertHTMLHtmlStartTagInBody(AtomicHTMLToken*);
    void insertHTMLBodyStartTagInBody(AtomicHTMLToken*);

    void reparent(HTMLElementStack::ElementRecord* newParent, HTMLElementStack::ElementRecord* child);
    void reparent(HTMLElementStack::ElementRecord* newParent, HTMLStackItem* child);
    // insertAlreadyParsedChild assumes that |child| has already been parsed (i.e., we're just
    // moving it around in the tree rather than parsing it for the first time). That means
    // this function doesn't call beginParsingChildren / finishParsingChildren.
    void insertAlreadyParsedChild(HTMLStackItem* newParent, HTMLElementStack::ElementRecord* child);
    void takeAllChildren(HTMLStackItem* newParent, HTMLElementStack::ElementRecord* oldParent);

    HTMLStackItem* createElementFromSavedToken(HTMLStackItem*);

    bool shouldFosterParent() const;
    void fosterParent(Node*);

    bool indexOfFirstUnopenFormattingElement(unsigned& firstUnopenElementIndex) const;
    void reconstructTheActiveFormattingElements();

    void generateImpliedEndTags();
    void generateImpliedEndTagsWithExclusion(const AtomicString& tagName);

    bool inQuirksMode();

    bool isEmpty() const { return !m_openElements.stackDepth(); }
    HTMLElementStack::ElementRecord* currentElementRecord() const { return m_openElements.topRecord(); }
    Element* currentElement() const { return m_openElements.top(); }
    Node* currentNode() const { return m_openElements.topNode(); }
    HTMLStackItem* currentStackItem() const { return m_openElements.topStackItem(); }
    HTMLStackItem* oneBelowTop() const { return m_openElements.oneBelowTop(); }
    Document& ownerDocumentForCurrentNode();
    HTMLElementStack* openElements() const { return &m_openElements; }
    HTMLFormattingElementList* activeFormattingElements() const { return &m_activeFormattingElements; }
    bool currentIsRootNode() { return m_openElements.topNode() == m_openElements.rootNode(); }

    Element* head() const { return m_head->element(); }
    HTMLStackItem* headStackItem() const { return m_head; }

    void setForm(HTMLFormElement*);
    HTMLFormElement* form() const { return m_form; }
    HTMLFormElement* takeForm();

    Document* document()
    {
        return m_document;
    }

    class RedirectToFosterParentGuard : public gc {
    public:
        RedirectToFosterParentGuard(HTMLConstructionSite& tree)
            : m_tree(tree)
            , m_wasRedirectingBefore(tree.m_redirectAttachToFosterParent)
        {
            m_tree.m_redirectAttachToFosterParent = true;
        }

        ~RedirectToFosterParentGuard()
        {
            m_tree.m_redirectAttachToFosterParent = m_wasRedirectingBefore;
        }

    private:
        HTMLConstructionSite& m_tree;
        bool m_wasRedirectingBefore;
    };

private:
    // In the common case, this queue will have only one task because most
    // tokens produce only one DOM mutation.
    typedef std::vector<HTMLConstructionSiteTask, gc_allocator<HTMLConstructionSiteTask>> TaskQueue;

    void setCompatibilityMode(Document::CompatibilityMode);
    void setCompatibilityModeFromDoctype(String* name, String* publicId, String* systemId);

    void attachLater(Node* parent, Node* child, bool selfClosing = false);

    void findFosterSite(HTMLConstructionSiteTask&);

    Element* createHTMLElement(AtomicHTMLToken*);
    Element* createElement(AtomicHTMLToken* token, const AtomicString& namespaceURI);

    void mergeAttributesFromTokenIntoElement(AtomicHTMLToken*, Element*);
    void dispatchDocumentElementAvailableIfNeeded();

    void executeTask(HTMLConstructionSiteTask&);
    void queueTask(const HTMLConstructionSiteTask&);

    Document* m_document;

    // This is the root ContainerNode to which the parser attaches all newly
    // constructed nodes. It points to a DocumentFragment when parsing fragments
    // and a Document in all other cases.
    Node* m_attachmentRoot;

    HTMLStackItem* m_head;
    HTMLFormElement* m_form;
    mutable HTMLElementStack m_openElements;
    mutable HTMLFormattingElementList m_activeFormattingElements;

    TaskQueue m_taskQueue;

    struct PendingText {
        PendingText()
            : parent(nullptr)
            , nextChild(nullptr)
            , whitespaceMode(WhitespaceUnknown)
        {
        }

        void append(Node* newParent, Node* newNextChild, String* newString, WhitespaceMode newWhitespaceMode)
        {
            STARFISH_ASSERT(!parent || parent == newParent);
            parent = newParent;
            STARFISH_ASSERT(!nextChild || nextChild == newNextChild);
            nextChild = newNextChild;
            stringBuilder.append(newString->toUTF32String());
            whitespaceMode = std::min(whitespaceMode, newWhitespaceMode);
        }

        void swap(PendingText& other)
        {
            std::swap(whitespaceMode, other.whitespaceMode);
            std::swap(parent, other.parent);
            std::swap(nextChild, other.nextChild);
            std::swap(stringBuilder, other.stringBuilder);
        }

        void discard()
        {
            PendingText discardedText;
            swap(discardedText);
        }

        bool isEmpty()
        {
            // When the stringbuilder is empty, the parent and whitespace should also be "empty".
            STARFISH_ASSERT((stringBuilder.length() == 0) == !parent);
            STARFISH_ASSERT(!(stringBuilder.length() == 0) || !nextChild);
            STARFISH_ASSERT(!(stringBuilder.length() == 0) || (whitespaceMode == WhitespaceUnknown));
            return (stringBuilder.length() == 0);
        }

        Node* parent;
        Node* nextChild;
        // StringBuilder stringBuilder;
        UTF32String stringBuilder;
        WhitespaceMode whitespaceMode;
    };

    PendingText m_pendingText;

    bool m_isParsingFragment;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-intable
    // In the "in table" insertion mode, we sometimes get into a state where
    // "whenever a node would be inserted into the current node, it must instead
    // be foster parented."  This flag tracks whether we're in that state.
    bool m_redirectAttachToFosterParent;

    bool m_inQuirksMode;
};

}

#endif
