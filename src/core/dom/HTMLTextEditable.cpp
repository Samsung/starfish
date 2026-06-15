/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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
#include "Starfish.h"

#include "core/dom/HTMLTextEditable.h"
#include "core/dom/CompositionEvent.h"
#include "core/dom/Document.h"
#include "core/dom/KeyboardEvent.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/modules/message_loop/Timer.h"
#include "core/modules/renderer/Renderer.h"

namespace Starfish {

// TODO HTMLInputElement will inherit this class

// TODO: We should discuss the maxlength limitation
// because the spec doesn't describe the actual number.
// 524288 is Chromium's
static const int INITIAL_MAXLENGTH = 524288;

HTMLTextEditable::HTMLTextEditable(Document* document,
                                   const QualifiedName& qname)
    : HTMLFormControl(document, qname)
    , m_dirtyValueFlag(false)
    , m_shouldDrawCaret(false)
    , m_caretBlinkingIntervalId(TimerInvalidID)
    , m_currentCaretPosition(0)
    , m_currentEditingText(String::emptyString)
    , m_maxlength(INITIAL_MAXLENGTH)
    , m_editStatus(EditStatus::None)
    , m_havePreedit(false)
    , m_preeditEndPos(0)
    , m_preeditStartPos(0)
{
}

bool HTMLTextEditable::readonly() const
{
    return getAttribute(starfish()->staticStrings()->m_readonly).hasValue();
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
                [](void* data) {
                    HTMLTextEditable* e = (HTMLTextEditable*)data;
                    e->m_shouldDrawCaret = !e->m_shouldDrawCaret;
                    e->setNeedsPainting();
                },
                500, this);
            webView()->renderer()->showSoftwareKeyboardIfPossible();
        } else if (oldGotFocus && !newGotFocus) {
            webView()->renderer()->hideSoftwareKeyboardIfPossible();
            m_shouldDrawCaret = false;
            m_currentEditingText = String::emptyString;
            window()->clearInterval(m_caretBlinkingIntervalId);
        } else if (oldGotFocus && newGotFocus) {
            webView()->renderer()->showSoftwareKeyboardIfPossible();
        }
    }
}

LayoutUnit HTMLTextEditable::caretThickness() const
{
    return LayoutUnit(CARET_THICKNESS);
}

