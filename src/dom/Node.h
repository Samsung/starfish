#ifndef __StarFishNode__
#define __StarFishNode__

#include "util/String.h"
#include "style/Unit.h"
#include "style/Length.h"
#include "style/ComputedStyle.h"
#include "platform/canvas/Canvas.h"
#include "platform/canvas/image/ImageData.h"
#include "dom/Event.h"
#include "dom/EventTarget.h"
#include "dom/HTMLCollection.h"
#include "dom/DOMTokenList.h"
#include "dom/NamedNodeMap.h"
#include "dom/DOMException.h"

namespace StarFish {

class CharacterData;
class DocumentFragment;
class Element;
class Frame;
class NodeList;
class RareNodeMembers;
class RareElementMembers;

// TODO use weak reference for activeHtmlCollectionLists
typedef std::vector<std::pair<String*, HTMLCollection*>, gc_allocator<std::pair<String*, HTMLCollection*>>> ActiveHTMLCollectionList;

class RareNodeMembers : public gc {
public:
    RareNodeMembers()
        : m_children(nullptr)
        , m_childNodeList(nullptr)
        , m_domTokenList(nullptr)
        , m_activeHtmlCollectionListsForTagName(nullptr)
        , m_activeHtmlCollectionListsForClassName(nullptr)
    {
    }

    virtual bool isRareElementMembers()
    {
        return false;
    }

    RareElementMembers* asRareElementMembers()
    {
        STARFISH_ASSERT(isRareElementMembers());
        return (RareElementMembers*)(this);
    }

    ActiveHTMLCollectionList* ensureActiveHtmlCollectionListForTagName()
    {
        if (m_activeHtmlCollectionListsForTagName == nullptr) {
            m_activeHtmlCollectionListsForTagName = new(GC) ActiveHTMLCollectionList;
        }
        return m_activeHtmlCollectionListsForTagName;
    }

    ActiveHTMLCollectionList* ensureActiveHtmlCollectionListForClassName()
    {
        if (m_activeHtmlCollectionListsForClassName == nullptr) {
            m_activeHtmlCollectionListsForClassName = new(GC) ActiveHTMLCollectionList;
        }
        return m_activeHtmlCollectionListsForClassName;
    }

    HTMLCollection* hasQueryInActiveHtmlCollectionList(ActiveHTMLCollectionList* list, String* query);
    void putActiveHtmlCollectionListWithQuery(ActiveHTMLCollectionList* list, String* query, HTMLCollection* coll);
    void invalidateActiveActiveNodeListCacheIfNeeded();

    HTMLCollection* m_children;
    NodeList* m_childNodeList;
    DOMTokenList* m_domTokenList;

    ActiveHTMLCollectionList* m_activeHtmlCollectionListsForTagName;
    ActiveHTMLCollectionList* m_activeHtmlCollectionListsForClassName;
};

class Node : public EventTarget {
protected:
    Node(Document* document, ScriptBindingInstance* instance)
        : EventTarget()
    {
        m_document = document;
        initNode();
    }

    Node(Document* document);

    void initNode()
    {
        m_parentNode = nullptr;

        m_nextSibling = nullptr;
        m_previousSibling = nullptr;

        m_firstChild = nullptr;
        m_lastChild = nullptr;

        m_state = NodeStateNormal;

        m_inParsing = false;
        m_needsStyleRecalc = true;
        m_childNeedsStyleRecalc = true;

        m_needsFrameTreeBuild = true;
        m_childNeedsFrameTreeBuild = true;

        m_didInlineStyleModifiedAfterAttributeSet = false;
        m_hasDirAttribute = false;

        m_style = nullptr;
        m_frame = nullptr;
        m_rareNodeMembers = nullptr;
    }

public:
    virtual ~Node()
    {
    }

    /* 4.4 Interface Node */

    enum NodeType {
        ELEMENT_NODE = 1,
        ATTRIBUTE_NODE = 2, // historical
        TEXT_NODE = 3,
        CDATA_SECTION_NODE = 4, // historical
        ENTITY_REFERENCE_NODE = 5, // historical
        ENTITY_NODE = 6, // historical
        PROCESSING_INSTRUCTION_NODE = 7,
        COMMENT_NODE = 8,
        DOCUMENT_NODE = 9,
        DOCUMENT_TYPE_NODE = 10,
        DOCUMENT_FRAGMENT_NODE = 11,
        NOTATION_NODE = 12, // historical
    };

