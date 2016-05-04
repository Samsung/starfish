#include "StarFishConfig.h"
#include "HTMLElementStack.h"

#include "dom/Element.h"

namespace StarFish {

namespace {

inline bool isRootNode(HTMLStackItem* item)
{
    return item->isDocumentFragmentNode()
        || item->hasTagName(item->node()->document()->window()->starFish()->staticStrings()->m_htmlLocalName);
}

inline bool isScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return item->hasTagName(s.m_appletLocalName)
        || item->hasTagName(s.m_captionLocalName)
        || item->hasTagName(s.m_marqueeLocalName)
        || item->hasTagName(s.m_objectLocalName)
        || item->hasTagName(s.m_tableLocalName)
        || item->hasTagName(s.m_tdLocalName)
        || item->hasTagName(s.m_thLocalName) /*
        || item->hasTagName(MathMLNames::miTag)
        || item->hasTagName(MathMLNames::moTag)
        || item->hasTagName(MathMLNames::mnTag)
        || item->hasTagName(MathMLNames::msTag)
        || item->hasTagName(MathMLNames::mtextTag)
        || item->hasTagName(MathMLNames::annotation_xmlTag)
        || item->hasTagName(SVGNames::foreignObjectTag)
        || item->hasTagName(SVGNames::descTag)
        || item->hasTagName(SVGNames::titleTag)*/
        || item->hasTagName(s.m_templateLocalName)
        || isRootNode(item);
}

inline bool isListItemScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return isScopeMarker(item)
        || item->hasTagName(s.m_olLocalName)
        || item->hasTagName(s.m_ulLocalName);
}

inline bool isTableScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return item->hasTagName(s.m_tableLocalName)
        || item->hasTagName(s.m_templateLocalName)
        || isRootNode(item);
}

inline bool isTableBodyScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return item->hasTagName(s.m_tbodyLocalName)
        || item->hasTagName(s.m_tfootLocalName)
        || item->hasTagName(s.m_theadLocalName)
        || item->hasTagName(s.m_templateLocalName)
        || isRootNode(item);
}

inline bool isTableRowScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return item->hasTagName(s.m_trLocalName)
        || item->hasTagName(s.m_templateLocalName)
        || isRootNode(item);
}

inline bool isForeignContentScopeMarker(HTMLStackItem* item)
{
    // const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return HTMLElementStack::isMathMLTextIntegrationPoint(item)
        // || HTMLElementStack::isHTMLIntegrationPoint(item)
        || item->isInHTMLNamespace();
}

inline bool isButtonScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return isScopeMarker(item)
        || item->hasTagName(s.m_buttonLocalName);
}

inline bool isSelectScopeMarker(HTMLStackItem* item)
{
    const StaticStrings& s = *item->node()->document()->window()->starFish()->staticStrings();
    return !item->hasTagName(s.m_optgroupLocalName)
        && !item->hasTagName(s.m_optionLocalName);
}

}

HTMLElementStack::ElementRecord::ElementRecord(HTMLStackItem* item, ElementRecord* next)
    : m_item(item)
    , m_next(next)
{
    STARFISH_ASSERT(m_item);
}

HTMLElementStack::ElementRecord::~ElementRecord()
{
}

void HTMLElementStack::ElementRecord::replaceElement(HTMLStackItem* item)
{
    STARFISH_ASSERT(item);
    STARFISH_ASSERT(!m_item || m_item->isElementNode());
    // FIXME: Should this call finishParsingChildren?
    m_item = item;
}

bool HTMLElementStack::ElementRecord::isAbove(ElementRecord* other) const
{
    for (ElementRecord* below = next(); below; below = below->next()) {
        if (below == other)
            return true;
    }
    return false;
}

HTMLElementStack::HTMLElementStack()
    : m_top(0)
    , m_rootNode(0)
    , m_headElement(0)
    , m_bodyElement(0)
    , m_stackDepth(0)
{
}

HTMLElementStack::~HTMLElementStack()
{
}

bool HTMLElementStack::hasOnlyOneElement() const
{
    return !topRecord()->next();
}

bool HTMLElementStack::secondElementIsHTMLBodyElement() const
{
    // This is used the fragment case of <body> and <frameset> in the "in body"
    // insertion mode.
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
    ASSERT(m_rootNode);
    // If we have a body element, it must always be the second element on the
    // stack, as we always start with an html element, and any other element
    // would cause the implicit creation of a body element.
    return !!m_bodyElement;
}

void HTMLElementStack::popHTMLHeadElement()
{
    ASSERT(top() == m_headElement);
    m_headElement = 0;
    popCommon();
}

