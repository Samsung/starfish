/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "core/dom/HTMLTextEditable.h"
#include "core/dom/CompositionEvent.h"
#include "core/dom/Document.h"
#include "core/dom/KeyboardEvent.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "platform/window/PlatformWindow.h"

namespace StarFish {

// TODO HTMLInputElement will inherit this class

// TODO: We should discuss the maxlength limitation
// because the spec doesn't describe the actual number.
// 524288 is Chromium's
static const int INITIAL_MAXLENGTH = 524288;

HTMLTextEditable::HTMLTextEditable(Document* document)
    : HTMLFormControl(document)
    , m_dirtyValueFlag(false)
    , m_shouldDrawCaret(false)
    , m_caretBlinkingIntervalId(SIZE_MAX)
    , m_currentCaretPosition(0)
    , m_currentEditingText(String::emptyString)
    , m_maxlength(INITIAL_MAXLENGTH)
{
}

bool HTMLTextEditable::readonly() const
{
    return getAttribute(starFish()->staticStrings()->m_readonly).hasValue();
}

void HTMLTextEditable::didStateChanged(int oldState, int newState)
{
    HTMLElement::didStateChanged(oldState, newState);

    if (disabled()) {
        return;
    }

    bool oldGotFocus = oldState & Node::NodeStateFocused;
    bool newGotFocus = newState & Node::NodeStateFocused;

    if (isEditableType()) {
        if (!oldGotFocus && newGotFocus) {
            m_currentCaretPosition = textValue()->length();
            m_caretBlinkingIntervalId = window()->setInterval(
                [](Window* window, void* data) {
                    HTMLTextEditable* e = (HTMLTextEditable*)data;
                    e->m_shouldDrawCaret = !e->m_shouldDrawCaret;
                    e->setNeedsPainting();
                },
                500, this);
            starFish()->platformWindow()->showSoftwareKeyboardIfPossible();
            setNeedsFrameTreeBuildWithoutSelf();
        } else if (oldGotFocus && !newGotFocus) {
            starFish()->platformWindow()->hideSoftwareKeyboardIfPossible();
            m_shouldDrawCaret = false;
            m_currentCaretPosition = 0;
            m_currentEditingText = String::emptyString;
            window()->clearInterval(m_caretBlinkingIntervalId);
        }
    }
}

LayoutUnit HTMLTextEditable::caretThickness() const
{
    return LayoutUnit(CARET_THICKNESS / window()->devicePixelRatio());
}

String* HTMLTextEditable::placeholder()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_placeholder);
}

bool HTMLTextEditable::handleDefaultEvent(Event* event)
{
    if (HTMLFormControl::handleDefaultEvent(event)) {
        return true;
    }

    if (isDisabled() || !isEditableType()) {
        return false;
    }

    if ((event->isMouseEvent() && event->type()->equals("mousedown")) ||
        (event->isTouchEvent() && event->type()->equals("touchstart"))) {
        return true;
    }

    if (document()->browsingContext()->focusedNode() != this) {
        return false;
    }

    String* value = textValue();
    m_currentCaretPosition = std::min(m_currentCaretPosition, value->length());

    String* oldValue = value;
    if (event->isKeyboardEvent() && event->type()->equals("keydown")) {
        bool isUseful = false;
        switch (event->asKeyboardEvent()->keyValue()) {
        case KeyValue::BackspaceKey: {
            if (value->length()) {
                if (m_currentCaretPosition > 0) {
                    StringBuilder sb;
                    sb.appendSubString(value, 0, m_currentCaretPosition - 1);
                    sb.appendSubString(value, m_currentCaretPosition,
                                       value->length());
                    value = sb.finalize();
                    m_currentCaretPosition--;
                    m_shouldDrawCaret = true;
                }
                isUseful = true;
            }
        } break;
        case KeyValue::DeleteKey: {
            if (value->length()) {
                if (m_currentCaretPosition >= 0 &&
                    m_currentCaretPosition < value->length()) {
                    StringBuilder sb;
                    sb.appendSubString(value, 0, m_currentCaretPosition);
                    sb.appendSubString(value, m_currentCaretPosition + 1,
                                       value->length());
                    value = sb.finalize();
                    m_shouldDrawCaret = true;
                }
                isUseful = true;
            }
        } break;
        case KeyValue::EnterKey: {
            if (m_currentCaretPosition < (size_t)maxLength()) {
                value = value->concat('\n');
                m_currentCaretPosition++;
                m_shouldDrawCaret = true;
                isUseful = true;
            }
        } break;
        default: {
            if (String::isASCIIPrintableKey(
                    event->asKeyboardEvent()->keyValue()) &&
                m_currentCaretPosition < (size_t)maxLength()) {
                char key = (char)event->asKeyboardEvent()->keyValue();
                value = value->concat(key);
                m_currentCaretPosition++;
                m_shouldDrawCaret = true;
                isUseful = true;
            }
        } break;
        }

        if (isUseful) {
            if (!value->equals(oldValue)) {
                setValue(value);
            }
            return true;
        }
    } else if (event->isCompositionEvent()) {
        if (event->type()->equals("compositionstart")) {
        } else if (event->type()->equals("compositionupdate")) {
            value = value->remove(m_currentCaretPosition,
                                  m_currentEditingText->length());
            m_currentEditingText = event->asCompositionEvent()->data();
            value = value->insert(m_currentEditingText, m_currentCaretPosition);
            m_shouldDrawCaret = true;
        } else if (event->type()->equals("compositionend") &&
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
            setValue(value);
        }
        return true;
    }
    return false;
}
}