    enum DocumentPosition {
        DOCUMENT_POSITION_DISCONNECTED = 0x01,
        DOCUMENT_POSITION_PRECEDING = 0x02,
        DOCUMENT_POSITION_FOLLOWING = 0x04,
        DOCUMENT_POSITION_CONTAINS = 0x08,
        DOCUMENT_POSITION_CONTAINED_BY = 0x10,
        DOCUMENT_POSITION_IMPLEMENTATION_SPECIFIC = 0x20,
    };

    virtual NodeType nodeType() = 0;
    virtual String* nodeName() = 0;
    virtual void beginParsing()
    {
        m_inParsing = true;
    }
    virtual void finishParsing()
    {
        m_inParsing = false;
    }

    Document* ownerDocument() const
    {
        if (isDocument()) {
            return nullptr;
        } else {
            return m_document;
        }
    }

    Node* parentNode() const
    {
        return m_parentNode;
    }

    Element* parentElement()
    {
        Node* parent = parentNode();
        if (parent && parent->nodeType() == ELEMENT_NODE) {
            return parent->asElement();
        } else {
            return nullptr;
        }
    }

    virtual String* localName()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    bool hasChildNodes() const
    {
        return firstChild();
    }

    NodeList* childNodes();

    Node* firstChild() const
    {
        return m_firstChild;
    }

    Node* lastChild() const
    {
        return m_lastChild;
    }

    Node* previousSibling() const
    {
        return m_previousSibling;
    }

    Node* nextSibling() const
    {
        return m_nextSibling;
    }

    virtual String* nodeValue()
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    virtual void setNodeValue(String* val) { };

    virtual String* textContent() = 0;

    virtual void setTextContent(String* val) { };

    bool isEqualNode(Node* other);

    virtual Node* cloneNode(bool deep = false);

    unsigned short compareDocumentPosition(const Node* other);

    bool contains(const Node* other) const
    {
        if (other == nullptr) {
            return false;
        }
        if (this == other) {
            return true;
        }
        for (Node* child = firstChild(); child != nullptr; child = child->nextSibling()) {
            if (child->contains(other)) {
                return true;
            }
        }
        return false;
    }

    String* lookupPrefix(String* namespaceUri);

    String* lookupNamespaceURI(String* prefix) const
    {
        if (prefix == nullptr) {
            return String::emptyString;
        }
        if (prefix->equals(String::emptyString)) {
            return String::emptyString;
        }
        // Impl here
        return String::emptyString;
    }
    bool isDefaultNamespace(String* namespaceUri)
    {
        // Impl here
        return false;
    }

    bool isInDocumentScope();

    Node* appendChild(Node* child);
    Node* insertBefore(Node* child, Node* childRef = nullptr);
    Node* replaceChild(Node* child, Node* childToRemove);
    Node* removeChild(Node* child);

    Node* parserAppendChild(Node* child);
    void parserRemoveChild(Node*);
    void parserInsertBefore(Node* newChild, Node* refChild);
    void parserTakeAllChildrenFrom(Node* oldParent);

    /* 4.5. Interface Document */
    HTMLCollection* getElementsByTagName(QualifiedName qualifiedName);
    HTMLCollection* getElementsByClassName(String* classNames);


    /* Other methods (not in Node Interface) */
    enum NodeState {
        NodeStateNormal,
        NodeStateActive,
    };

    virtual bool isNode() const
    {
        return true;
    }

    virtual bool isElement() const
    {
        return false;
    }

    virtual bool isDocument() const
    {
        return false;
    }

    virtual bool isCharacterData() const
    {
        return false;
    }

    virtual bool isDocumentType() const
    {
        return false;
    }

    virtual bool isDocumentFragment() const
    {
        return false;
    }

    virtual bool isComment() const
    {
        return false;
    }

    virtual bool isText() const
    {
        return false;
    }

    virtual bool isAttr() const
    {
        return false;
    }

    CharacterData* asCharacterData()
    {
        STARFISH_ASSERT(isCharacterData());
        return (CharacterData*)this;
    }

    Element* asElement()
    {
        STARFISH_ASSERT(isElement());
        return (Element*)this;
    }

    Document* asDocument()
    {
        STARFISH_ASSERT(isDocument());
        return (Document*)this;
    }

    DocumentType* asDocumentType()
    {
        STARFISH_ASSERT(isDocumentType());
        return (DocumentType*)this;
    }

    Text* asText()
    {
        STARFISH_ASSERT(isText());
        return (Text*)this;
    }

    void setFirstChild(Node* s)
    {
        m_firstChild = s;
    }

    void setNextSibling(Node* s)
    {
        m_nextSibling = s;
    }

    void setPreviousSibling(Node* s)
    {
        m_previousSibling = s;
    }

    void setParentNode(Node* s)
    {
        m_parentNode = s;
    }

