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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

String* HTMLIFrameElement::localName()
{
    return starFish()->staticStrings()->m_iframeTagName.localName();
}

QualifiedName HTMLIFrameElement::name()
{
    return starFish()->staticStrings()->m_iframeTagName;
}

void HTMLIFrameElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

String* HTMLIFrameElement::src()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_src);
}

uint32_t HTMLIFrameElement::frameWidth()
{
    String* v = width();
    if (v->length()) {
        return String::parseInt(v);
    }
    return STARFISH_DEFAULT_IFRAME_WIDTH;
}

uint32_t HTMLIFrameElement::frameHeight()
{
    String* v = height();
    if (v->length()) {
        return String::parseInt(v);
    }
    return STARFISH_DEFAULT_IFRAME_HEIGHT;
}

String* HTMLIFrameElement::width()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_width);
}

void HTMLIFrameElement::setWidth(String* width)
{
    setAttribute(starFish()->staticStrings()->m_width, width);
}

String* HTMLIFrameElement::height()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_height);
}

void HTMLIFrameElement::setHeight(String* height)
{
    setAttribute(starFish()->staticStrings()->m_height, height);
}

void HTMLIFrameElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_src) {
        unloadSrc();
        if (value->length() && document()->doesParticipateInRendering()) {
            loadSrc();
        }
    } else if (name == starFish()->staticStrings()->m_width ||
               name == starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    }
}

void HTMLIFrameElement::didNodeAdopted()
{
    HTMLElement::didNodeAdopted();
    if (document()->doesParticipateInRendering()) {
        loadSrc();
    } else {
        unloadSrc();
    }
}

void HTMLIFrameElement::loadSrc()
{
    unloadSrc();
    m_browsingContext = BrowsingContext::create(this);
    String* s = src();
    if (s->length()) {
        m_browsingContext->navigateAsync(
            new ResourceURL(s, document()->documentURI()->baseURI()));
    } else {
        m_browsingContext->navigateAsync(
            new ResourceURL(String::createASCIIString("about:blank")));
    }
}

void HTMLIFrameElement::unloadSrc()
{
    if (m_browsingContext) {
        m_browsingContext->close();
        m_browsingContext = nullptr;
    }
}
}
