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

#include "core/dom/Event.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Text.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/InputEvent.h"
#include "core/dom/CompositionEvent.h"
#include "core/dom/HTMLFormElement.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/layout/FrameInputBox.h"

namespace StarFish {

// TODO: We should discuss the maxlength limitation
// because the spec doesn't describe the actual number.
// 524288 is Chromium's
static const int INITIAL_MAXLENGTH = 524288;

HTMLInputElement::HTMLInputElement(Document* document)
    : HTMLFormControl(document)
    , m_checkness(false)
    , m_dirtiness(false)
    , m_shouldDrawCaret(false)
    , m_caretBlinkingIntervalId(SIZE_MAX)
    , m_currentCaretPosition(0)
    , m_currentEditingText(String::emptyString)
    , m_maxlength(INITIAL_MAXLENGTH)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
}

void* HTMLInputElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLInputElement)] = { 0 };
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLInputElement, m_currentEditingText));
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLInputElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLInputElement::name()
{
    return starFish()->staticStrings()->m_inputTagName;
}

bool HTMLInputElement::canHaveValue()
{
    String* typeString = type();
    if (typeString->equals("")) {
        return true;
    } else if (typeString->equals("text")) {
        return true;
    } else if (typeString->equals("submit")) {
        return true;
    } else if (typeString->equals("button")) {
        return true;
    } else if (typeString->equals("email")) {
        return true;
    } else if (typeString->equals("password")) {
        return true;
    } else if (typeString->equals("checkbox")) {
        return true;
    } else if (typeString->equals("search")) {
        return true;
    } else if (typeString->equals("tel")) {
        return true;
    } else if (typeString->equals("url")) {
        return true;
    }

    return false;
}

String* HTMLInputElement::type()
{
    String* typeAttr = getAttributeOrEmpty(starFish()->staticStrings()->m_type);
    typeAttr = typeAttr->toASCIILower();

    if (typeAttr->equals("hidden") || typeAttr->equals("text") ||
        typeAttr->equals("search") || typeAttr->equals("tel") ||
        typeAttr->equals("url") || typeAttr->equals("email") ||
        typeAttr->equals("password") || typeAttr->equals("date") ||
        typeAttr->equals("month") || typeAttr->equals("week") ||
        typeAttr->equals("time") || typeAttr->equals("datetime-local") ||
        typeAttr->equals("number") || typeAttr->equals("range") ||
        typeAttr->equals("color") || typeAttr->equals("checkbox") ||
        typeAttr->equals("radio") || typeAttr->equals("file") ||
        typeAttr->equals("submit") || typeAttr->equals("image") ||
        typeAttr->equals("reset") || typeAttr->equals("button")) {
        return typeAttr;
    }

    return starFish()->staticStrings()->m_text.localName();
}

String* HTMLInputElement::defaultValue()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_value);
}

void HTMLInputElement::setDefaultValue(String* defaultValue)
{
    setAttribute(starFish()->staticStrings()->m_value, defaultValue);
    m_dirtiness = true;
}

String* HTMLInputElement::value()
{
    if (!m_dirtiness) {
        return defaultValue();
    }

    return HTMLFormControl::value();
}

// TODO:
// https://html.spec.whatwg.org/multipage/input.html#value-sanitization-algorithm
void HTMLInputElement::setValue(String* val)
{
    String* oldValue = value();
    m_value = val;
    m_dirtiness = true;

    if (type()->equals("date") || type()->equals("month") ||
        type()->equals("week") || type()->equals("time") ||
        type()->equals("datetime-local")) {
        // TODO
    } else if (type()->equals("number") || type()->equals("range")) {
        // TODO
    }

    if (!oldValue->equals(val) && m_currentCaretPosition > 0) {
        m_currentCaretPosition = val->length();
    }
}

String* HTMLInputElement::checkboxTickSymbol()
{
    return String::createUTF32String(U'\u2714'); // tick
}

String* HTMLInputElement::placeholder()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_placeholder);
}

void HTMLInputElement::setPlaceholder(String* value)
{
    setAttribute(starFish()->staticStrings()->m_placeholder, value);
}

void HTMLInputElement::toggleChecked()
{
    if (checked()) {
        setChecked(false);
    } else {
        setChecked(true);
    }

    auto fn = [](size_t handle, void* data) {
        HTMLInputElement* element = (HTMLInputElement*)data;
        String* eventType =
            element->starFish()->staticStrings()->m_change.localName();
        Event* e =
            new Event(element->document(), eventType, EventInit(true, false));
        element->EventTarget::dispatchEventByUA(element, e);
    };
    starFish()->messageLoop()->addIdler(document()->browsingContext(), fn,
                                        this);
}

bool HTMLInputElement::defaultChecked()
{
    Nullable<String*> val =
        getAttribute(starFish()->staticStrings()->m_checked);

    return val.hasValue();
}

