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
#include "StarFish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGStyleElement.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/Text.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace StarFish {

void* SVGStyleElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGStyleElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGStyleElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGStyleElement, m_generatedSheet));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGStyleElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName SVGStyleElement::name()
{
    return starFish()->staticStrings()->m_svgstyleTagName;
}

bool isCSSType(const char* type);

String* SVGStyleElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void SVGStyleElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

String* SVGStyleElement::media()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_media);
}

void SVGStyleElement::setMedia(String* media)
{
    setAttribute(starFish()->staticStrings()->m_media, media);
}

void SVGStyleElement::didCharacterDataModified(String* before, String* after)
{
    SVGElement::didCharacterDataModified(before, after);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        removeStyleSheet();
        generateStyleSheet();
    }
}

void SVGStyleElement::didNodeInserted(Node* parent, Node* newChild)
{
    SVGElement::didNodeInserted(parent, newChild);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        removeStyleSheet();
        generateStyleSheet();
    }
}

void SVGStyleElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    SVGElement::didNodeInserted(parent, oldChild);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        removeStyleSheet();
        generateStyleSheet();
    }
}

void SVGStyleElement::didNodeInsertedToDocumentTree()
{
    SVGElement::didNodeInsertedToDocumentTree();
    generateStyleSheet();
    dispatchLoadEvent();
}

void SVGStyleElement::didNodeRemovedFromDocumentTree()
{
    SVGElement::didNodeRemovedFromDocumentTree();
    removeStyleSheet();
}

void SVGStyleElement::generateStyleSheet()
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

    CSSStyleSheet* sheet = new CSSStyleSheet(this, str);
    sheet->parseSheetIfneeds();
    m_generatedSheet = sheet;
    document()->styleResolver().addSheet(sheet);

    CSSParser parser(document());
    parser.makeToken(media());
    MediaQuerySet* mediaQuerySet = parser.parseMediaQuery();
    sheet->setMediaQuerySet(mediaQuerySet);
    const MediaQueryEvaluator& evaluator =
        document()->styleResolver().mediaQueryEvaluator();
    if (evaluator.eval(mediaQuerySet)) {
        window()->browsingContext()->setNeedsStyleSheetsRecalc();
    }
}

void SVGStyleElement::removeStyleSheet()
{
    if (m_generatedSheet) {
        document()->styleResolver().removeSheet(m_generatedSheet);
        window()->browsingContext()->setNeedsStyleSheetsRecalc();
        m_generatedSheet = nullptr;
    }
}

void SVGStyleElement::dispatchLoadEvent()
{
    starFish()->messageLoop()->addIdler(
        document()->browsingContext(),
        [](size_t handle, void* data) {
            SVGStyleElement* element = (SVGStyleElement*)data;
            if (!element->hasLoaded()) {
                String* eventType =
                    element->starFish()->staticStrings()->m_load.localName();
                Event* e = new Event(element->document(), eventType,
                                     EventInit(false, false));
                element->dispatchEventByUA(e);
                element->setLoaded();
            }
        },
        this);
}
}
