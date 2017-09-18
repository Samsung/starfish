/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
        navigate(new ResourceURL(s, document()->documentURI()->baseURI()),
                 HistoryManager::Action::Intact, nullptr);
    } else {
        navigate(new ResourceURL(String::createASCIIString("about:blank")),
                 HistoryManager::Action::Intact, nullptr);
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

void HTMLIFrameElement::navigate(ResourceURL* url, HistoryManager::Action type,
                                 ResourceURL* referrerURL)
{
    unloadSrc();
    if (!m_historyManager) {
        m_historyManager = HistoryManager::create(this);
    }
    m_browsingContext = BrowsingContext::create(this);
    m_browsingContext->navigate(url, type, referrerURL);
}

void HTMLIFrameElement::childBrowsingContextLoaded()
{
    String* eventType = starFish()->staticStrings()->m_load.localName();
    Event* e = new Event(document(), eventType, EventInit(false, false));
    dispatchEventByUA(e);
}
}
