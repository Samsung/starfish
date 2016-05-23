#include "StarFishConfig.h"
#include "Element.h"
#include "Text.h"

#include "Document.h"
#include "DocumentFragment.h"

#include "dom/parser/HTMLParser.h"

#include "style/Style.h"
#include "style/CSSParser.h"

#include "HTMLHtmlElement.h"
#include "HTMLHeadElement.h"
#include "HTMLBodyElement.h"
#include "HTMLDivElement.h"
#include "HTMLImageElement.h"
#include "HTMLScriptElement.h"
#include "HTMLStyleElement.h"
#include "HTMLBRElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMetaElement.h"
#include "HTMLParagraphElement.h"
#include "HTMLSpanElement.h"
#include "HTMLUnknownElement.h"
#ifdef STARFISH_ENABLE_AUDIO
#include "HTMLAudioElement.h"
#endif
#include "dom/Attr.h"

namespace StarFish {

size_t Element::hasAttribute(QualifiedName name)
{
    for (size_t i = 0; i < m_attributes.size(); i ++) {
        if (m_attributes[i].name() == name) {
            return i;
        }
    }
    return SIZE_MAX;
}

String* Element::getAttribute(size_t pos)
{
    return m_attributes[pos].value();
}

void Element::setAttribute(QualifiedName name, String* value)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
        m_attributes.push_back(Attribute(name, value));
        didAttributeChanged(name, String::emptyString, value, true, false);
    } else {
        String* v = m_attributes[idx].value();
        m_attributes[idx].setValue(value);
        didAttributeChanged(name, v, value, false, false);
    }
}

void Element::removeAttribute(QualifiedName name)
{
    size_t idx = hasAttribute(name);
    if (idx == SIZE_MAX) {
    } else {
        String* v = m_attributes[idx].value();
        m_attributes.erase(m_attributes.begin() + idx);
        Attr* attrNode = attr(name);
        if (attrNode) {
            attrNode->detachFromElement(v);
            STARFISH_ASSERT(hasRareMembers());
            STARFISH_ASSERT(rareMembers()->isRareElementMembers());
            STARFISH_ASSERT(rareMembers()->asRareElementMembers()->m_attrList);
            AttrList* attrList = rareMembers()->asRareElementMembers()->m_attrList;
            for (unsigned i = 0, size = attrList->size(); i < size; i++) {
                if (attrList->at(i) == attrNode) {
                    attrList->erase(attrList->begin() + i);
                    break;
                }
            }
        }
        didAttributeChanged(name, v, String::emptyString, false, true);
    }
}

void Element::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_id) {
        m_id = value;
        setNeedsStyleRecalc();
    } else if (name == ss->m_class) {
        DOMTokenList::tokenize(&m_classNames, value);
        setNeedsStyleRecalc();

        // propagate invalidate nodeList cache(getElementsByClassName) damage to parent tree
        Node* parent = parentNode();
        while (parent) {
            parent->invalidateNodeListCacheDueToChangeClassNameOfDescendant();
            parent = parent->parentNode();
        }
    } else if (name == ss->m_style) {
        if (old->equals(String::emptyString)) {
            attributeData(name).registerGetterCallback(this, [](Element* element, const Attribute * const attr) -> String* {
                if (element->m_didInlineStyleModifiedAfterAttributeSet) {
                    return element->inlineStyle()->generateCSSText();
                } else {
                    return attr->valueWithoutCheckGetter();
                }
                return String::emptyString;
            });
        }
        inlineStyle()->clear();
        CSSParser parser(document());
        parser.parseStyleDeclaration(value, inlineStyle());
        m_didInlineStyleModifiedAfterAttributeSet = false;
    }
}

void Element::setTextContent(String* text)
{
    Text* node = new Text(document(), text);

    while (firstChild()) {
        removeChild(firstChild());
    }
    if (!text->equals(String::emptyString)) {
        appendChild(node);
    }
}

String* Element::innerHTML()
{
    STARFISH_RELEASE_ASSERT_NOT_REACHED();
}

void Element::setInnerHTML(String* html)
{
    while (firstChild()) {
        removeChild(firstChild());
    }

    DocumentFragment* df = document()->createDocumentFragment();

    HTMLParser parser(document()->window()->starFish(), df, this, html);
    parser.startParse();
    parser.parseStep();
    appendChild(df);
}

Node* Element::clone()
{
    Element* newNode = nullptr;
    if (isHTMLElement()) {
        newNode = document()->createElement(name());
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    STARFISH_ASSERT(newNode);

    for (const Attribute& attr : m_attributes) {
        newNode->setAttribute(attr.name(), attr.value());
    }
    newNode->m_inlineStyle = inlineStyle()->clone(document(), newNode);

    return newNode;
}

NamedNodeMap* Element::attributes()
{
    if (hasRareMembers()) {
        STARFISH_ASSERT(rareMembers()->isRareElementMembers());
        if (!rareMembers()->asRareElementMembers()->m_namedNodeMap)
            return nullptr;
    }
    RareElementMembers* rareMembers = ensureRareElementMembers();
    if (!rareMembers->m_namedNodeMap)
        rareMembers->m_namedNodeMap = new NamedNodeMap(document()->scriptBindingInstance(), this);
    return rareMembers->m_namedNodeMap;
}

RareNodeMembers* Element::ensureRareMembers()
{
    if (!hasRareMembers())
        m_rareNodeMembers = new RareElementMembers();
    STARFISH_ASSERT(m_rareNodeMembers->isRareElementMembers());
    return m_rareNodeMembers;
}

RareElementMembers* Element::ensureRareElementMembers()
{
    RareNodeMembers* rareMembers =  ensureRareMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    return rareMembers->asRareElementMembers();
}

void Element::addAttr(Attr* attr)
{
    RareElementMembers* rareMembers = ensureRareElementMembers();
    STARFISH_ASSERT(rareMembers->isRareElementMembers());
    if (!rareMembers->m_attrList)
        rareMembers->m_attrList = new (GC) AttrList();
    STARFISH_ASSERT(this->attr(attr->name()) == nullptr);
    rareMembers->m_attrList->push_back(attr);
}

Attr* Element::attr(QualifiedName name)
{
    STARFISH_ASSERT((hasRareMembers() && rareMembers()->isRareElementMembers()) || !hasRareMembers());
    if (hasRareMembers() && rareMembers()->asRareElementMembers()->m_attrList) {
        AttrList* attrList = rareMembers()->asRareElementMembers()->m_attrList;
        for (Attr* item : *attrList) {
            STARFISH_ASSERT(item);
            if (item->name() == name) {
                return item;
            }
        }
    }
    return nullptr;
}

Attr* Element::ensureAttr(QualifiedName name)
{
    STARFISH_ASSERT(hasAttribute(name) != SIZE_MAX);
    Attr* returnAttr = attr(name);
    if (!returnAttr) {
        RareElementMembers* rareMembers = ensureRareElementMembers();
        STARFISH_ASSERT(rareMembers->isRareElementMembers());
        if (!rareMembers->m_attrList)
            rareMembers->m_attrList = new (GC) AttrList();
        returnAttr = new Attr(document(), document()->scriptBindingInstance(), this, name);
        rareMembers->m_attrList->push_back(returnAttr);
    }
    return returnAttr;
}

}
