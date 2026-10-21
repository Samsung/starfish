/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include <limits>

#include "core/dom/Event.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Text.h"
#include "core/dom/KeyboardEvent.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/Touch.h"
#include "core/dom/TouchEvent.h"
#include "core/dom/TouchList.h"
#include "core/dom/InputEvent.h"
#include "core/dom/CompositionEvent.h"
#include "core/dom/HTMLFormElement.h"
#include "core/dom/NodeList.h"
#include "core/dom/NodeListImpl.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/layout/FrameInputBox.h"
#include "core/layout/FrameRangeBox.h"
#ifdef STARFISH_ENABLE_A11Y_ATSPI
#include "core/page/A11yAtspiTreeSource.h"
#endif

namespace Starfish {

// TODO: We should discuss the maxlength limitation
// because the spec doesn't describe the actual number.
// 524288 is Chromium's
static const int INITIAL_MAXLENGTH = 524288;

// The range state is the only stepped state implemented, so its bookkeeping
// details are the only ones the helpers below need.
// https://html.spec.whatwg.org/multipage/input.html#range-state-(type=range)
static const double DEFAULT_MINIMUM_FOR_RANGE_TYPE = 0;
static const double DEFAULT_MAXIMUM_FOR_RANGE_TYPE = 100;
static const double DEFAULT_STEP_FOR_RANGE_TYPE = 1;

// https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#valid-floating-point-number
// String::validDouble() only screens the character set, so it accepts "",
// "1.2.3", "e" and "+1"; deciding whether a value "is not a valid
// floating-point number" the way the value sanitization steps mean it needs
// the actual grammar.
static bool parseFloatingPointNumber(String* input, double* out)
{
    size_t length = input->length();
    size_t i = 0;

    if (i < length && input->charAt(i) == '-') {
        i++;
    }

    size_t integerDigits = 0;
    while (i < length && String::isASCIIDigit(input->charAt(i))) {
        i++;
        integerDigits++;
    }

    size_t fractionDigits = 0;
    if (i < length && input->charAt(i) == '.') {
        i++;
        while (i < length && String::isASCIIDigit(input->charAt(i))) {
            i++;
            fractionDigits++;
        }
        // A fraction part must have at least one digit ("1." is not valid),
        // while an absent integer part is fine (".5" is valid).
        if (!fractionDigits) {
            return false;
        }
    }
    if (!integerDigits && !fractionDigits) {
        return false;
    }

    if (i < length && (input->charAt(i) == 'e' || input->charAt(i) == 'E')) {
        i++;
        if (i < length &&
            (input->charAt(i) == '+' || input->charAt(i) == '-')) {
            i++;
        }
        size_t exponentDigits = 0;
        while (i < length && String::isASCIIDigit(input->charAt(i))) {
            i++;
            exponentDigits++;
        }
        if (!exponentDigits) {
            return false;
        }
    }

    // Trailing content makes the whole string invalid; the value sanitization
    // steps ask for a *valid* floating-point number, not a parsable prefix.
    if (i != length) {
        return false;
    }

    double value = String::parseDouble(input);
    if (!std::isfinite(value)) {
        return false;
    }

    *out = value;
    return true;
}

// Dividing by the step almost never lands exactly on an integer even when the
// value does sit on the step (0.6 / 0.1 is 5.999999999999999 in double), so a
// multiple within a rounding error of an integer counts as on-step.
static bool isIntegralMultipleOfStep(double multiple)
{
    double rounded = std::round(multiple);
    return std::abs(multiple - rounded) <=
           std::abs(multiple) * std::numeric_limits<double>::epsilon() * 8;
}

// https://html.spec.whatwg.org/multipage/common-microsyntaxes.html#best-representation-of-the-number-as-a-floating-point-number
// String::fromDouble() prints with "%g", which caps at 6 significant digits
// and turns 1234567 into "1.23457e+06"; the best representation is the
// shortest form that round-trips. Magnitudes outside what "%g" prints plainly
// (below 1e-4, at or above 1e+15) still come out in exponent form with a
// zero-padded exponent, which JS number serialization would not do.
static String* serializeFloatingPointNumber(double value)
{
    char buf[64];
    for (int precision = 15; precision <= 17; precision++) {
        snprintf(buf, sizeof(buf), "%.*g", precision, value);
        if (strtod(buf, nullptr) == value) {
            break;
        }
    }
    return String::fromUTF8(buf, strnlen(buf, sizeof(buf)));
}

HTMLInputElement::HTMLInputElement(Document* document,
                                   const QualifiedName& qname)
    : HTMLTextEditable(document, qname)
    , m_dirtiness(false)
    , m_checkness(false)
    , m_dirtyCheckness(false)
    , m_previousCheckness(false)
    , m_previousCheckedRadioButton(nullptr)
    , m_rangeValueAtDragStart(0)
{
}

void* HTMLInputElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLInputElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLInputElement)] = { 0 };
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLInputElement, m_currentEditingText));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLInputElement,
                                        m_previousCheckedRadioButton));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLInputElement, m_selectedFiles));
        HTMLTextEditable::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLInputElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