void HTMLElementStack::popHTMLBodyElement()
{
    ASSERT(top() == m_bodyElement);
    m_bodyElement = 0;
    popCommon();
}

void HTMLElementStack::popAll()
{
    m_rootNode = 0;
    m_headElement = 0;
    m_bodyElement = 0;
    m_stackDepth = 0;
    while (m_top) {
        Node* node = topNode();
        if (node->isElement())
            node->asElement()->finishParsing();
        m_top = m_top->releaseNext();
    }
}

void HTMLElementStack::pop()
{
    STARFISH_ASSERT(!topStackItem()->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_headLocalName));
    popCommon();
}

void HTMLElementStack::popUntil(const QualifiedName& tagName)
{
    while (!topStackItem()->matchesHTMLTag(tagName)) {
        // pop() will ASSERT if a <body>, <head> or <html> will be popped.
        pop();
    }
}

void HTMLElementStack::popUntilPopped(const QualifiedName& tagName)
{
    popUntil(tagName);
    pop();
}

void HTMLElementStack::popUntilNumberedHeaderElementPopped()
{
    while (!topStackItem()->isNumberedHeaderElement())
        pop();
    pop();
}

void HTMLElementStack::popUntil(Element* element)
{
    while (top() != element)
        pop();
}

void HTMLElementStack::popUntilPopped(Element* element)
{
    popUntil(element);
    pop();
}

void HTMLElementStack::popUntilTableScopeMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#clear-the-stack-back-to-a-table-context
    while (!isTableScopeMarker(topStackItem()))
        pop();
}

void HTMLElementStack::popUntilTableBodyScopeMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#clear-the-stack-back-to-a-table-body-context
    while (!isTableBodyScopeMarker(topStackItem()))
        pop();
}

void HTMLElementStack::popUntilTableRowScopeMarker()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#clear-the-stack-back-to-a-table-row-context
    while (!isTableRowScopeMarker(topStackItem()))
        pop();
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#mathml-text-integration-point
bool HTMLElementStack::isMathMLTextIntegrationPoint(HTMLStackItem* item)
{
    if (!item->isElementNode())
        return false;
    return false;
    /*
    return item->hasTagName(MathMLNames::miTag)
        || item->hasTagName(MathMLNames::moTag)
        || item->hasTagName(MathMLNames::mnTag)
        || item->hasTagName(MathMLNames::msTag)
        || item->hasTagName(MathMLNames::mtextTag);
    */
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#html-integration-point
bool HTMLElementStack::isHTMLIntegrationPoint(HTMLStackItem* item)
{
    if (!item->isElementNode())
        return false;
    /*
    if (item->hasTagName(MathMLNames::annotation_xmlTag)) {
        Attribute* encodingAttr = item->getAttributeItem(MathMLNames::encodingAttr);
        if (encodingAttr) {
            const String& encoding = encodingAttr->value();
            return equalIgnoringCase(encoding, "text/html")
                || equalIgnoringCase(encoding, "application/xhtml+xml");
        }
        return false;
    }
    return item->hasTagName(SVGNames::foreignObjectTag)
        || item->hasTagName(SVGNames::descTag)
        || item->hasTagName(SVGNames::titleTag);
    */
    return false;
}

void HTMLElementStack::popUntilForeignContentScopeMarker()
{
    while (!isForeignContentScopeMarker(topStackItem()))
        pop();
}

void HTMLElementStack::pushRootNode(HTMLStackItem* rootItem)
{
    ASSERT(rootItem->isDocumentFragmentNode());
    pushRootNodeCommon(rootItem);
}

void HTMLElementStack::pushHTMLHtmlElement(HTMLStackItem* item)
{
    STARFISH_ASSERT(item->hasTagName(item->node()->document()->window()->starFish()->staticStrings()->m_htmlLocalName));
    pushRootNodeCommon(item);
}

void HTMLElementStack::pushRootNodeCommon(HTMLStackItem* rootItem)
{
    STARFISH_ASSERT(!m_top);
    STARFISH_ASSERT(!m_rootNode);
    m_rootNode = rootItem->node();
    pushCommon(rootItem);
}

void HTMLElementStack::pushHTMLHeadElement(HTMLStackItem* item)
{
    STARFISH_ASSERT(item->hasTagName(item->node()->document()->window()->starFish()->staticStrings()->m_headLocalName));
    STARFISH_ASSERT(!m_headElement);
    m_headElement = item->element();
    pushCommon(item);
}

void HTMLElementStack::pushHTMLBodyElement(HTMLStackItem* item)
{
    STARFISH_ASSERT(item->hasTagName(item->node()->document()->window()->starFish()->staticStrings()->m_bodyLocalName));
    STARFISH_ASSERT(!m_bodyElement);
    m_bodyElement = item->element();
    pushCommon(item);
}

void HTMLElementStack::push(HTMLStackItem* item)
{
    STARFISH_ASSERT(!item->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_htmlLocalName));
    STARFISH_ASSERT(!item->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_headLocalName));
    STARFISH_ASSERT(!item->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_bodyLocalName));
    ASSERT(m_rootNode);
    pushCommon(item);
}

