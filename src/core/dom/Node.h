/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishNode__
#define __StarFishNode__

#include "core/dom/EventTarget.h"
#include "core/style/Style.h"

namespace StarFish {

class CSSSelector;
class CharacterData;
class ComputedStyle;
class Document;
class DocumentFragment;
class DOMTokenList;
class Element;
class Frame;
class HTMLCollection;
class HTMLFormControl;
class NodeList;
class RareNodeMembers;
class RareElementMembers;

typedef GCVector<std::pair<String*, HTMLCollection*>> ActiveHTMLCollectionList;
typedef GCVector<std::pair<String*, NodeList*>> ActiveNodeListVector;

class RareNodeMembers : public gc {
public:
    RareNodeMembers()
        : m_children(nullptr)
        , m_childNodeList(nullptr)
        , m_domTokenList(nullptr)
        , m_activeHtmlCollectionListsForTagName(nullptr)
        , m_activeHtmlCollectionListsForClassName(nullptr)
        , m_activeNodeListVectorForName(nullptr)
    {
    }

    virtual bool isRareElementMembers() const
    {
        return false;
    }

    RareElementMembers* asRareElementMembers() const
    {
        STARFISH_ASSERT(isRareElementMembers());
        return (RareElementMembers*)(this);
    }

    ActiveHTMLCollectionList* ensureActiveHtmlCollectionListForTagName();
    ActiveHTMLCollectionList* ensureActiveHtmlCollectionListForClassName();
    ActiveNodeListVector* ensureActiveNodeListVectorForName();

    NodeList* ensureQueryInActiveNodeListVectorForName(Node* ownerNode,
                                                       String* query);

    HTMLCollection* hasQueryInActiveHtmlCollectionList(
        ActiveHTMLCollectionList* list, String* query);
    void putActiveHtmlCollectionListWithQuery(ActiveHTMLCollectionList* list,
                                              String* query,
                                              HTMLCollection* coll);
    void invalidateActiveActiveNodeListCacheIfNeeded();

    HTMLCollection* m_children;
    NodeList* m_childNodeList;
    DOMTokenList* m_domTokenList;

    ActiveHTMLCollectionList* m_activeHtmlCollectionListsForTagName;
    ActiveHTMLCollectionList* m_activeHtmlCollectionListsForClassName;
    ActiveNodeListVector* m_activeNodeListVectorForName;
};

class Node : public EventTarget {
protected:
    Node(Document* document)
        : EventTarget(document)
        , m_inParsing(false)
        , m_needsStyleRecalc(true)
        , m_childNeedsStyleRecalc(true)
        , m_needsFrameTreeBuild(true)
        , m_childNeedsFrameTreeBuild(true)
        , m_didInlineStyleModifiedAfterAttributeSet(false)
        , m_tabIndexWasSetExplicitly(false)
        , m_hasDirAttribute(false)
        , m_state(NodeStateNormal)
        , m_rareNodeMembers(nullptr)
        , m_nextSibling(nullptr)
        , m_previousSibling(nullptr)
        , m_firstChild(nullptr)
        , m_lastChild(nullptr)
        , m_parentNode(nullptr)
        , m_style(nullptr)
        , m_frame(nullptr)
    {
    }

public:
    /* 4.4 Interface Node */

    enum NodeType {
        ELEMENT_NODE = 1,
        ATTRIBUTE_NODE = 2, // historical
        TEXT_NODE = 3,
        CDATA_SECTION_NODE = 4,    // historical
        ENTITY_REFERENCE_NODE = 5, // historical
        ENTITY_NODE = 6,           // historical
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

    virtual NodeType nodeType() const = 0;
    virtual String* nodeName() = 0;
    virtual Nullable<String*> prefix()
    {
        // For nodes other than elements and attributes, the prefix is always
        // null
        return Nullable<String*>();
    }

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

    String* baseURI() const;

    Nullable<String*> nodeValue() const;
    void setNodeValue(Nullable<String*> newVal);

    Nullable<String*> textContent() const;
    void setTextContent(Nullable<String*> val);

    bool isEqualNode(Node* other);