bool HTMLInputElement::shouldCreateFrameText()
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
    } else if (typeString->equals("number")) {
        return true;
    }

    return false;
}

String* HTMLInputElement::type()
{
    String* typeAttr = getAttributeOrEmpty(starfish()->staticStrings()->m_type);
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

    return starfish()->staticStrings()->m_text.localName();
}

String* HTMLInputElement::defaultValue()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_value);
}

void HTMLInputElement::setDefaultValue(String* defaultValue)
{
    setAttribute(starfish()->staticStrings()->m_value, defaultValue);
}

// IDL attribute
String* HTMLInputElement::value()
{
    if (!m_dirtiness) {
        if (type()->equals("range")) {
            // The value content attribute has not been through the range
            // state's sanitization when it is absent, or when it was set
            // before `type` became range, so run it here.
            return sanitizedRangeValue(defaultValue());
        }

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

    sanitizeValue();

    if (!oldValue->equals(val) || m_shouldDrawCaret) {
        if (type()->equals("range")) {
            // A range control has no child frames; only where its thumb lands
            // changes, so the frame tree can stay as it is.
            setNeedsPainting();
        } else {
            setNeedsFrameTreeBuildWithoutSelf();
        }
    }
}

void HTMLInputElement::sanitizeValue()
{
    if (type()->equals("date") || type()->equals("month") ||
        type()->equals("week") || type()->equals("time") ||
        type()->equals("datetime-local")) {
        // TODO
    } else if (type()->equals("range")) {
        m_value = sanitizedRangeValue(m_value);
    } else if (type()->equals("number")) {
        // TODO
        for (size_t i = 0; i < m_value->length(); i++) {
            size_t pos = i;
            char32_t ch = m_value->charAt(pos);
            if (!isdigit(ch)) {
                m_value = String::emptyString;
                break;
            }
        }

        if (m_value->length()) {
            size_t pos = m_value->length() - 1;
            char32_t ch = m_value->charAt(pos);
            if (!isdigit(ch)) {
                m_value = m_value->substring(0, pos);
            }
        }
    }
}

// https://html.spec.whatwg.org/multipage/input.html#range-state-(type=range):concept-input-value-default-range
double HTMLInputElement::defaultValueForRangeType()
{
    double min = minimum();
    double max = maximum();
    // "the minimum plus half the difference between the minimum and the
    // maximum", or the minimum when the maximum is less than the minimum.
    return max < min ? min : min + ((max - min) / 2);
}

// Rounds to the nearest value that sits on the allowed value step, and then
// keeps the result inside [min, max] as the range state demands.
// The arithmetic is done in double, so a value exactly halfway between two
// candidates of a decimal step (0.15 with step 0.1) can round to the smaller
// one where an arbitrary-precision implementation picks the larger.
double HTMLInputElement::alignToAllowedValueStep(double val, double stepVal,
                                                 double min, double max)
{
    double base = stepBase();
    double multiple = (val - base) / stepVal;
    if (isIntegralMultipleOfStep(multiple)) {
        // Already on the step. Keep the value as it came in rather than
        // recomputing base + nearest * step, which would reintroduce the
        // division's rounding error into the serialized value.
        return val;
    }

    double lower = base + std::floor(multiple) * stepVal;
    double upper = base + std::ceil(multiple) * stepVal;

    // Of two equally close candidates the spec asks for the larger one.
    double aligned = (val - lower) < (upper - val) ? lower : upper;
    if (aligned > max) {
        aligned = lower;
    }
    if (aligned < min) {
        aligned = upper;
    }
    return aligned;
}

// https://html.spec.whatwg.org/multipage/input.html#range-state-(type=range):value-sanitization-algorithm
// The range state's value sanitization together with the underflow, overflow
// and step mismatch corrections the state layers on top of it: parse, fall
// back to the default value, clamp, then align to the step.
String* HTMLInputElement::sanitizedRangeValue(String* input)
{
    double val;
    if (!parseFloatingPointNumber(input, &val)) {
        val = defaultValueForRangeType();
    }

    double min = minimum();
    double max = std::max(min, maximum());
    val = std::min(std::max(val, min), max);

    double stepVal;
    if (allowedValueStep(&stepVal)) {
        val = alignToAllowedValueStep(val, stepVal, min, max);
    }

    return serializeFloatingPointNumber(val);
}

String* HTMLInputElement::checkboxTickSymbol()
{
    return String::createUTF32String(U'\u2714'); // tick
}

String* HTMLInputElement::placeholder()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_placeholder);
}

