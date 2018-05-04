/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "core/dom/HTMLAnchorElement.h"

#include "StarFish.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMTokenList.h"
#include "core/dom/Event.h"
#include "core/page/Location.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

#include "browser/history/HistoryManager.h"

namespace StarFish {

void* HTMLAnchorElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLAnchorElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLAnchorElement, m_relList));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLAnchorElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLAnchorElement::name()
{
    return starFish()->staticStrings()->m_aTagName;
}

void HTMLAnchorElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* val, bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, val, attributeCreated,
                                     attributeRemoved);

    StaticStrings* ss = starFish()->staticStrings();
    if (name == ss->m_tabindex) {
        m_tabIndexWasSetExplicitly = true;
        if (m_tabIndex == -1)
            m_tabIndex = 0;
    } else if (name == ss->m_href) {
        if (attributeCreated) {
            setState(NodeStateLink, true);
        }
        if (attributeRemoved) {
            setState(NodeStateLink, false);
        }
    }
}

DOMTokenList* HTMLAnchorElement::relList()
{
    if (!m_relList) {
        m_relList = new DOMTokenList(this, starFish()->staticStrings()->m_rel);
    }
    return m_relList;
}

String* HTMLAnchorElement::referrerPolicy()
{
    return getAttributeOrEmpty(
        document()->starFish()->staticStrings()->m_referrerpolicy);
}

void HTMLAnchorElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(document()->starFish()->staticStrings()->m_referrerpolicy,
                     policy);
    }
}

bool HTMLAnchorElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }
    // TODO : Apply noreferrer
    if (event->type()->equals("click")) {
        auto href = starFish()->staticStrings()->m_href;
        Nullable<String*> hrefAttr = getAttribute(href);
        if (hrefAttr.hasValue()) {
            ResourceURL* rUrl =
                new ReferrerURL(document()->documentURI(), referrerPolicy());
            String* hrefStr = hrefAttr.getValue()->trim();
            if (hrefStr->length()) {
                if (hrefStr->startsWith("#")) {
                    window()->location()->setHash(hrefStr);
                } else if (hrefStr->startsWith("javascript:", false)) {
                    String* ret2 = toBrowserString(
                        window()->scriptBindingInstance(),
                        evaluateString(
                            window()->scriptBindingInstance(),
                            hrefStr->substring(11, hrefStr->length() - 11)));
                    if (!ret2->equalsIgnoreCase("undefined")) {
                        try {
                            GCVector<String*> value0;
                            value0.push_back(ret2);
                            document()->write(value0);
                        } catch (DOMException* e) {
                            // TODO: should throw the exception into onError
                            // event handler
                            STARFISH_RELEASE_ASSERT_NOT_REACHED();
                        }
                    }
                } else {
                    window()->location()->setLocation(hrefStr, rUrl);
                }
            } else {
                window()->location()->setLocation(
                    document()->documentURI()->urlString(), rUrl);
            }
            return true;
        }
    }
    return false;
}

bool HTMLAnchorElement::supportsFocus()
{
    auto href = starFish()->staticStrings()->m_href;
    return const_cast<HTMLAnchorElement*>(this)->hasAttribute(href) != SIZE_MAX
               ? true
               : false;
}
}
