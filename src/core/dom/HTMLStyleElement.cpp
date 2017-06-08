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

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLStyleElement.h"
#include "core/dom/Text.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/window/Window.h"

namespace StarFish {

bool isCSSType(const char* type)
{
    if (strcmp("", type) == 0) {
        return true;
    } else if (strcmp("text/css", type) == 0) {
        return true;
    }
    return false;
}

String* HTMLStyleElement::localName()
{
    return starFish()->staticStrings()->m_styleTagName.localName();
}

QualifiedName HTMLStyleElement::name()
{
    return starFish()->staticStrings()->m_styleTagName;
}

String* HTMLStyleElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLStyleElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

String* HTMLStyleElement::media()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_media);
}

void HTMLStyleElement::setMedia(String* media)
{
    setAttribute(starFish()->staticStrings()->m_media, media);
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

void HTMLStyleElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    generateStyleSheet();
    dispatchLoadEvent();
}

void HTMLStyleElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
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

    CSSParser parser(document());

    String* str = String::emptyString;

    Node* child = firstChild();
    while (child) {
        if (child->isCharacterData() && child->asCharacterData()->isText()) {
            str = str->concat(child->asCharacterData()->data());
        }
        child = child->nextSibling();
    }

    CSSToken* token = parser.makeToken(media());
    MediaQuerySet* mediaQuerySet = parser.parseMediaQuery();
    const MediaQueryEvaluator& evaluator =
        document()->styleResolver().mediaQueryEvaluator();
    if (evaluator.eval(mediaQuerySet)) {
        CSSStyleSheet* sheet = new CSSStyleSheet(this, str);
        m_generatedSheet = sheet;
        document()->styleResolver().addSheet(sheet);
        window()->setWholeDocumentNeedsStyleRecalc();
    }
}

void HTMLStyleElement::removeStyleSheet()
{
    if (m_generatedSheet) {
        document()->styleResolver().removeSheet(m_generatedSheet);
        window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLStyleElement::dispatchLoadEvent()
{
    starFish()->messageLoop()->addIdler(
        [](size_t handle, void* data) {
            HTMLStyleElement* element = (HTMLStyleElement*)data;
            if (!element->hasLoaded()) {
                String* eventType =
                    element->starFish()->staticStrings()->m_load.localName();
                Event* e = new Event(eventType, EventInit(false, false));
                element->dispatchEvent(e);
                element->setLoaded();
            }
        },
        this);
}
}