void HTMLInputElement::setPlaceholder(String* value)
{
    setAttribute(starfish()->staticStrings()->m_placeholder, value);
}

void HTMLInputElement::toggleChecked()
{
    if (checked()) {
        setChecked(false);
    } else {
        setChecked(true);
    }
}

bool HTMLInputElement::defaultChecked()
{
    Optional<String*> val =
        getAttribute(starfish()->staticStrings()->m_checked);

    return val.hasValue();
}

void HTMLInputElement::setDefaultChecked(bool checked)
{
    if (checked) {
        setAttribute(starfish()->staticStrings()->m_checked,
                     String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_checked);
    }
}

// IDL attribute
bool HTMLInputElement::checked()
{
    if (!m_dirtyCheckness) {
        return defaultChecked();
    }
    return m_checkness;
}

// https://html.spec.whatwg.org/multipage/input.html#radio-button-state-(type=radio)
void HTMLInputElement::setChecked(bool checked)
{
    m_checkness = checked;
    m_dirtyCheckness = true;

    if (type()->equals("radio") && m_checkness) {
        resetRadioButtons();
    }
    setNeedsFrameTreeBuildWithoutSelf();
#ifdef STARFISH_ENABLE_A11Y_ATSPI
    // Checkedness is internal state (not an attribute); notify so the
    // AT-SPI bridge can emit state-change::checked.
    A11yAtspiTreeSource::notifyPageChanged(document());
#endif
}

void HTMLInputElement::resetRadioButtons()
{
    GCVector<HTMLInputElement*>* list = radioButtonGroup();
    if (!list) {
        return;
    }

    for (HTMLInputElement* input : *list) {
        if (input != this) {
            input->m_checkness = false;
        }
    }
}

// https://html.spec.whatwg.org/multipage/input.html#radio-button-group
GCVector<HTMLInputElement*>* HTMLInputElement::radioButtonGroup()
{
    String* name = getAttributeOrEmpty(starfish()->staticStrings()->m_name);
    if (name->equals(String::emptyString)) {
        return nullptr;
    }

    GCVector<HTMLInputElement*>* radioButtonGroup =
        new GCVector<HTMLInputElement*>();

    HTMLFormElement* form = formOwner(); // It is ok when form is null

    NodeList* nodeList = document()->getElementsByName(name);
    NodeListImpl& listImpl = nodeList->getNodeListImpl();
    GCVector<Node*> list;
    listImpl.getherDescendant(&list, listImpl.root());

    for (Node* n : list) {
        if (n->isHTMLInputElement()) {
            HTMLInputElement* input = n->asHTMLInputElement();
            if (input->type()->equals("radio") &&
                (form == input->formOwner()) &&
                (document() == input->document())) {
                radioButtonGroup->push_back(input);
            }
        }
    }

    return radioButtonGroup;
}

uint32_t HTMLInputElement::size()
{
    String* size = getAttributeOrEmpty(starfish()->staticStrings()->m_size);
    if (size->length()) {
        // if the value is in the range 1 to 2147483647 inclusive,
        // the resulting value must be returned.
        int32_t value = String::parseInt64(size);
        return value > 0 ? value : DEFAULT_SIZE;
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
        throw new DOMException(executionContext(),
                               DOMException::Code::INDEX_SIZE_ERR, msg);
    } else if (size < 0) {
        setAttribute(starfish()->staticStrings()->m_size,
                     String::fromInt(DEFAULT_SIZE));
    } else {
        setAttribute(starfish()->staticStrings()->m_size, sizeStr);
    }
}