    bool isDescendantOf(const Node* other);

    Node* cloneNode(bool deep = false);

    unsigned short compareDocumentPosition(const Node* other);

    bool contains(const Node* other) const
    {
        if (other == nullptr) {
            return false;
        }
        if (this == other) {
            return true;
        }
        for (Node* child = firstChild(); child != nullptr;
             child = child->nextSibling()) {
            if (child->contains(other)) {
                return true;
            }
        }
        return false;
    }

    // https://dom.spec.whatwg.org/#dom-node-lookupnamespaceuri
    Nullable<String*> lookupPrefix(Nullable<String*> namespaceUri);
    // https://dom.spec.whatwg.org/#dom-node-lookupnamespaceuri
    Nullable<String*> lookupNamespaceURI(Nullable<String*> prefix);
    // https://dom.spec.whatwg.org/#dom-node-isdefaultnamespace
    bool isDefaultNamespace(Nullable<String*> namespaceUri);

    bool isInDocumentScope();
    bool isInDocumentScopeAndDocumentParticipateInRendering();

    Node* appendChild(Node* child);
    Node* insertBefore(Node* child, Node* childRef = nullptr);
    Node* replaceChild(Node* child, Node* childToRemove);
    Node* removeChild(Node* child);

    Node* parserAppendChild(Node* child);
    void parserRemoveChild(Node*);
    void parserInsertBefore(Node* newChild, Node* refChild);
    void parserTakeAllChildrenFrom(Node* oldParent);

    /* 4.5. Interface Document */
    HTMLCollection* getElementsByTagName(String* name);
    HTMLCollection* getElementsByTagName(QualifiedName qualifiedName);
    HTMLCollection* getElementsByClassName(String* classNames);

    void parseSelector(GCVector<CSSSelectorList*>& selectorListContainer,
                       String* selectors);
    Element* querySelector(String* selector);
    NodeList* querySelectorAll(String* selector);

    /* Other methods (not in Node Interface) */
    enum NodeState {
        NodeStateNormal = 0,
        NodeStateActive = 1 << 0,
        NodeStateFocused = 1 << 1,
        NodeStateHovered = 1 << 2,
        NodeStateTarget = 1 << 3,
    };

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNode() const override;

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

    void setDocument(Document* s)
    {
        m_document = s;
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
            if (matchedDescendant) {
                return matchedDescendant;
            }
            child = child->nextSibling();
        }
        return nullptr;
    }

    virtual Node* clone() = 0;

    Node* nearestParentElement();

    void setState(NodeState state, bool enable);

    int state()
    {
        return m_state;
    }

    // MUST uses same bit with StyleResolver::StyleDamageFrom
    enum StyleChangeReason {
        JustNeedsRecalcSelf = 0,
        InlineStyleChange = 0,
        IdChange = 1,
        ClassChange = 1 << 1,
        AttributeChange = 1 << 2,
        ElementStateChange = 1 << 3,
        DOMTreeChange = 1 << 4,
    };
    void setNeedsStyleRecalc(
        StyleChangeReason reason = StyleChangeReason::JustNeedsRecalcSelf);
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
        Node* parent = parentNode();
        while (parent) {
            if (parent->m_childNeedsStyleRecalc) {
                break;
            }
            parent->m_childNeedsStyleRecalc = true;
            parent = parent->parentNode();
        }
    }

    bool childNeedsStyleRecalc()
    {
        return m_childNeedsStyleRecalc;
    }

    void clearChildNeedsStyleRecalc()
    {
        m_childNeedsStyleRecalc = false;
    }

    enum FrameTreeBuildReason {
        AppendChild,
        InsertBefore,
        RemoveFromParent,
        UpdateFromParent,
        UpdateAtSelf,
    };
    void setNeedsFrameTreeBuild(FrameTreeBuildReason reason);

    void markNeedsFrameTreeBuild()
    {
        m_needsFrameTreeBuild = true;
    }

    bool needsFrameTreeBuild()
    {
        return m_needsFrameTreeBuild;
    }

    void clearNeedsFrameTreeBuild()
    {
        m_needsFrameTreeBuild = false;
    }

