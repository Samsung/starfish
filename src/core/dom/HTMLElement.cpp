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

#include "core/dom/HTMLElement.h"

#include "StarFish.h"
#include "core/dom/Event.h"
#include "core/dom/Text.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLBRElement.h"
#include "core/layout/FrameBox.h"
#include "core/layout/FrameBlockBox.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"

namespace StarFish {

void* HTMLElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void HTMLElement::didAttributeChanged(QualifiedName name, String* old,
                                      String* value, bool attributeCreated,
                                      bool attributeRemoved)
{
    Element::didAttributeChanged(name, old, value, attributeCreated,
                                 attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();
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
    } else if (name == ss->m_onresize) {
        setAttributeEventListener(ss->m_resize, value, this);
    }
}

void HTMLElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    if (m_hasDirAttribute) {
        CSSStyleValuePair pair;
        String* str = getAttributeOrEmpty(starFish()->staticStrings()->m_dir);
        str = str->toLower();
        if (str->equals("ltr")) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Direction);
            pair.setValueKind(CSSStyleValuePair::ValueKind::DirectionValueKind);
            pair.setValue(DirectionValue::LtrDirectionValue);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::KeyKind::UnicodeBidi);
            pair.setValueKind(
                CSSStyleValuePair::ValueKind::UnicodeBidiValueKind);
            pair.setValue(UnicodeBidiValue::IsolateUnicodeBidiValue);
            cssValues.push_back(pair);
        } else if (str->equals("rtl")) {
            CSSStyleValuePair pair;
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Direction);
            pair.setValueKind(CSSStyleValuePair::ValueKind::DirectionValueKind);
            pair.setValue(DirectionValue::RtlDirectionValue);
            cssValues.push_back(pair);

            pair.setKeyKind(CSSStyleValuePair::KeyKind::UnicodeBidi);
            pair.setValueKind(
                CSSStyleValuePair::ValueKind::UnicodeBidiValueKind);
            pair.setValue(UnicodeBidiValue::IsolateUnicodeBidiValue);
            cssValues.push_back(pair);
        } else {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Direction);
            pair.setValueKind(CSSStyleValuePair::ValueKind::DirectionValueKind);
            pair.setValue(DirectionValue::LtrDirectionValue);
            cssValues.push_back(pair);
        }
    }
}

int HTMLElement::tabIndex() const
{
    if (supportsFocus()) {
        return Element::tabIndex();
    }
    return -1;
}

void HTMLElement::setTabIndex(int index, bool setExplicitly)
{
    Element::setTabIndex(index, setExplicitly);
}

LayoutRect HTMLElement::offsetRect()
{
    window()->browsingContext()->webView()->layoutIfNeeds();
    if (frame()) {
        Frame* frameObject = frame();
        if (frameObject->isFrameBox()) {
            FrameBox* box = frameObject->asFrameBox();
            return LayoutRect(box->marginLeft(), box->marginTop(),
                              box->contentWidth() + box->paddingWidth() +
                                  box->borderWidth(),
                              box->contentHeight() + box->paddingHeight() +
                                  box->borderHeight());
        } else {
            if (frameObject->isFrameInline()) {
                Frame* nearestFrameBox = frameObject->parent();
                while (!nearestFrameBox->isFrameBox()) {
                    nearestFrameBox = nearestFrameBox->parent();
                }

                if (nearestFrameBox) {
                    Node* offsetParentNode = frameObject->offsetParent();
                    FrameBox* offsetParent;
                    if (!offsetParentNode) {
                        offsetParent = document()->frame()->asFrameBox();
                    } else if (offsetParentNode->frame()->isFrameBox()) {
                        offsetParent = offsetParentNode->frame()->asFrameBox();
                    } else {
                        offsetParent = document()->frame()->asFrameBox();
                    }
                    FrameBox* box = nearestFrameBox->asFrameBox();
                    LayoutRect result(0, 0, 0, 0);
                    box->iterateChildFrameBox([&](FrameBox* childBox) {
                        if (childBox->isInlineNonReplacedBox()) {
                            if (childBox->asInlineNonReplacedBox()
                                    ->origin()
                                    ->node() == this) {
                                LayoutRect rect =
                                    childBox->absoluteRect(offsetParent);

                                result.unite(rect);
                            }
                        }
                    });
                    return result;
                }
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        }
    } else {
    }
    return LayoutRect(0, 0, 0, 0);
}

Element* HTMLElement::offsetParent()
{
    Frame* frameObject = frame();
    return frameObject ? frameObject->offsetParent() : nullptr;
}

String* HTMLElement::innerText()
{
    // TODO
    // https://html.spec.whatwg.org/multipage/dom.html#the-innertext-idl-attribute
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    auto v = textContent();
    return v.hasValue() ? v.getValue() : String::emptyString;
}

void HTMLElement::setInnerText(String* text)
{
    while (firstChild()) {
        removeChild(firstChild());
    }

    GCVector<StringView> v = StringUtils::tokenize(text, "\r\n", 2);
    for (size_t i = 0; i < v.size(); i++) {
        appendChild(new Text(document(), new StringView(v[i])));
        if (i + 1 < v.size()) {
            appendChild(new HTMLBRElement(document()));
        }
    }
}

String* HTMLElement::dir()
{
    String* dir =
        getAttributeOrEmpty(starFish()->staticStrings()->m_dir)->toLower();
    if (dir->equals("ltr") || dir->equals("rtl") || dir->equals("auto")) {
        return dir;
    }
    return String::emptyString;
}

void HTMLElement::setDir(String* dir)
{
    setAttribute(starFish()->staticStrings()->m_dir, dir);
}

String* HTMLElement::title()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_title);
}

