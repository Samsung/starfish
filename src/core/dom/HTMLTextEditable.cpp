/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
            if (!shouldUsePlaceholder()) {
                m_currentCaretPosition = visibleValue()->length();
            }
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

    String* value = visibleValue();
    m_currentCaretPosition = std::min(m_currentCaretPosition, value->length());

    String* oldValue = value;
    if (event->isKeyboardEvent() && event->type()->equals("keydown")) {
        bool isUseful = false;
        switch (event->asKeyboardEvent()->keyValue()) {
        case KeyValue::BackspaceKey: {
            if (value->length()) {
                if (m_currentCaretPosition > 0 &&
                    (int32_t)value->length() > minLength()) {
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
                if (m_currentCaretPosition < value->length() &&
                    (int32_t)value->length() > minLength()) {
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
            if (!ignoreLineBreaks() &&
                m_currentCaretPosition < (size_t)maxLength()) {
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
