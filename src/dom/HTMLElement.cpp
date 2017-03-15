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
}