void HTMLInputElement::setDefaultChecked(bool checked)
{
    if (checked) {
        setAttribute(starFish()->staticStrings()->m_checked,
                     String::emptyString);
    } else {
        removeAttribute(starFish()->staticStrings()->m_checked);
    }
}

bool HTMLInputElement::checked()
{
    return m_checkness;
}

void HTMLInputElement::setChecked(bool checked)
{
    m_checkness = checked;
    m_dirtiness = true;
    updateInputboxValue(m_checkness ? checkboxTickSymbol()
                                    : String::emptyString);
}

uint32_t HTMLInputElement::size()
{
    String* size = getAttributeOrEmpty(starFish()->staticStrings()->m_size);
    if (!size->equals(String::emptyString)) {
        return String::parseInt64(size);
    }

    return DEFAULT_SIZE;
}

void HTMLInputElement::setSize(String* sizeStr)
{
    int size = String::parseInt64(sizeStr);
    if (size == 0) {
        COMPOSE_MESSAGE(reason, INVALID_SIZE, "0");
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "size", "HTMLInputElement",
                        reason);
        throw new DOMException(document(), DOMException::Code::INDEX_SIZE_ERR,
                               msg);
    } else if (size < 0) {
        setAttribute(starFish()->staticStrings()->m_size,
                     String::fromInt(DEFAULT_SIZE));
    } else {
        setAttribute(starFish()->staticStrings()->m_size, sizeStr);
    }
}

bool HTMLInputElement::isSizableType()
{
    String* typeString = type();
    if (typeString->equals("text")) {
        return true;
    } else if (typeString->equals("email")) {
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

String* HTMLInputElement::obscurePhrase(String* phrase)
{
    StringBuilder sb;
    for (size_t i = 0; i < phrase->length(); i++) {
        sb.appendChar(U'\u25CF'); // block circle
    }
    return sb.finalize();
}

void HTMLInputElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* val, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLFormControl::didAttributeChanged(name, old, val, attributeCreated,
                                         attributeRemoved);

    if (name == starFish()->staticStrings()->m_type ||
        name == starFish()->staticStrings()->m_value) {
        if (name == starFish()->staticStrings()->m_type || !old->equals(val)) {
            String* textToDisplay = visibleValue();
            updateInputboxValue(textToDisplay);
        }

        if (name == starFish()->staticStrings()->m_value) {
            // https://html.spec.whatwg.org/multipage/input.html#attr-input-value
            if (!m_dirtiness) {
                if (attributeCreated) {
                    m_value = val;
                } else if (attributeRemoved) {
                    m_value = String::emptyString;
                }
                m_dirtiness = true;
            }

            // TODO: fire correct inputevent
            // TODO: we should fire this event in handleDefaultEvent
            InputEvent* event =
                new InputEvent(document(), String::createASCIIString("input"));
            event->setCancelable(false);
            event->setBubbles(true);
            event->setComposed(true);
            event->setData(val);
            event->setInputType(String::createASCIIString("insertText"));
            dispatchEventByUA(event);
        }
    } else if (name == starFish()->staticStrings()->m_checked) {
        if (val->equals("checked") || val->equals(String::emptyString)) {
            setChecked(true);
        }
    }
}

String* HTMLInputElement::visibleValue()
{
    String* val = value();
    String* typeVal = type();

    if (typeVal->equals("submit") && val->equals(String::emptyString)) {
        val = String::createASCIIString("submit");
    } else if (typeVal->equals("password") &&
               !val->equals(String::emptyString)) {
        val = HTMLInputElement::obscurePhrase(val);
    } else if (typeVal->equals("checkbox")) {
        val = checked() ? HTMLInputElement::checkboxTickSymbol()
                        : String::emptyString;
    } else if (shouldUsePlaceholder()) {
        val = placeholder();
    }

    return val;
}

void HTMLInputElement::updateInputboxValue(String* value)
{
    setNeedsFrameTreeBuild(Node::UpdateFromParent);
}

bool HTMLInputElement::shouldUsePlaceholder()
{
    if (isEditableType() && value()->equals(String::emptyString) &&
        !placeholder()->equals(String::emptyString)) {
        return true;
    }
    return false;
}

bool HTMLInputElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (isDisabled()) {
        return false;
    }

    if (event->isMouseEvent() || event->isTouchEvent()) {
        if (event->type()->equalsIgnoreCase("click")) {
            if (type()->equals("submit")) {
                fireSubmitEvent();
                return true;
            } else if (type()->equals("checkbox")) {
                toggleChecked();
                return true;
            } else if (shouldUsePlaceholder()) {
                setValue(String::emptyString);
                return true;
            }
        } else if (event->type()->equals("mousedown") ||
                   event->type()->equals("touchstart")) {
            if (isEditableType()) {
                return true;
            }
        }
    } else {
        if (document()->browsingContext()->focusedNode() == this &&
            isEditableType()) {
            String* value =
                getAttributeOrEmpty(starFish()->staticStrings()->m_value);
            m_currentCaretPosition =
                std::min(m_currentCaretPosition, value->length());

            String* oldValue = value;
            if (event->isKeyboardEvent() &&
                event->type()->equalsIgnoreCase("keydown")) {
                bool isUseful = false;
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
                            m_shouldDrawCaret = true;
                        }
                        isUseful = true;
                    }
                } else {
                    if (event->asKeyboardEvent()->isASCIIVisibleChar() &&
                        m_currentCaretPosition < (size_t)maxLength()) {
                        char key = (char)event->asKeyboardEvent()->keyValue();
                        value = value->concat(key);
                        m_currentCaretPosition++;
                        m_shouldDrawCaret = true;
                        isUseful = true;
                    }
                }
                if (isUseful) {
                    if (!value->equals(oldValue)) {
                        setAttribute(starFish()->staticStrings()->m_value,
                                     value);
                    }
                    return true;
                }
            } else if (event->isCompositionEvent()) {
                if (event->type()->equalsIgnoreCase("compositionstart")) {
                } else if (event->type()->equalsIgnoreCase(
                               "compositionupdate")) {
                    value = value->remove(m_currentCaretPosition,
                                          m_currentEditingText->length());
                    m_currentEditingText = event->asCompositionEvent()->data();
                    value = value->insert(m_currentEditingText,
                                          m_currentCaretPosition);
                    m_shouldDrawCaret = true;
                } else if (event->type()->equalsIgnoreCase("compositionend") &&
                           m_currentCaretPosition < (size_t)maxLength()) {
                    value = value->remove(m_currentCaretPosition,
                                          m_currentEditingText->length());
                    value = value->insert(event->asCompositionEvent()->data(),
                                          m_currentCaretPosition);
                    m_currentEditingText = String::emptyString;
                    m_currentCaretPosition +=
                        event->asCompositionEvent()->data()->length();
                    m_shouldDrawCaret = true;
                }
                if (!value->equals(oldValue)) {
                    setAttribute(starFish()->staticStrings()->m_value, value);
                }
                return true;
            }
        }
    }
    return false;
}