bool HTMLInputElement::hasActivationBehavior()
{
    if (type()->equals("checkbox") || type()->equals("radio") ||
        type()->equals("file") || type()->equals("submit") ||
        type()->equals("image") || type()->equals("reset") ||
        type()->equals("button")) {
        return true;
    }

    return false;
}

HTMLInputElement* HTMLInputElement::getCurrentCheckedRadioButton()
{
    GCVector<HTMLInputElement*>* list = radioButtonGroup();
    if (list) {
        for (HTMLInputElement* input : *list) {
            if (input->checked()) {
                return input;
            }
        }
    }
    return nullptr;
}

bool HTMLInputElement::isInSameRadioButtonGroup(HTMLInputElement* other)
{
    STARFISH_ASSERT(other->type()->equals("radio"));
    GCVector<HTMLInputElement*>* list = radioButtonGroup();
    if (list) {
        for (HTMLInputElement* input : *list) {
            if (input == other) {
                return true;
            }
        }
    }
    return false;
}

// https://html.spec.whatwg.org/multipage/input.html#the-input-element:activation-behaviour
void HTMLInputElement::activationBehavior()
{
    // TODO: check "apply" and mutability
    if (type()->equals("hidden")) {
        return;
    }

    if (type()->equals("checkbox") || type()->equals("radio")) {
        // input is composed as well as bubbling; change only bubbles.
        fireEvent(starfish()->staticStrings()->m_input, true, false, true);
        fireEvent(starfish()->staticStrings()->m_change, true, false);
    } else if (type()->equals("file")) {
    } else if (type()->equals("submit")) {
    } else if (type()->equals("image")) {
    } else if (type()->equals("reset")) {
    } else if (type()->equals("button")) {
    }
    setNeedsFrameTreeBuildWithoutSelf();
}

void HTMLInputElement::reset()
{
    m_dirtyValueFlag = m_dirtyCheckness = false;

    m_value = defaultValue();
    m_checkness = defaultChecked();
    sanitizeValue();
}

void HTMLInputElement::legacyPreActivationBehavior()
{
    if (type()->equals("checkbox")) {
        m_previousCheckness = m_checkness;
        m_checkness = !m_checkness;
        m_dirtyCheckness = true;
    } else if (type()->equals("radio") && !checked()) {
        m_previousCheckedRadioButton = getCurrentCheckedRadioButton();
        if (m_previousCheckedRadioButton) {
            m_previousCheckedRadioButton->m_checkness = false;
        }
        m_checkness = true;
        m_dirtyCheckness = true;
    }
}

void HTMLInputElement::legacyCanceledActivationBehavior()
{
    if (type()->equals("checkbox")) {
        m_checkness = m_previousCheckness;
    } else if (type()->equals("radio")) {
        if (m_previousCheckedRadioButton &&
            isInSameRadioButtonGroup(m_previousCheckedRadioButton)) {
            m_previousCheckedRadioButton->m_checkness = true;
        }
        m_checkness = false;
    }
    setNeedsFrameTreeBuildWithoutSelf();
}

