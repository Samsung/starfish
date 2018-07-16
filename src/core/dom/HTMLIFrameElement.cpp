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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Document.h"
#include "browser/history/HistoryManager.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Event.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

void* HTMLIFrameElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLIFrameElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLIFrameElement)] = { 0 };
        HTMLIFrameElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLIFrameElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

HTMLIFrameElement::HTMLIFrameElement(Document* document)
    : HTMLElement(document)
    , m_browsingContext(nullptr)
    , m_historyManager(nullptr)
{
    m_tabIndexWasSetExplicitly = true;
    m_tabIndex = 0;
}

QualifiedName HTMLIFrameElement::name()
{
    return starFish()->staticStrings()->m_iframeTagName;
}

void HTMLIFrameElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

String* HTMLIFrameElement::src()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_src);
}

uint32_t HTMLIFrameElement::frameWidth()
{
    String* v = width();
    if (v->length()) {
        return String::parseInt(v);
    }
    return STARFISH_DEFAULT_IFRAME_WIDTH;
}

uint32_t HTMLIFrameElement::frameHeight()
{
    String* v = height();
    if (v->length()) {
        return String::parseInt(v);
    }
    return STARFISH_DEFAULT_IFRAME_HEIGHT;
}

String* HTMLIFrameElement::width()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_width);
}

void HTMLIFrameElement::setWidth(String* width)
{
    setAttribute(starFish()->staticStrings()->m_width, width);
}

String* HTMLIFrameElement::height()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_height);
}

void HTMLIFrameElement::setHeight(String* height)
{
    setAttribute(starFish()->staticStrings()->m_height, height);
}

String* HTMLIFrameElement::scrolling()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_scrolling);
}

void HTMLIFrameElement::setScrolling(String* scrolling)
{
    setAttribute(starFish()->staticStrings()->m_scrolling, scrolling);
}

void HTMLIFrameElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_src) {
        unloadSrc();
        if (value->length() && document()->doesParticipateInRendering()) {
            loadSrc();
        }
    } else if (name == starFish()->staticStrings()->m_width ||
               name == starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    } else if (name == starFish()->staticStrings()->m_tabindex) {
        m_tabIndexWasSetExplicitly = true;
        if (m_tabIndex == -1)
            m_tabIndex = 0;
    } else if (name == starFish()->staticStrings()->m_frameborder) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == starFish()->staticStrings()->m_name) {
        if (m_browsingContext) {
            m_browsingContext->setName(value);
        }
    }
}

void HTMLIFrameElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    if (document()->doesParticipateInRendering()) {
        loadSrc();
    } else {
        unloadSrc();
    }
}

void HTMLIFrameElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    unloadSrc();
}

void HTMLIFrameElement::loadSrc()
{
    String* s = src();
    if (s->length()) {
        navigate(new ResourceURL(s, document()->baseURL()->baseURI()),
                 HistoryManager::Action::Intact, document()->documentURI());
    } else {
        navigate(new ResourceURL(String::createASCIIString("about:blank")),
                 HistoryManager::Action::Intact, document()->documentURI());
    }
}

void HTMLIFrameElement::unloadSrc()
{
    if (m_browsingContext) {
        m_browsingContext->dispose();
        m_browsingContext = nullptr;
    }
}

Document* HTMLIFrameElement::contentDocument() const
{
    if (m_browsingContext) {
        return m_browsingContext->document();
    }
    return nullptr;
}

Window* HTMLIFrameElement::contentWindow() const
{
    if (m_browsingContext) {
        return m_browsingContext->window();
    }
    return nullptr;
}

String* HTMLIFrameElement::referrerPolicy()
{
    return getAttributeOrEmpty(
        document()->starFish()->staticStrings()->m_referrerpolicy);
}

void HTMLIFrameElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(document()->starFish()->staticStrings()->m_referrerpolicy,
                     policy);
    }
}

void HTMLIFrameElement::navigate(ResourceURL* url, HistoryManager::Action type,
                                 ResourceURL* referrerURL)
{
    if (ResourceURL::isValidURL(url->urlString())) {
        unloadSrc();

        if (!m_historyManager) {
            m_historyManager = HistoryManager::create(this);
        }

        ResourceURL* rUrl = referrerURL;
        if (!rUrl->isReferrerURL()) {
            rUrl = new ReferrerURL(rUrl, referrerPolicy());
        }
        if (m_browsingContext) {
            m_browsingContext->dispose();
        }
        m_browsingContext = BrowsingContext::create(this);
        m_browsingContext->setName(nameAttr());
        m_browsingContext->open(url, type, rUrl);
    }
}

void HTMLIFrameElement::childBrowsingContextLoaded()
{
    String* eventType = starFish()->staticStrings()->m_load.localName();
    Event* e = new Event(document(), eventType, EventInit(false, false));
    dispatchEventByUA(this, e, true);
}

void HTMLIFrameElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);
    size_t idx = hasAttribute(starFish()->staticStrings()->m_frameborder);
    if (idx != SIZE_MAX) {
        String* val = getAssuredAttribute(idx);
        if (val->equals("0") || val->equalsIgnoreCase("none")) {
            CSSStyleValuePair pair;
            pair.setLengthValue(CSSLength(0));
            pair.setKeyKind(CSSStyleValuePair::BorderTopWidth);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::BorderRightWidth);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::BorderBottomWidth);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::BorderLeftWidth);
            cssValues.push_back(pair);
        }
    }
}

String* HTMLIFrameElement::nameAttr()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_name);
}

void HTMLIFrameElement::setNameAttr(String* name)
{
    setAttribute(starFish()->staticStrings()->m_name, name);
}
}
