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
#include "core/dom/Event.h"
#include "core/dom/HTMLTextAreaElement.h"

namespace StarFish {

const float DEFAULT_ROWS_MARGIN = 0.5;
const uint32_t DEFAULT_ROWS = 2;
const uint32_t DEFAULT_COLS = 20;

static String* textAreaLineBreakNormalizationTransformation(String* rawValue)
{
    // https://html.spec.whatwg.org/multipage/form-elements.html#textarea-line-break-normalisation-transformation
    // TODO
    return rawValue;
}

static String* textAreaWrappingTransformation(String* rawValue)
{
    // https://html.spec.whatwg.org/multipage/form-elements.html#textarea-wrapping-transformation
    // TODO
    return rawValue;
}

HTMLTextAreaElement::HTMLTextAreaElement(Document* document)
    : HTMLTextEditable(document)
{
}

void* HTMLTextAreaElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLTextAreaElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLTextAreaElement)] = { 0 };
        HTMLTextEditable::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLTextAreaElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

Node* HTMLTextAreaElement::clone()
{
    // The cloning steps for textarea elements must propagate the raw value and
    // dirty value flag from the node being cloned to the copy.
    HTMLTextAreaElement* n = HTMLTextEditable::clone()->asHTMLTextAreaElement();
    n->m_dirtyValueFlag = m_dirtyValueFlag;
    n->m_value = m_value;
    return n;
}

void HTMLTextAreaElement::didNodeInserted(Node* parent, Node* newChild)
{
    // The child text content change steps for textarea elements must, if the
    // element's dirty value flag is false, set the element's raw value to its
    // child text content.
    if (parent == this && !m_dirtyValueFlag) {
        Nullable<String*> content = textContent();
        if (content.hasValue()) {
            m_value = content.getValue();
        }
    }
}

void HTMLTextAreaElement::didAttributeChanged(QualifiedName name, String* old,
                                              String* val,
                                              bool attributeCreated,
                                              bool attributeRemoved)
{
    HTMLTextEditable::didAttributeChanged(name, old, val, attributeCreated,
                                          attributeRemoved);
    if (name == starFish()->staticStrings()->m_dir) {
        Event* e = new Event(document(),
                             starFish()->staticStrings()->m_input.localName());
        e->setBubbles(true);
        dispatchEventIdleTimeByUA(e);
        return;
    }
}

void HTMLTextAreaElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLTextEditable::styleForPresentationAttribute(cssValues);
    // Width
    CSSStyleValuePair width;
    width.setKeyKind(CSSStyleValuePair::KeyKind::Width);
    width.setLengthValue(CSSLength(CSSLength::EM, cols()));
    cssValues.push_back(width);
    // Height
    CSSStyleValuePair height;
    height.setKeyKind(CSSStyleValuePair::KeyKind::Height);
    height.setLengthValue(
        CSSLength(CSSLength::EM, rows() + DEFAULT_ROWS_MARGIN));
    cssValues.push_back(height);
}

QualifiedName HTMLTextAreaElement::name()
{
    return starFish()->staticStrings()->m_textareaTagName;
}

bool HTMLTextAreaElement::supportsFocus()
{
    return isMutable();
}

bool HTMLTextAreaElement::isPlaceholderVisible()
{
    if (!placeholder()->equals(String::emptyString) &&
        value()->equals(String::emptyString)) {
        return true;
    }
    return false;
}

void HTMLTextAreaElement::reset()
{
    // The reset algorithm for textarea elements is to set the dirty value flag
    // back to false, and set the raw value of element to its child text
    // content.
    m_dirtyValueFlag = false;
    Nullable<String*> content = textContent();
    if (content.hasValue()) {
        m_value = content.getValue();
    } else {
        m_value = String::emptyString;
    }
}

String* HTMLTextAreaElement::type()
{
    return starFish()->staticStrings()->m_textareaTagName.localName();
}

// For historical reasons, the element's value is normalized
// in three different ways for three different purposes.
//   1. raw value: HTMLFormControl.m_value
//   2. API value: Used in the value IDL attribute -> apiValue()
//   3. Element's value: Used in form submission -> value()

String* HTMLTextAreaElement::value()
{
    // Element's value
    // Used in form submission
    // = Raw value with the textarea wrapping transformation applied
    return textAreaWrappingTransformation(m_value);
}

String* HTMLTextAreaElement::apiValue()
{
    // API Value
    // Used in the value IDL attribute, textLength IDL attribute, and by the
    // maxlength and minlength content attributes
    // = Raw value with the textarea line break normalization transformation
    // applied
    return textAreaLineBreakNormalizationTransformation(m_value);
}

void HTMLTextAreaElement::setApiValue(String* value)
{
    // https://html.spec.whatwg.org/multipage/form-elements.html#dom-textarea-value
    // TODO Reset selection diretion if necessary
    m_value = value;
    m_dirtyValueFlag = true;
    setNeedsFrameTreeBuildWithoutSelf();
}

String* HTMLTextAreaElement::defaultValue()
{
    // https://html.spec.whatwg.org/multipage/form-elements.html#dom-textarea-defaultvalue
    Nullable<String*> content = textContent();
    return content.hasValue() ? content.getValue() : String::emptyString;
}

void HTMLTextAreaElement::setDefaultValue(String* value)
{
    // https://html.spec.whatwg.org/multipage/form-elements.html#dom-textarea-defaultvalue
    setTextContent(value);
}

int HTMLTextAreaElement::textLength()
{
    return apiValue()->length();
}

uint32_t HTMLTextAreaElement::cols()
{
    Nullable<String*> attrValue =
        getAttribute(starFish()->staticStrings()->m_cols);
    if (attrValue.hasValue()) {
        int32_t parsed = String::parseInt(attrValue.getValue());
        if (parsed >= 0) {
            return parsed;
        }
    }
    return DEFAULT_COLS;
}

void HTMLTextAreaElement::setCols(uint32_t value)
{
    setAttribute(starFish()->staticStrings()->m_cols, String::fromInt(value));
    setNeedsFrameTreeBuildWithoutSelf();
}

uint32_t HTMLTextAreaElement::rows()
{
    Nullable<String*> attrValue =
        getAttribute(starFish()->staticStrings()->m_rows);
    if (attrValue.hasValue()) {
        int32_t parsed = String::parseInt(attrValue.getValue());
        if (parsed >= 0) {
            return parsed;
        }
    }
    return DEFAULT_ROWS;
}

void HTMLTextAreaElement::setRows(uint32_t value)
{
    setAttribute(starFish()->staticStrings()->m_rows, String::fromInt(value));
    setNeedsFrameTreeBuildWithoutSelf();
}
}