    void markChildNeedsFrameTreeBuild()
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

    void propagateMarkChildNeedsFrameTreeBuild();
    void setNeedsLayout();
    void setNeedsPainting();
    void setNeedsComposite();

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
        auto s = nodeName()->toUTF8NonGCString();
        printf("[%s] ", s.data());
    }
#endif
    void loadFontAndChangeFontPercentToFixedIfNeeded(StarFish* sf,
                                                     float fixedParentFontSize,
                                                     Length parentFontSize,
                                                     Length rootFontSize,
                                                     Font* parentFont);

    OverflowValue appliedOverflowX();
    OverflowValue appliedOverflowY();

    Element* firstElementChild();
    Element* lastElementChild();
    unsigned long childElementCount();

    HTMLCollection* children();
    DOMTokenList* classList();
    virtual NamedNodeMap* attributes();

    Element* nextElementSibling();
    Element* previousElementSibling();

    virtual void didComputedStyleChanged(ComputedStyle* oldStyle,
                                         ComputedStyle* newStyle);

    template <typename F>
    void notifyDOMEventToParentTree(Node* parent, const F& fn)
    {
        while (parent) {
            fn(parent);
            parent = parent->parentNode();
        }
    }

    virtual void didCharacterDataModified(String* before, String* after)
    {
    }
    virtual void didNodeInserted(Node* parent, Node* newChild);
    virtual void didNodeRemoved(Node* parent, Node* oldChild);

    // These two callbacks are fired only document participate in rendering
    virtual void didNodeInsertedToDocumentTree()
    {
    }
    virtual void didNodeRemovedFromDocumentTree();

    virtual void didNodeAdopted()
    {
    }

    virtual void didStateChanged(int oldState, int newState)
    {
    }

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

    bool isPseudoElement() const
    {
        return getPseudoId() !=
               StyleResolver::PseudoElementType::PseudoElementNone;
    }

    bool isBeforePseudoElement() const
    {
        return getPseudoId() ==
               StyleResolver::PseudoElementType::PseudoElementBefore;
    }

    bool isAfterPseudoElement() const
    {
        return getPseudoId() ==
               StyleResolver::PseudoElementType::PseudoElementAfter;
    }

    bool isFirstLetterPseudoElement() const
    {
        return getPseudoId() ==
               StyleResolver::PseudoElementType::PseudoElementFirstLetter;
    }

    virtual StyleResolver::PseudoElementType getPseudoId() const
    {
        return StyleResolver::PseudoElementType::PseudoElementNone;
    }

    virtual bool isContainerNode()
    {
        return true;
    }

    virtual bool isHTMLFormControl() const
    {
        return false;
    }

    HTMLFormControl* asHTMLFormObject()
    {
        STARFISH_ASSERT(isHTMLFormControl());
        return reinterpret_cast<HTMLFormControl*>(this);
    }

private:
    void validatePreinsert(Node* child, Node* childRef);
    void validateReplace(Node* child, Node* childToRemove);

    void setSiblingsNeedsStyleRecalcIfNeeded(StyleChangeReason reason);
    void setChildrenNeedsStyleRecalcIfNeeded(StyleChangeReason reason);

protected:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_object));   // ScriptWrappable
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_document)); // DocumentHoladable
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_eventListeners)); // EventTarget
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_rareNodeMembers));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_nextSibling));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_previousSibling));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_firstChild));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_lastChild));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_parentNode));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_style));
        GC_set_bit(desc, GC_WORD_OFFSET(Node, m_frame));
    }

    Node* getDoctypeChild();
    bool m_inParsing : 1;
    bool m_needsStyleRecalc : 1;
    bool m_childNeedsStyleRecalc : 1;
    bool m_needsFrameTreeBuild : 1;
    bool m_childNeedsFrameTreeBuild : 1;
    // for Element
    bool m_didInlineStyleModifiedAfterAttributeSet : 1;
    bool m_tabIndexWasSetExplicitly : 1;
    // for HTMLElelement
    bool m_hasDirAttribute : 1;

    int m_state : 8;

    RareNodeMembers* m_rareNodeMembers;

private:
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