bool HTMLInputElement::isListedElement()
{
    auto typeString = type();
    if (!typeString->equals("image")) {
        return true;
    }
    return false;
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
    } else if (typeString->equals("number")) {
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

void HTMLInputElement::didAttributeChanged(QualifiedName name,
                                           Optional<String*> old, String* val,
                                           bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLFormControl::didAttributeChanged(name, old, val, attributeCreated,
                                         attributeRemoved);

    if (name == starfish()->staticStrings()->m_type) {
        setNeedsFrameTreeBuild();
    } else if (name == starfish()->staticStrings()->m_min ||
               name == starfish()->staticStrings()->m_max ||
               name == starfish()->staticStrings()->m_step) {
        // The range state keeps its value inside [min, max] and on the step,
        // so a change to any of the three re-runs sanitization even for a
        // value the author already set.
        if (type()->equals("range")) {
            if (m_dirtiness) {
                sanitizeValue();
            }
            setNeedsFrameTreeBuild();
        }
    } else if (name == starfish()->staticStrings()->m_value) {
        // https://html.spec.whatwg.org/multipage/input.html#attr-input-value
        if (!m_dirtiness) {
            if (attributeRemoved) {
                m_value = String::emptyString;
            } else { // created or updated
                m_value = val;
            }
            sanitizeValue();
        }

        setNeedsFrameTreeBuild();
    } else if (name == starfish()->staticStrings()->m_checked) {
        if (!m_dirtyCheckness) {
            if (attributeCreated) {
                m_checkness = true;
            } else if (attributeRemoved) {
                m_checkness = false;
            }
        }
    } else if (name == starfish()->staticStrings()->m_name) {
        if (type()->equals("radio") && checked()) {
            resetRadioButtons();
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
    if (HTMLTextEditable::handleDefaultEvent(event)) {
        return true;
    }

    // The range handler sees disabled controls too, to let go of a drag.
    if (type()->equals("range") && handleRangeEvent(event)) {
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
            } else if (shouldUsePlaceholder()) {
                return true;
            }
        } else if (event->type()->equals("mousedown") ||
                   event->type()->equals("touchstart")) {
            if (isEditableType()) {
                return true;
            }
        }
    }
    return false;
}

// https://html.spec.whatwg.org/multipage/input.html#range-state-(type=range)
// The spec leaves how the user moves a range control to the user agent; this
// follows what other engines do: arrow keys step, Page keys jump a tenth of
// the range, Home/End go to the ends, and a pointer drags the thumb.
bool HTMLInputElement::handleRangeEvent(Event* event)
{
    // Only the user moves the control; a script dispatching the same events
    // gets no default action, as with other engines.
    if (!event->isTrusted()) {
        return false;
    }

    if (isDisabled()) {
        // Disabled mid-drag: let go where the thumb is, committing what the
        // drag changed so far.
        if (draggingRange()) {
            endRangeDrag();
        }
        return false;
    }

    if (event->isKeyboardEvent()) {
        if (!event->type()->equals("keydown") ||
            document()->browsingContext()->focusedNode() != this) {
            return false;
        }
        return handleRangeKeydown(event->asKeyboardEvent());
    }

    // A drag holds the pointer capture, so the moves and the release keep
    // coming here once the pointer leaves the thin box of the control, and
    // the browsing context lets go of it on release even when that happens
    // somewhere this control no longer hears about.
    BrowsingContext* browsingContext = document()->browsingContext();
    if (event->isMouseEvent()) {
        MouseEvent* mouseEvent = event->asMouseEvent();
        if (mouseEvent->button() != 0) {
            return false;
        }
        if (event->type()->equals("mousedown")) {
            m_rangeValueAtDragStart = valueAsNumber();
            browsingContext->setPointerCaptureTarget(this);
            moveRangeThumbTo(mouseEvent->pageX(), mouseEvent->pageY());
            return true;
        }
        if (!draggingRange()) {
            return false;
        }
        if (event->type()->equals("mousemove")) {
            moveRangeThumbTo(mouseEvent->pageX(), mouseEvent->pageY());
            return true;
        }
        if (event->type()->equals("mouseup")) {
            moveRangeThumbTo(mouseEvent->pageX(), mouseEvent->pageY());
            endRangeDrag();
            return true;
        }
        return false;
    }

    if (event->isTouchEvent()) {
        TouchEvent* touchEvent = event->asTouchEvent();
        if (event->type()->equals("touchstart")) {
            if (touchEvent->touches()->length() == 0) {
                return false;
            }
            Touch* touch = touchEvent->touches()->item(0);
            m_rangeValueAtDragStart = valueAsNumber();
            browsingContext->setPointerCaptureTarget(this);
            moveRangeThumbTo(touch->pageX(), touch->pageY());
            return true;
        }
        if (!draggingRange()) {
            return false;
        }
        if (event->type()->equals("touchmove")) {
            if (touchEvent->touches()->length()) {
                Touch* touch = touchEvent->touches()->item(0);
                moveRangeThumbTo(touch->pageX(), touch->pageY());
            }
            return true;
        }
        if (event->type()->equals("touchend") ||
            event->type()->equals("touchcancel")) {
            endRangeDrag();
            return true;
        }
    }
    return false;
}

bool HTMLInputElement::draggingRange()
{
    return document()->browsingContext()->pointerCaptureTarget() == this;
}

void HTMLInputElement::endRangeDrag()
{
    document()->browsingContext()->releasePointerCaptureTarget(this);
    commitRangeValue(m_rangeValueAtDragStart);
}

bool HTMLInputElement::handleRangeKeydown(KeyboardEvent* event)
{
    double min = minimum();
    double max = maximum();
    if (max < min) {
        return false;
    }

    // A control with step="any" has no step to move by; a hundredth of the
    // range keeps the keys useful, as in other engines.
    double step;
    if (!allowedValueStep(&step)) {
        step = (max - min) / 100;
    }
    double pageStep = std::max((max - min) / 10, step);
    bool rtl = frame() && frame()->style()->direction() ==
                              DirectionValue::RtlDirectionValue;

    double current = valueAsNumber();
    double next;
    switch (event->keyValue()) {
    case LWE::KeyValue::ArrowUpKey:
        next = current + step;
        break;
    case LWE::KeyValue::ArrowDownKey:
        next = current - step;
        break;
    // Left and right follow the direction the track is drawn in.
    case LWE::KeyValue::ArrowRightKey:
        next = rtl ? current - step : current + step;
        break;
    case LWE::KeyValue::ArrowLeftKey:
        next = rtl ? current + step : current - step;
        break;
    case LWE::KeyValue::PageUpKey:
        next = current + pageStep;
        break;
    case LWE::KeyValue::PageDownKey:
        next = current - pageStep;
        break;
    case LWE::KeyValue::HomeKey:
        next = min;
        break;
    case LWE::KeyValue::EndKey:
        next = max;
        break;
    default:
        return false;
    }

    setRangeValueByUser(std::min(std::max(next, min), max));
    commitRangeValue(current);
    return true;
}

void HTMLInputElement::moveRangeThumbTo(double pageX, double pageY)
{
    if (!frame() || !frame()->isFrameRangeBox()) {
        return;
    }
    Optional<double> fraction =
        static_cast<FrameRangeBox*>(frame())->fractionAtPagePoint(pageX, pageY);
    if (!fraction) {
        return;
    }
    double min = minimum();
    double max = maximum();
    if (max < min) {
        return;
    }
    setRangeValueByUser(min + (max - min) * fraction.value());
}

// https://html.spec.whatwg.org/multipage/input.html#common-input-element-events
// input fires whenever the user changes the value, change once the change is
// committed: on a key press right away, on a pointer drag when it ends.
void HTMLInputElement::setRangeValueByUser(double value)
{
    String* oldValue = this->value();
    setValue(serializeFloatingPointNumber(value));
    if (!this->value()->equals(oldValue)) {
        fireEvent(starfish()->staticStrings()->m_input, true, false, true);
    }
}

void HTMLInputElement::commitRangeValue(double valueBefore)
{
    if (valueAsNumber() != valueBefore) {
        fireEvent(starfish()->staticStrings()->m_change, true, false);
    }
}

bool HTMLInputElement::isPlaceholderVisible()
{
    return shouldUsePlaceholder();
}

bool HTMLInputElement::supportsFocus()
{
    return !type()->equals("hidden");
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

bool HTMLInputElement::ignoreLineBreaks()
{
    String* typeString = type();
    if (typeString->equals("text")) {
        return true;
    } else if (typeString->equals("search")) {
        return true;
    } else if (typeString->equals("tel")) {
        return true;
    } else if (typeString->equals("password")) {
        return true;
    } else if (typeString->equals("submit")) {
        return true;
    } else if (typeString->equals("button")) {
        return true;
    } else if (typeString->equals("number")) {
        return true;
    } else if (typeString->equals("checkbox")) {
        return true;
    }
    return false;
}

void HTMLInputElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues, MatchedStyleRules<>& matchedRules,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues, matchedRules,
                                               cssCustomValues);

    if (isSizableType()) {
        auto siz = size();
        CSSStyleValuePair pair;
        pair.setKeyKind(CSSStyleValuePair::KeyKind::Width);
        pair.setValueKind(CSSStyleValuePair::ValueKind::Length);
        pair.setLengthValue(CSSLength(CSSLength::EM, siz));
        cssValues.push_back(pair);
    }
}

