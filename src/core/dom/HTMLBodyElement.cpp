/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"

namespace StarFish {

QualifiedName HTMLBodyElement::name()
{
    return starFish()->staticStrings()->m_bodyTagName;
}

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, load);

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, message);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, messageerror);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, unload);

void HTMLBodyElement::didComputedStyleChanged(ComputedStyle* oldStyle,
                                              ComputedStyle* newStyle)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle);
    if (newStyle && (!newStyle->backgroundColor().isTransparent() ||
                     (newStyle->backgroundImage() &&
                      !newStyle->backgroundImage()->type().isNone()))) {
        document()->browsingContext()->m_hasBodyElementBackground = true;
    } else {
        document()->browsingContext()->m_hasBodyElementBackground = false;
    }
}

void HTMLBodyElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    StaticStrings* ss = starFish()->staticStrings();
    if (name == ss->m_onload) {
        window()->setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onunload) {
        window()->setAttributeEventListener(ss->m_unload, value, this);
    } else if (name == ss->m_alink) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    } else if (name == ss->m_background) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    } else if (name == ss->m_bgcolor) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    } else if (name == ss->m_link) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    } else if (name == ss->m_text) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    } else if (name == ss->m_vlink) {
        setNeedsStyleRecalc(StyleChangeReason::AttributeChange);
    }
}

void HTMLBodyElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    HTMLElement::styleForPresentationAttribute(cssValues);

    // TODO add alink, link, vlink when :visited, :link implemented

    // attr background
    {
        CSSStyleValuePair containerPair;
        CSSStyleValuePair pair;
        String* value =
            getAttributeOrEmpty(starFish()->staticStrings()->m_background);
        if (value->length()) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundImage);
            pair.setValueKind(CSSStyleValuePair::ValueKind::UrlValueKind);
            pair.setUrlValue(value);

            ValueList* values =
                new ValueList(ValueList::Separator::CommaSeparator);
            values->push_back(pair);
            containerPair.setKeyKind(
                CSSStyleValuePair::KeyKind::BackgroundImage);
            containerPair.setValueList(values);
            cssValues.push_back(containerPair);
        }
    }

    // attr bgcolor
    {
        CSSStyleValuePair pair;
        String* value =
            getAttributeOrEmpty(starFish()->staticStrings()->m_bgcolor);
        if (value->length()) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundColor);
            CSSTokenVector v;
            v.push_back(value->toUTF8NonGCString());
            if (pair.updateValueColor(document(), v)) {
                cssValues.push_back(pair);
            }
        }
    }

    // attr text
    {
        CSSStyleValuePair pair;
        String* value =
            getAttributeOrEmpty(starFish()->staticStrings()->m_text);
        if (value->length()) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Color);
            CSSTokenVector v;
            v.push_back(value->toUTF8NonGCString());
            if (pair.updateValueColor(document(), v)) {
                cssValues.push_back(pair);
            }
        }
    }
}
}