    void remove()
    {
        if (m_parentNode)
            m_parentNode->removeChild(this);
    }

    template <typename T>
    Node* childMatchedBy(Node* parent, T fn)
    {
        Node* child = parent->firstChild();
        while (child) {
            if (fn(child)) {
                return child;
            }
            Node* matchedDescendant = childMatchedBy(child, fn);
            if (matchedDescendant)
                return matchedDescendant;
            child = child->nextSibling();
        }
        return nullptr;
    }

    Document* document()
    {
        return m_document;
    }

    virtual Node* clone() = 0;

    void setState(NodeState state)
    {
        if (state != m_state) {
            m_state = state;
            setNeedsStyleRecalc();
        }
    }

    NodeState state()
    {
        return m_state;
    }

    inline void setNeedsStyleRecalc();
    bool needsStyleRecalc()
    {
        return m_needsStyleRecalc;
    }

    void clearNeedsStyleRecalc()
    {
        m_needsStyleRecalc = false;
    }

    void setChildNeedsStyleRecalc()
    {
        m_childNeedsStyleRecalc = true;
    }

    bool childNeedsStyleRecalc()
    {
        return m_childNeedsStyleRecalc;
    }

    void clearChildNeedsStyleRecalc()
    {
        m_childNeedsStyleRecalc = false;
    }

    void setNeedsFrameTreeBuild();

    bool needsFrameTreeBuild()
    {
        return m_needsFrameTreeBuild;
    }

    void clearNeedsFrameTreeBuild()
    {
        m_needsFrameTreeBuild = false;
    }

    void setChildNeedsFrameTreeBuild()
    {
        m_childNeedsFrameTreeBuild = true;
    }

    bool childNeedsFrameTreeBuild()
    {
        return m_childNeedsFrameTreeBuild;
    }

    void clearChildNeedsFrameTreeBuild()
    {
        m_childNeedsFrameTreeBuild = false;
    }

    inline void setNeedsLayout();
    inline void setNeedsPainting();
    inline void setNeedsComposite();

    void setStyle(ComputedStyle* style)
    {
        ComputedStyle* old = m_style;
        m_style = style;
        didComputedStyleChanged(old, style);
    }

    ComputedStyle* style()
    {
        return m_style;
    }

    void setFrame(Frame* frame)
    {
        m_frame = frame;
    }

    Frame* frame()
    {
        return m_frame;
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump()
    {
        printf("[%s] ", localName()->utf8Data());
    }
    virtual void dumpStyle();
#endif

    Element* firstElementChild();
    Element* lastElementChild();
    unsigned long childElementCount();

    HTMLCollection* children();
    DOMTokenList* classList();
    virtual NamedNodeMap* attributes();

    Element* nextElementSibling();
    Element* previousElementSibling();

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle, ComputedStyle* newStyle);

    template <typename F>
    void notifyDOMEventToParentTree(Node* parent, const F& fn)
    {
        while (parent) {
            fn(parent);
            parent = parent->parentNode();
        }
    }

    virtual void didCharacterDataModified(String* before, String* after) { }
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);
    virtual void didNodeInsertedToDocumenTree() { }
    virtual void didNodeRemovedFromDocumenTree() { }

    virtual bool dispatchEvent(Event* event);

    bool hasRareMembers()
    {
        return m_rareNodeMembers != nullptr;
    }
    RareNodeMembers* rareMembers()
    {
        return m_rareNodeMembers;
    }
    virtual RareNodeMembers* ensureRareMembers();

    void invalidateNodeListCacheDueToChangeClassNameOfDescendant();
private:
    String* lookupNamespacePrefix(String* namespaceUri, Element* element);
    virtual String* prefix()
    {
        // For nodes other than elements and attributes, the prefix is always null
        return nullptr;
    }

    void validatePreinsert(Node* child, Node* childRef);
    void validateReplace(Node* child, Node* childToRemove);

protected:
    Node* getDocTypeChild();
    bool m_inParsing : 1;
    bool m_needsStyleRecalc : 1;
    bool m_childNeedsStyleRecalc : 1;
    bool m_needsFrameTreeBuild : 1;
    bool m_childNeedsFrameTreeBuild : 1;
    // for Element
    bool m_didInlineStyleModifiedAfterAttributeSet : 1;
    // for HTMLElelement
    bool m_hasDirAttribute : 1;

    NodeState m_state : 1;

    RareNodeMembers* m_rareNodeMembers;
private:
    Document* m_document;
    Node* m_nextSibling;
    Node* m_previousSibling;
    Node* m_firstChild;
    Node* m_lastChild;
    Node* m_parentNode;
    ComputedStyle* m_style;
    Frame* m_frame;
};
}

#endif
