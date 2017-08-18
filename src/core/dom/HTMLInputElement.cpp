/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "core/dom/HTMLInputElement.h"
#include "core/dom/Document.h"
#include "core/dom/Text.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/HTMLFormElement.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/layout/FrameInputBox.h"

namespace StarFish {

HTMLInputElement::HTMLInputElement(Document* document)
    : HTMLElement(document)
    , m_shouldDrawCaret(false)
    , m_caretBlinkingIntervalId(SIZE_MAX)
    , m_currentCaretPosition(SIZE_MAX)
    , m_currentEditingText(String::emptyString)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
    setTabIndex(0, false);
}

void* HTMLInputElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLInputElement)] = { 0 };
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLInputElement, m_currentEditingText));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLInputElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLInputElement::name()
{
    return starFish()->staticStrings()->m_inputTagName;
}

String* HTMLInputElement::domName()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_name);
}

void HTMLInputElement::setDomName(String* name)
{
    setAttribute(starFish()->staticStrings()->m_name, name);
}

String* HTMLInputElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLInputElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

String* HTMLInputElement::value()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_value);
}

void HTMLInputElement::setValue(String* value)
{
    setAttribute(starFish()->staticStrings()->m_value, value);
}

String* HTMLInputElement::formEnctype()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formEnctype);
}

void HTMLInputElement::setFormEnctype(String* enctype)
{
    setAttribute(starFish()->staticStrings()->m_formEnctype, enctype);
}

String* HTMLInputElement::formMethod()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formMethod);
}

void HTMLInputElement::setFormMethod(String* method)
{
    setAttribute(starFish()->staticStrings()->m_formMethod, method);
}

String* HTMLInputElement::formTarget()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formTarget);
}

void HTMLInputElement::setFormTarget(String* target)
{
    setAttribute(starFish()->staticStrings()->m_formTarget, target);
}

String* HTMLInputElement::formAction()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_formAction);
}

void HTMLInputElement::setFormAction(String* formAction)
{
    setAttribute(starFish()->staticStrings()->m_formAction, formAction);
}

HTMLFormElement* HTMLInputElement::form()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p == nullptr) {
            break;
        } else if (p->isHTMLIFrameElement()) {
            return nullptr;
        } else if (p->isHTMLFormElement()) {
            return p->asHTMLFormElement();
        }
    }
    return nullptr;
}

String* HTMLInputElement::obscurePhrase(String* phrase)
{
    std::string s(phrase->length(), '*');
    return String::createASCIIString(s.c_str());
}

void HTMLInputElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);

    if (starFish()->staticStrings()->m_value == name) {
        if (type()->equalsWithoutCase("password")) {
            value = obscurePhrase(value);
        }

        if (frame() && !document()->browsingContext()->needsLayout()) {
            auto box = frame()->asFrameInputBox();
            box->firstChild()->asFrameText()->node()->asText()->setData(value);
            // Do partial layout for performance
            LayoutContext ctx(starFish(),
                              document()->frame()->asFrameDocument());
            box->layout(ctx, Frame::ResolveAll);
            setNeedsPainting();
        } else if (frame()) {
            auto box = frame()->asFrameInputBox();
            box->firstChild()->asFrameText()->node()->asText()->setData(value);
            setNeedsLayout();
        } else if (document()->doesParticipateInRendering()) {
            setNeedsFrameTreeBuild();
        }
    }
}

bool HTMLInputElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (((event->isMouseEvent() || event->isTouchEvent())) &&
        event->type()->equalsWithoutCase("click")) {
        if (type()->equalsWithoutCase("submit") ||
            type()->equalsWithoutCase("button")) {
            HTMLFormElement* formNode = form();
            if (formNode) {
                formNode->setSubmitter(this);
                formNode->submit();
                return true;
            }
        }
    } else if (event->isKeyboardEvent() &&
               document()->browsingContext()->focusedNode() == this &&
               event->type()->equalsWithoutCase("keydown")) {
        if (isUserKeyboardInputAllowed()) {
            String* value =
                getAttributeOrEmpty(starFish()->staticStrings()->m_value);
            String* oldValue = value;
            if (event->asKeyboardEvent()->keyValue() ==
                KeyValue::BackspaceKey) {
                if (value->length()) {
                    if (m_currentCaretPosition > 0) {
                        StringBuilder sb;
                        sb.appendSubString(value, 0,
                                           m_currentCaretPosition - 1);
                        sb.appendSubString(value, m_currentCaretPosition,
                                           value->length());
                        value = sb.finalize();
                        m_currentCaretPosition--;
                    }
                }
            } else if (event->asKeyboardEvent()->keyCode()) {
                value = value->concat(
                    (char32_t)event->asKeyboardEvent()->keyCode());
                m_currentCaretPosition++;
            }
            if (!value->equals(oldValue)) {
                setAttribute(starFish()->staticStrings()->m_value, value);
            }
            return true;
        }
    }
    return false;
}

void HTMLInputElement::didStateChanged(int oldState, int newState)
{
    HTMLElement::didStateChanged(oldState, newState);

    bool oldGotFocus = oldState & Node::NodeStateFocused;
    bool newGotFocus = newState & Node::NodeStateFocused;

    if (isUserKeyboardInputAllowed()) {
        if (!oldGotFocus && newGotFocus) {
            String* value =
                getAttributeOrEmpty(starFish()->staticStrings()->m_value);
            m_currentCaretPosition = value->length();
            window()->setInterval(
                [](Window* window, void* data) {
                    HTMLInputElement* e = (HTMLInputElement*)data;
                    e->m_shouldDrawCaret = !e->m_shouldDrawCaret;
                    e->setNeedsPainting();
                },
                500, this);
        } else if (oldGotFocus && !newGotFocus) {
            m_currentCaretPosition = SIZE_MAX;
            window()->clearInterval(m_caretBlinkingIntervalId);
        }
    }
}

bool HTMLInputElement::supportsFocus() const
{
    auto type = starFish()->staticStrings()->m_type;
    return !const_cast<HTMLInputElement*>(this)
                ->getAttributeOrEmpty(type)
                ->equalsWithoutCase("hidden");
}

bool HTMLInputElement::isUserKeyboardInputAllowed()
{
    String* typeString = type();
    if (typeString->equals("") || typeString->equalsWithoutCase("text")) {
        return true;
    } else if (typeString->equals("email")) {
        return true;
    } else if (typeString->equals("number")) {
        return true;
    } else if (typeString->equals("password")) {
        return true;
    } else if (typeString->equals("url")) {
        return true;
    } else if (typeString->equals("tel")) {
        return true;
    } else if (typeString->equals("search")) {
        return true;
    }
    return false;
}
}