String* HTMLInputElement::max()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_max);
}

void HTMLInputElement::setMax(String* max)
{
    setAttribute(starfish()->staticStrings()->m_max, max);
}

String* HTMLInputElement::min()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_min);
}

void HTMLInputElement::setMin(String* min)
{
    setAttribute(starfish()->staticStrings()->m_min, min);
}

double HTMLInputElement::minimum()
{
    double val;
    if (parseFloatingPointNumber(min(), &val)) {
        return val;
    }
    return DEFAULT_MINIMUM_FOR_RANGE_TYPE;
}

double HTMLInputElement::maximum()
{
    double val;
    if (parseFloatingPointNumber(max(), &val)) {
        return val;
    }
    return DEFAULT_MAXIMUM_FOR_RANGE_TYPE;
}

String* HTMLInputElement::step()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_step);
}

void HTMLInputElement::setStep(String* step)
{
    setAttribute(starfish()->staticStrings()->m_step, step);
}

// https://html.spec.whatwg.org/multipage/input.html#concept-input-step
bool HTMLInputElement::allowedValueStep(double* ret)
{
    String* stepVal = step();
    if (stepVal->toASCIILower()->equals("any")) {
        // no allowed value step
        return false;
    }

    // An absent, unparsable or non-positive step attribute falls back to the
    // default step, which the range state's step scale factor of 1 leaves as
    // is.
    double val;
    if (!parseFloatingPointNumber(stepVal, &val) || val <= 0) {
        val = DEFAULT_STEP_FOR_RANGE_TYPE;
    }

    *ret = val;
    return true;
}