void HTMLElementStack::insertAbove(HTMLStackItem* item, ElementRecord* recordBelow)
{
    STARFISH_ASSERT(item);
    STARFISH_ASSERT(recordBelow);
    STARFISH_ASSERT(m_top);
    STARFISH_ASSERT(!item->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_htmlLocalName));
    STARFISH_ASSERT(!item->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_headLocalName));
    STARFISH_ASSERT(!item->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_bodyLocalName));
    STARFISH_ASSERT(m_rootNode);
    if (recordBelow == m_top) {
        push(item);
        return;
    }

    for (ElementRecord* recordAbove = m_top; recordAbove; recordAbove = recordAbove->next()) {
        if (recordAbove->next() != recordBelow)
            continue;

        m_stackDepth++;
        recordAbove->setNext(new ElementRecord(item, recordAbove->releaseNext()));
        recordAbove->next()->element()->beginParsing();
        return;
    }
    STARFISH_ASSERT_NOT_REACHED();
}

HTMLElementStack::ElementRecord* HTMLElementStack::topRecord() const
{
    STARFISH_ASSERT(m_top);
    return m_top;
}

HTMLStackItem* HTMLElementStack::oneBelowTop() const
{
    // We should never call this if there are fewer than 2 elements on the stack.
    STARFISH_ASSERT(m_top);
    STARFISH_ASSERT(m_top->next());
    if (m_top->next()->stackItem()->isElementNode())
        return m_top->next()->stackItem();
    return 0;
}

void HTMLElementStack::removeHTMLHeadElement(Element* element)
{
    STARFISH_ASSERT(m_headElement == element);
    if (m_top->element() == element) {
        popHTMLHeadElement();
        return;
    }
    m_headElement = 0;
    removeNonTopCommon(element);
}

void HTMLElementStack::remove(Element* element)
{
    // ASSERT(!element->hasTagName(HTMLNames::headTag));
    if (m_top->element() == element) {
        pop();
        return;
    }
    removeNonTopCommon(element);
}

HTMLElementStack::ElementRecord* HTMLElementStack::find(Element* element) const
{
    for (ElementRecord* pos = m_top; pos; pos = pos->next()) {
        if (pos->node() == element)
            return pos;
    }
    return 0;
}

HTMLElementStack::ElementRecord* HTMLElementStack::topmost(const QualifiedName& tagName) const
{
    for (ElementRecord* pos = m_top; pos; pos = pos->next()) {
        if (pos->stackItem()->matchesHTMLTag(tagName))
            return pos;
    }
    return 0;
}

bool HTMLElementStack::contains(Element* element) const
{
    return !!find(element);
}

bool HTMLElementStack::contains(const QualifiedName& tagName) const
{
    return !!topmost(tagName);
}

template <bool isMarker(HTMLStackItem*)>
bool inScopeCommon(HTMLElementStack::ElementRecord* top, const QualifiedName& targetTag)
{
    for (HTMLElementStack::ElementRecord* pos = top; pos; pos = pos->next()) {
        HTMLStackItem* item = pos->stackItem();
        if (item->matchesHTMLTag(targetTag))
            return true;
        if (isMarker(item))
            return false;
    }
    STARFISH_ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return false;
}

bool HTMLElementStack::hasNumberedHeaderElementInScope() const
{
    for (ElementRecord* record = m_top; record; record = record->next()) {
        HTMLStackItem* item = record->stackItem();
        if (item->isNumberedHeaderElement())
            return true;
        if (isScopeMarker(item))
            return false;
    }
    STARFISH_ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return false;
}

bool HTMLElementStack::inScope(Element* targetElement) const
{
    for (ElementRecord* pos = m_top; pos; pos = pos->next()) {
        HTMLStackItem* item = pos->stackItem();
        if (item->node() == targetElement)
            return true;
        if (isScopeMarker(item))
            return false;
    }
    STARFISH_ASSERT_NOT_REACHED(); // <html> is always on the stack and is a scope marker.
    return false;
}

