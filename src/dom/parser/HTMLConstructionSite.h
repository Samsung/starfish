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
    void insertForeignElement(AtomicHTMLToken*);

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
    void generateImpliedEndTagsWithExclusion(const QualifiedName& tagName);

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
    Element* createElement(AtomicHTMLToken*);

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
