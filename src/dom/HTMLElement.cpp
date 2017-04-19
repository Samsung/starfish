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

#include "StarFishConfig.h"
#include "Document.h"
#include "Text.h"
#include "HTMLElement.h"

#include "layout/FrameBox.h"

namespace StarFish {

void HTMLElement::didAttributeChanged(QualifiedName name, String* old,
                                      String* value, bool attributeCreated,
                                      bool attributeRemoved)
{
    Element::didAttributeChanged(name, old, value, attributeCreated,
                                 attributeRemoved);
    StaticStrings* ss = document()->window()->starFish()->staticStrings();
    if (name == ss->m_onclick) {
        setAttributeEventListener(ss->m_click, value, this);
    } else if (name == ss->m_onmouseover) {
        setAttributeEventListener(ss->m_mouseover, value, this);
    } else if (name == ss->m_onmouseup) {
        setAttributeEventListener(ss->m_mouseup, value, this);
    } else if (name == ss->m_onmousedown) {
        setAttributeEventListener(ss->m_mousedown, value, this);
    } else if (name == ss->m_onmousemove) {
        setAttributeEventListener(ss->m_mousemove, value, this);
    } else if (name == ss->m_ontouchstart) {
        setAttributeEventListener(ss->m_touchstart, value, this);
    } else if (name == ss->m_ontouchend) {
        setAttributeEventListener(ss->m_touchend, value, this);
    } else if (name == ss->m_ontouchmove) {
        setAttributeEventListener(ss->m_touchmove, value, this);
    } else if (name == ss->m_onload) {
        setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        setAttributeEventListener(ss->m_unload, value, this);
    } else if (name == ss->m_onerror) {
        setAttributeEventListener(ss->m_error, value, this);
    } else if (name == ss->m_dir) {
        if (attributeCreated) {
            m_hasDirAttribute = true;
        }
        if (attributeRemoved) {
            m_hasDirAttribute = false;
        }
        setNeedsStyleRecalc();
        String* orgValue = value;
        value = value->toLower();
        if (value->equals("")) {
            return;
        } else if (value->equals("ltr") || value->equals("rtl")) {
            if (!orgValue->equals(value)) {
                setAttribute(ss->m_dir, value);
            }
        } else {
            setAttribute(ss->m_dir, String::emptyString);
        }
    } else if (name == ss->m_onfocus) {
        setAttributeEventListener(ss->m_focus, value, this);
    } else if (name == ss->m_onblur) {
        setAttributeEventListener(ss->m_blur, value, this);
    } else if (name == ss->m_onfocusin) {
        setAttributeEventListener(ss->m_focusin, value, this);
    } else if (name == ss->m_onfocusout) {
        setAttributeEventListener(ss->m_focusout, value, this);
    } else if (name == ss->m_onkeydown) {
        setAttributeEventListener(ss->m_keydown, value, this);
    }
}

int HTMLElement::tabIndex()
{
    if (supportsFocus()) {
        return Element::tabIndex();
    }
    return -1;
}

bool HTMLElement::supportsFocus()
{
    return Element::supportsFocus();
}

LayoutRect HTMLElement::offsetRect()
{
    document()->window()->layoutIfNeeds();
    if (frame()) {
        if (frame()->isFrameBox()) {
            FrameBox* box = frame()->asFrameBox();
            return LayoutRect(box->marginLeft(), box->marginTop(),
                              box->contentWidth() + box->paddingWidth() +
                                  box->borderWidth(),
                              box->contentHeight() + box->paddingHeight() +
                                  box->borderHeight());
        }
    }
    return LayoutRect(0, 0, 0, 0);
}

Element* HTMLElement::offsetParent()
{
    Frame* frameObject = frame();
    return frameObject ? frameObject->offsetParent() : nullptr;
}

String* HTMLElement::dir()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_dir);
}

void HTMLElement::setDir(String* dir)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_dir, dir);
}

void HTMLElement::click()
{
    String* eventType =
        document()->window()->starFish()->staticStrings()->m_click.localName();
    dispatchEvent(new Event(eventType, EventInit(true, true)));
}

void HTMLElement::focus()
{
    String* eventType =
        document()->window()->starFish()->staticStrings()->m_focus.localName();
    dispatchEvent(new Event(eventType, EventInit(true, true)));
}

ScriptValue HTMLElement::onclickEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_click;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnclickEventListener(ScriptValue onclick)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_click;

    if (onclick.isObject()) {
        window->setAttributeEventListener(attr, onclick);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLElement::onmouseoverEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_mouseover;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnmouseoverEventListener(ScriptValue onmouseover)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_mouseover;

    if (onmouseover.isObject()) {
        window->setAttributeEventListener(attr, onmouseover);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLElement::onkeydownEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keydown;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnkeydownEventListener(ScriptValue onkeydown)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keydown;

    if (onkeydown.isObject()) {
        window->setAttributeEventListener(attr, onkeydown);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLElement::onkeyupEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_keyup;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnkeyupEventListener(ScriptValue onkeyup);

ScriptValue HTMLElement::onfocusEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_focus;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnfocusEventListener(ScriptValue onfocus)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_focus;

    if (onfocus.isObject()) {
        window->setAttributeEventListener(attr, onfocus);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLElement::onerrorEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_error;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnerrorEventListener(ScriptValue onerror)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_error;

    if (onerror.isObject()) {
        window->setAttributeEventListener(attr, onerror);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLElement::onloadEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_load;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnloadEventListener(ScriptValue onload)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_onload;

    if (onload.isObject()) {
        window->setAttributeEventListener(attr, onload);
    } else {
        window->clearAttributeEventListener(attr);
    }
}

ScriptValue HTMLElement::onunloadEventListener()
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;
    return window->attributeEventListener(attr);
}

void HTMLElement::setOnunloadEventListener(ScriptValue onunload)
{
    Window* window = document()->window();
    QualifiedName attr = window->starFish()->staticStrings()->m_unload;

    if (onunload.isObject()) {
        window->setAttributeEventListener(attr, onunload);
    } else {
        window->clearAttributeEventListener(attr);
    }
}
}
