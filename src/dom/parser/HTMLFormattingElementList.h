#ifndef __StarFishHTMLFormattingElementList__
#define __StarFishHTMLFormattingElementList__

#include "dom/parser/HTMLStackItem.h"

namespace StarFish {

class Element;

// This may end up merged into HTMLElementStack.
class HTMLFormattingElementList : public gc {
public:
    HTMLFormattingElementList();
    ~HTMLFormattingElementList();

    // Ideally Entry would be private, but HTMLTreeBuilder has to coordinate
    // between the HTMLFormattingElementList and HTMLElementStack and needs
    // access to Entry::isMarker() and Entry::replaceElement() to do so.
    class Entry : public gc {
    public:
        // Inline because they're hot and Vector<T> uses them.
        explicit Entry(HTMLStackItem* item)
            : m_item(item)
        {
        }
        enum MarkerEntryType { MarkerEntry };
        explicit Entry(MarkerEntryType)
            : m_item(nullptr)
        {
        }
        ~Entry() { }

        bool isMarker() const { return !m_item; }

        HTMLStackItem* stackItem() const { return m_item; }
        Element* element() const
        {
            // The fact that !m_item == isMarker() is an implementation detail
            // callers should check isMarker() before calling element().
            if (m_item) {
                return m_item->element();
            } else {
                return nullptr;
            }
        }
        void replaceElement(HTMLStackItem* item) { m_item = item; }

        // Needed for use with Vector. These are super-hot and must be inline.
        bool operator==(Element* element) const { return !m_item ? !element : m_item->element() == element; }
        bool operator!=(Element* element) const { return !m_item ? !!element : m_item->element() != element; }

    private:
        HTMLStackItem* m_item;
    };

    class Bookmark {
    public:
        explicit Bookmark(Entry* entry)
            : m_hasBeenMoved(false)
            , m_mark(entry)
        {
        }

        void moveToAfter(Entry* before)
        {
            m_hasBeenMoved = true;
            m_mark = before;
        }

        bool hasBeenMoved() const { return m_hasBeenMoved; }
        Entry* mark() const { return m_mark; }

    private:
        bool m_hasBeenMoved;
        Entry* m_mark;
    };

    bool isEmpty() const { return !size(); }
    size_t size() const { return m_entries.size(); }

    Element* closestElementInScopeWithName(const AtomicString&);

    Entry* find(Element*);
    bool contains(Element*);
    void append(HTMLStackItem*);
    void remove(Element*);

    Bookmark bookmarkFor(Element*);
    void swapTo(Element* oldElement, HTMLStackItem* newItem, const Bookmark&);

    void appendMarker();
    // clearToLastMarker also clears the marker (per the HTML5 spec).
    void clearToLastMarker();

    const Entry& at(size_t i) const { return m_entries[i]; }
    Entry& at(size_t i) { return m_entries[i]; }

#ifndef NDEBUG
    void show();
#endif

private:
    Entry* first() { return &at(0); }

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#list-of-active-formatting-elements
    // These functions enforce the "Noah's Ark" condition, which removes redundant mis-nested elements.
    void tryToEnsureNoahsArkConditionQuickly(HTMLStackItem*, std::vector<HTMLStackItem*, gc_allocator<HTMLStackItem*>>& remainingCandiates);
    void ensureNoahsArkCondition(HTMLStackItem*);

    std::vector<Entry, gc_allocator<Entry>> m_entries;
};

}

#endif