double HTMLInputElement::stepBase()
{
    double val;
    if (parseFloatingPointNumber(min(), &val)) {
        return val;
    }

    if (parseFloatingPointNumber(defaultValue(), &val)) {
        return val;
    }

    // https://html.spec.whatwg.org/multipage/input.html#week-state-(type=week):concept-input-step-default-base
    if (type()->equals("week")) {
        return -259200000;
    }

    return 0;
}

double HTMLInputElement::rangeValueFraction()
{
    double min = minimum();
    double max = maximum();
    if (max <= min) {
        return 0;
    }

    double val;
    if (!parseFloatingPointNumber(value(), &val)) {
        return 0;
    }
    return (std::min(std::max(val, min), max) - min) / (max - min);
}

// https://html.spec.whatwg.org/multipage/input.html#dom-input-valueasnumber
double HTMLInputElement::valueAsNumber()
{
    // Range is the only state whose value sanitization is implemented, and for
    // a state that does not define a value-as-number the attribute is defined
    // to return NaN.
    if (!type()->equals("range")) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double val;
    if (!parseFloatingPointNumber(value(), &val)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return val;
}

void HTMLInputElement::setValueAsNumber(double val)
{
    if (!type()->equals("range")) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    // The spec asks for a TypeError on an infinite value, which needs a throw
    // path the bindings do not offer yet; an unrepresentable number takes the
    // NaN route instead of leaking "inf" into the value.
    if (!std::isfinite(val)) {
        setValue(String::emptyString);
        return;
    }

    setValue(serializeFloatingPointNumber(val));
}

// https://html.spec.whatwg.org/multipage/input.html#dom-input-stepup
void HTMLInputElement::stepUp(int32_t n)
{
    applyStep(n);
}

void HTMLInputElement::stepDown(int32_t n)
{
    applyStep(-(int64_t)n);
}

void HTMLInputElement::applyStep(int64_t n)
{
    // Only the range state has an implemented step; every other state is in
    // the spec's "the method is not supported" case.
    if (!type()->equals("range")) {
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    double stepVal;
    if (!allowedValueStep(&stepVal)) {
        // step="any"
        throw new DOMException(executionContext(),
                               DOMException::INVALID_STATE_ERR,
                               "InvalidStateError");
    }

    double min = minimum();
    double max = maximum();
    if (min > max) {
        return;
    }

    double val;
    if (!parseFloatingPointNumber(value(), &val)) {
        return;
    }

    val += stepVal * n;

    // A value that started off-step is rounded in the direction of the step
    // rather than to the nearest multiple.
    double base = stepBase();
    double multiple = (val - base) / stepVal;
    if (!isIntegralMultipleOfStep(multiple)) {
        val = base +
              (n > 0 ? std::floor(multiple) : std::ceil(multiple)) * stepVal;
    }

    if (val < min) {
        val = base + std::ceil((min - base) / stepVal) * stepVal;
    } else if (val > max) {
        val = base + std::floor((max - base) / stepVal) * stepVal;
    }

    setValue(serializeFloatingPointNumber(val));
}
} // namespace Starfish