bool HTMLElementStack::inScope(const QualifiedName& targetTag) const
{
    return inScopeCommon<isScopeMarker>(m_top, targetTag);
}
/*
bool HTMLElementStack::inScope(const QualifiedName& tagName) const
{
    return inScope(tagName);
}
*/
bool HTMLElementStack::inListItemScope(const QualifiedName& targetTag) const
{
    return inScopeCommon<isListItemScopeMarker>(m_top, targetTag);
}
/*
bool HTMLElementStack::inListItemScope(const QualifiedName& tagName) const
{
    return inListItemScope(tagName);
}
*/
bool HTMLElementStack::inTableScope(const QualifiedName& targetTag) const
{
    return inScopeCommon<isTableScopeMarker>(m_top, targetTag);
}
/*
bool HTMLElementStack::inTableScope(const QualifiedName& tagName) const
{
    return inTableScope(tagName);
}
*/
bool HTMLElementStack::inButtonScope(const QualifiedName& targetTag) const
{
    return inScopeCommon<isButtonScopeMarker>(m_top, targetTag);
}
/*
bool HTMLElementStack::inButtonScope(const QualifiedName& tagName) const
{
    return inButtonScope(tagName);
}
*/
bool HTMLElementStack::inSelectScope(const QualifiedName& targetTag) const
{
    return inScopeCommon<isSelectScopeMarker>(m_top, targetTag);
}
/*
bool HTMLElementStack::inSelectScope(const QualifiedName& tagName) const
{
    return inSelectScope(tagName);
}
*/
bool HTMLElementStack::hasTemplateInHTMLScope() const
{
    return inScopeCommon<isRootNode>(m_top, m_top->node()->document()->window()->starFish()->staticStrings()->m_templateLocalName);
}

Element* HTMLElementStack::htmlElement() const
{
    STARFISH_ASSERT(m_rootNode);
    return m_rootNode->asElement();
}

Element* HTMLElementStack::headElement() const
{
    STARFISH_ASSERT(m_headElement);
    return m_headElement;
}

Element* HTMLElementStack::bodyElement() const
{
    STARFISH_ASSERT(m_bodyElement);
    return m_bodyElement;
}

/*
ContainerNode* HTMLElementStack::rootNode() const
{
    ASSERT(m_rootNode);
    return m_rootNode;
}*/

Node* HTMLElementStack::rootNode() const
{
    STARFISH_ASSERT(m_rootNode);
    return m_rootNode;
}

void HTMLElementStack::pushCommon(HTMLStackItem* item)
{
    STARFISH_ASSERT(m_rootNode);

    m_stackDepth++;
    m_top = new ElementRecord(item, m_top);
}

void HTMLElementStack::popCommon()
{
    STARFISH_ASSERT(!topStackItem()->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_htmlLocalName));
    STARFISH_ASSERT(!topStackItem()->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_headLocalName) || !m_headElement);
    STARFISH_ASSERT(!topStackItem()->hasTagName(topStackItem()->node()->document()->window()->starFish()->staticStrings()->m_bodyLocalName) || !m_bodyElement);
    top()->finishParsing();
    m_top = m_top->releaseNext();

    m_stackDepth--;
}

void HTMLElementStack::removeNonTopCommon(Element* element)
{
    STARFISH_ASSERT(!element->localName()->equals(element->document()->window()->starFish()->staticStrings()->m_htmlLocalName.string()));
    STARFISH_ASSERT(!element->localName()->equals(element->document()->window()->starFish()->staticStrings()->m_bodyLocalName.string()));
    STARFISH_ASSERT(top() != element);
    for (ElementRecord* pos = m_top; pos; pos = pos->next()) {
        if (pos->next()->element() == element) {
            // FIXME: Is it OK to call finishParsingChildren()
            // when the children aren't actually finished?
            // element->finishParsingChildren();
            pos->setNext(pos->next()->releaseNext());
            m_stackDepth--;
            return;
        }
    }
    ASSERT_NOT_REACHED();
}

HTMLElementStack::ElementRecord* HTMLElementStack::furthestBlockForFormattingElement(Element* formattingElement) const
{
    ElementRecord* furthestBlock = 0;
    for (ElementRecord* pos = m_top; pos; pos = pos->next()) {
        if (pos->element() == formattingElement)
            return furthestBlock;
        if (pos->stackItem()->isSpecialNode())
            furthestBlock = pos;
    }
    STARFISH_ASSERT_NOT_REACHED();
    return 0;
}

#ifndef NDEBUG

void HTMLElementStack::show()
{
    /*
    for (ElementRecord* record = m_top.get(); record; record = record->next())
        record->element()->showNode();
        */
}

#endif

}