void HTMLInputElement::didStateChanged(int oldState, int newState)
{
    HTMLElement::didStateChanged(oldState, newState);

    if (disabled()) {
        return;
    }

    bool oldGotFocus = oldState & Node::NodeStateFocused;
    bool newGotFocus = newState & Node::NodeStateFocused;

    if (isEditableType()) {
        if (!oldGotFocus && newGotFocus) {
            String* value = visibleValue();
            m_currentCaretPosition =
                shouldUsePlaceholder() ? 0 : value->length();
            m_caretBlinkingIntervalId = window()->setInterval(
                [](Window* window, void* data) {
                    HTMLInputElement* e = (HTMLInputElement*)data;
                    e->m_shouldDrawCaret = !e->m_shouldDrawCaret;
                    e->setNeedsPainting();
                },
                500, this);
            starFish()->platformWindow()->showSoftwareKeyboardIfPossible();
            updateInputboxValue(value);
        } else if (oldGotFocus && !newGotFocus) {
            starFish()->platformWindow()->hideSoftwareKeyboardIfPossible();
            m_shouldDrawCaret = false;
            m_currentCaretPosition = 0;
            m_currentEditingText = String::emptyString;
            window()->clearInterval(m_caretBlinkingIntervalId);
        }
    }
}

bool HTMLInputElement::supportsFocus()
{
    return !type()->equals("hidden");
}

LayoutUnit HTMLInputElement::caretThickness() const
{
    return LayoutUnit(CARET_THICKNESS / window()->devicePixelRatio());
}

bool HTMLInputElement::isEditableType()
{
    String* typeString = type();
    if (typeString->equals("text")) {
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

void HTMLInputElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    if (isSizableType()) {
        auto siz = size();
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::Width);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Length);
        pair.setLengthValue(CSSLength(CSSLength::EM, siz));
        cssValues.push_back(pair);
    }
}

int32_t HTMLInputElement::maxLength()
{
    int32_t result = 0;
    String* maxLengthStr =
        getAttributeOrEmpty(starFish()->staticStrings()->m_maxlength);

    if (maxLengthStr->equals(String::emptyString)) {
        result = -1;
    } else {
        result = String::parseInt(maxLengthStr);
        if (result < 0) {
            result = -1;
        }
    }
    return result;
}

void HTMLInputElement::setMaxLength(int32_t maxlength)
{
    if (maxlength < 0) {
        COMPOSE_MESSAGE(reason, NOT_POSITIVE,
                        String::fromInt(maxlength)->toUTF8NonGCString().data());
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "maxLength",
                        "HTMLInputElement", reason);
        throw new DOMException(document(), DOMException::DOM_EXCEPTION, msg);
    } else {
        setAttribute(starFish()->staticStrings()->m_maxlength,
                     String::fromInt(maxlength));
    }
}
}