String* HTMLTextEditable::placeholder()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_placeholder);
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

    String* value = this->value();
    m_currentCaretPosition = std::min(m_currentCaretPosition, value->length());

    String* oldValue = value;
    if (shouldUsePlaceholder()) {
        value = String::emptyString;
    }
    if (event->isKeyboardEvent() && event->type()->equals("keydown")) {
        bool isUseful = false;
        switch (event->asKeyboardEvent()->keyValue()) {
        case LWE::KeyValue::ArrowLeftKey: {
            if (m_currentCaretPosition > 0) {
                m_currentCaretPosition--;
            }
            m_shouldDrawCaret = true;
            isUseful = true;
            resetCurrentContext();
        } break;
        case LWE::KeyValue::ArrowRightKey: {
            if (m_currentCaretPosition < value->length()) {
                m_currentCaretPosition++;
            }
            m_shouldDrawCaret = true;
            isUseful = true;
            resetCurrentContext();
        } break;
        case LWE::KeyValue::BackspaceKey: {
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
            resetCurrentContext();
        } break;
        case LWE::KeyValue::DeleteKey: {
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
            resetCurrentContext();
        } break;
        case LWE::KeyValue::EnterKey: {
            if (!ignoreLineBreaks() &&
                m_currentCaretPosition < (size_t)maxLength()) {
                value = value->concat('\n');
                m_currentCaretPosition++;
                m_shouldDrawCaret = true;
                isUseful = true;
            }
            resetCurrentContext();
            webView()->renderer()->hideSoftwareKeyboardIfPossible();
        } break;
        default: {
            if (String::isASCIIPrintableKey(
                    event->asKeyboardEvent()->keyValue()) &&
                m_currentCaretPosition < (size_t)maxLength()) {
                char key = (char)event->asKeyboardEvent()->keyValue();

                StringBuilder sb;
                sb.appendSubString(value, 0, m_currentCaretPosition);
                sb.appendString(String::createASCIIString(key));
                sb.appendSubString(value, m_currentCaretPosition,
                                   value->length());
                value = sb.finalize();

                m_currentCaretPosition++;
                m_shouldDrawCaret = true;
                isUseful = true;
            }
        } break;
        }

        if (isUseful) {
            if (!value->equals(oldValue) || m_shouldDrawCaret) {
                setValue(value);
            }
            return true;
        }
    } else if (event->isCompositionEvent()) {
        STARFISH_LOG_INFO("Composition Event [%s]",
                          event->type()->toUTF8NonGCString().data());
        if (m_currentCaretPosition < (size_t)maxLength()) {
            String* data = event->asCompositionEvent()->data();

            if (event->type()->equals("compositionstart")) {
            } else if (event->type()->equals("compositionupdate")) {
                STARFISH_LOG_INFO("CompositionUpdate data[%s]",
                                  data->toUTF8NonGCString().data());
                setEditStatus(EditStatus::PreeditStart);
                if (data->isEmpty()) {
                    setEditStatus(EditStatus::PreeditEnd);
                }

                if (m_editStatus == EditStatus::PreeditStart ||
                    m_editStatus == EditStatus::PreeditEnd) {
                    consumeLastPreedit();
                }

                m_havePreedit = false;
                if (m_editStatus == EditStatus::PreeditStart) {
                    m_preeditStartPos = m_currentCaretPosition;

                    m_currentEditingText = data;
                    String* v = this->value()->insert(m_currentEditingText,
                                                      m_preeditStartPos);
                    setValue(v);
                    m_currentCaretPosition += m_currentEditingText->length();
                    m_preeditEndPos = m_currentCaretPosition;
                    m_havePreedit = true;
                }
                m_shouldDrawCaret = true;
            } else if (event->type()->equals("compositionend")) {
                setEditStatus(EditStatus::Commit);

                consumeLastPreedit();

                m_currentEditingText = data;
                String* v = this->value()->insert(m_currentEditingText,
                                                  m_currentCaretPosition);
                setValue(v);
                m_currentCaretPosition += m_currentEditingText->length();

                setEditStatus(EditStatus::None);
            }
        }
        if (!value->equals(oldValue)) {
            setValue(value);
        }
        STARFISH_LOG_INFO("Result [text:%s][cursor:%d]",
                          this->value()->toUTF8NonGCString().data(),
                          (int)m_currentCaretPosition);

        return true;
    }
    return false;
}

void HTMLTextEditable::setEditStatus(EditStatus status)
{
    STARFISH_LOG_INFO("Set edit status[%d]", status);
    m_editStatus = status;
}

void HTMLTextEditable::consumeLastPreedit()
{
    if (m_havePreedit) {
        int count = m_preeditEndPos - m_preeditStartPos;
        int start = m_currentCaretPosition - count;

        // The preedit range can be stale if the value was mutated out from
        // under the composition (e.g. script setting input.value
        // mid-composition). Only remove when the recorded range is still within
        // the value.
        if (count > 0 && start >= 0 &&
            (size_t)(start + count) <= value()->length()) {
            String* v = value()->remove(start, count);
            setValue(v);

            m_currentCaretPosition -= count;
        }
    }
    m_havePreedit = false;
    m_preeditEndPos = 0;
    m_preeditStartPos = 0;
}

void HTMLTextEditable::resetCurrentContext()
{
    setEditStatus(EditStatus::None);
    m_preeditStartPos = 0;
    m_preeditEndPos = 0;
    m_havePreedit = false;
}
} // namespace Starfish