void HTMLElement::setTitle(String* title)
{
    setAttribute(starFish()->staticStrings()->m_title, title);
}

String* HTMLElement::lang()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_lang);
}

void HTMLElement::setLang(String* lang)
{
    setAttribute(starFish()->staticStrings()->m_lang, lang);
}

void HTMLElement::click()
{
    String* eventType = starFish()->staticStrings()->m_click.localName();
    dispatchEvent(new Event(document(), eventType, EventInit(true, true)));
}

DEFINE_EVENT_LISTENER(HTMLElement, abort);
DEFINE_EVENT_LISTENER(HTMLElement, canplay);
DEFINE_EVENT_LISTENER(HTMLElement, canplaythrough);
DEFINE_EVENT_LISTENER(HTMLElement, click);
DEFINE_EVENT_LISTENER(HTMLElement, durationchange);
DEFINE_EVENT_LISTENER(HTMLElement, emptied);
DEFINE_EVENT_LISTENER(HTMLElement, ended);
DEFINE_EVENT_LISTENER(HTMLElement, error);
DEFINE_EVENT_LISTENER(HTMLElement, focus);
DEFINE_EVENT_LISTENER(HTMLElement, keydown);
DEFINE_EVENT_LISTENER(HTMLElement, keyup);
DEFINE_EVENT_LISTENER(HTMLElement, load);
DEFINE_EVENT_LISTENER(HTMLElement, loadeddata);
DEFINE_EVENT_LISTENER(HTMLElement, loadedmetadata);
DEFINE_EVENT_LISTENER(HTMLElement, loadstart);
DEFINE_EVENT_LISTENER(HTMLElement, mousedown);
DEFINE_EVENT_LISTENER(HTMLElement, mousemove);
DEFINE_EVENT_LISTENER(HTMLElement, mouseover);
DEFINE_EVENT_LISTENER(HTMLElement, mouseout);
DEFINE_EVENT_LISTENER(HTMLElement, mouseup);
DEFINE_EVENT_LISTENER(HTMLElement, pause);
DEFINE_EVENT_LISTENER(HTMLElement, play);
DEFINE_EVENT_LISTENER(HTMLElement, playing);
DEFINE_EVENT_LISTENER(HTMLElement, progress);
DEFINE_EVENT_LISTENER(HTMLElement, ratechange);
DEFINE_EVENT_LISTENER(HTMLElement, resize);
DEFINE_EVENT_LISTENER(HTMLElement, seeked);
DEFINE_EVENT_LISTENER(HTMLElement, seeking);
DEFINE_EVENT_LISTENER(HTMLElement, stalled);
DEFINE_EVENT_LISTENER(HTMLElement, suspend);
DEFINE_EVENT_LISTENER(HTMLElement, timeupdate);
DEFINE_EVENT_LISTENER(HTMLElement, volumechange);
DEFINE_EVENT_LISTENER(HTMLElement, waiting);
}
