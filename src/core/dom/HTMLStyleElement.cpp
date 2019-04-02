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

#include "StarfishConfig.h"

#include "core/dom/HTMLStyleElement.h"

#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/Text.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/csp/ContentSecurityPolicy.h"

namespace Starfish {

void* HTMLStyleElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLStyleElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLStyleElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLStyleElement, m_generatedSheet));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLStyleElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool isCSSType(const char* type)
{
    if (strcmp("", type) == 0) {
        return true;
    } else if (strcmp("text/css", type) == 0) {
        return true;
    }
    return false;
}

bool HTMLStyleElement::disabled()
{
    if (!m_generatedSheet) {
        return false;
    }
    return m_generatedSheet->disabled();
}

void HTMLStyleElement::setDisabled(bool disabled)
{
    if (!m_generatedSheet) {
        return;
    }
    m_generatedSheet->setDisabled(disabled);
}

StyleSheet* HTMLStyleElement::sheet()
{
    return m_generatedSheet;
}

void HTMLStyleElement::didCharacterDataModified(String* before, String* after)
{
    HTMLElement::didCharacterDataModified(before, after);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        removeStyleSheet();
        generateStyleSheet();
    }
}

void HTMLStyleElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        removeStyleSheet();
        generateStyleSheet();
    }
}

void HTMLStyleElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    HTMLElement::didNodeInserted(parent, oldChild);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        removeStyleSheet();
        generateStyleSheet();
    }
}

void HTMLStyleElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    generateStyleSheet();
    dispatchLoadEvent();
}

void HTMLStyleElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeRemovedFromDocumentTree();
    removeStyleSheet();
}

void HTMLStyleElement::generateStyleSheet()
{
    STARFISH_ASSERT(isInDocumentScopeAndDocumentParticipateInRendering());

    if (m_inParsing) {
        return;
    }

    if (m_generatedSheet) {
        removeStyleSheet();
    }

    String* str = String::emptyString;
    Node* child = firstChild();
    while (child) {
        if (child->isCharacterData() && child->asCharacterData()->isText()) {
            str = str->concat(child->asCharacterData()->data());
        }
        child = child->nextSibling();
    }

    if (str->length() > 0 &&
        !document()->contentSecurityPolicy()->allowInline(
            CSPDirectives::StyleSrc, str, nonce())) {
        String* eventType = starfish()->staticStrings()->m_error.localName();
        Event* e = new Event(document()->executionContext(), eventType,
                             EventInit(false, false));
        dispatchEventIdleTimeByUA(e);
        return;
    }

    CSSStyleSheet* sheet = new CSSStyleSheet(this, str);
    sheet->parseSheetIfneeds();
    m_generatedSheet = sheet;
    document()->styleResolver().addSheet(sheet);

    CSSParser parser(document());
    parser.makeToken(getAttributeOrEmpty(starfish()->staticStrings()->m_media));
    MediaQuerySet* mediaQuerySet = parser.parseMediaQuery();
    sheet->setMediaQuerySet(mediaQuerySet);
    const MediaQueryEvaluator& evaluator =
        document()->styleResolver().mediaQueryEvaluator();
    if (evaluator.eval(mediaQuerySet)) {
        window()->browsingContext()->setNeedsStyleSheetsRecalc();
        m_generatedSheet->willAddToDocument();
    }
}

void HTMLStyleElement::removeStyleSheet()
{
    if (m_generatedSheet) {
        m_generatedSheet->willRemovedFromDocument();
        document()->styleResolver().removeSheet(m_generatedSheet);
        window()->browsingContext()->setNeedsStyleSheetsRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLStyleElement::dispatchLoadEvent()
{
    webView()->messageLoop()->addIdler(
        window(),
        [](size_t handle, void* data) {
            HTMLStyleElement* element = (HTMLStyleElement*)data;
            if (!element->hasLoaded()) {
                String* eventType =
                    element->starfish()->staticStrings()->m_load.localName();
                Event* e = new Event(element->document()->executionContext(),
                                     eventType, EventInit(false, false));
                element->dispatchEventByUA(e);
                element->setLoaded();
            }
        },
        this);
}

String* HTMLStyleElement::nonce()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_nonce);
}

void HTMLStyleElement::setNonce(String* str)
{
    setAttribute(starfish()->staticStrings()->m_nonce, str);
}
}
