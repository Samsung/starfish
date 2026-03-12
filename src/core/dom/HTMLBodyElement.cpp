/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/HTMLBodyElement.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/ComputedStyle.h"
#include "core/style/CSSParser.h"

namespace Starfish {

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, blur);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, error);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, focus);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, load);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, resize);
// DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, load);

DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, message);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, messageerror);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, unload);
DEFINE_GLOBAL_EVENT_LISTENER(HTMLBodyElement, hashchange);

void HTMLBodyElement::didComputedStyleChanged(
    ComputedStyle* oldStyle, ComputedStyle* newStyle,
    Optional<StyleResolveContext*> ctx)
{
    HTMLElement::didComputedStyleChanged(oldStyle, newStyle, ctx);
    if (newStyle && (!newStyle->backgroundColor().isTransparent() ||
                     newStyle->backgroundLayerSize())) {
        document()->browsingContext()->m_hasBodyElementBackground = true;
        document()->setNeedsPainting();
    } else {
        document()->browsingContext()->m_hasBodyElementBackground = false;
        document()->setNeedsPainting();
    }
}

void HTMLBodyElement::didAttributeChanged(QualifiedName name,
                                          Optional<String*> old, String* value,
                                          bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();
    if (name == ss->m_onblur) {
        window()->setAttributeEventListener(ss->m_blur, value, this);
    } else if (name == ss->m_onerror) {
        window()->setAttributeEventListener(ss->m_error, value, this);
    } else if (name == ss->m_onfocus) {
        window()->setAttributeEventListener(ss->m_focus, value, this);
    } else if (name == ss->m_onload) {
        window()->setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onresize) {
        window()->setAttributeEventListener(ss->m_resize, value, this);
    } else if (name == ss->m_onmessage) {
        window()->setAttributeEventListener(ss->m_message, value, this);
    } else if (name == ss->m_onmessageerror) {
        window()->setAttributeEventListener(ss->m_messageerror, value, this);
    } else if (name == ss->m_onunload) {
        window()->setAttributeEventListener(ss->m_unload, value, this);
    } else if (name == ss->m_alink) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_background) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_bgcolor) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_link) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_text) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_vlink) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_marginwidth) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_leftmargin) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_rightmargin) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_marginheight) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_topmargin) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    } else if (name == ss->m_bottommargin) {
        setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
    }
}

static std::pair<bool, CSSStyleValuePair> parseStyleLength(String* input)
{
    STARFISH_ASSERT(input != nullptr);
    CSSStyleValuePair pair;
    if (input->length() == 0) {
        return std::make_pair(false, pair);
    }
    CSSTokenVector tokens;
    CSSTokenValue token(input->toUTF8NonGCString());
    tokens.push_back(token);
    if (pair.updateValueLength(tokens, CSSPropertyParser::AllowWithoutUnit)) {
        return std::make_pair(true, pair);
    }
    return std::make_pair(false, pair);
}

static bool parseMarginWidthKind(String* input,
                                 CSSStyleValuePairVectorHolder& cssValues)
{
    STARFISH_ASSERT(input != nullptr);
    auto test = parseStyleLength(input);
    if (test.first) {
        test.second.setKeyKind(CSSStyleValuePair::KeyKind::MarginLeft);
        cssValues.push_back(test.second);
        test.second.setKeyKind(CSSStyleValuePair::KeyKind::MarginRight);
        cssValues.push_back(test.second);
        return true;
    }
    return false;
}

static bool parseMarginHeightKind(String* input,
                                  CSSStyleValuePairVectorHolder& cssValues)
{
    STARFISH_ASSERT(input != nullptr);
    auto test = parseStyleLength(input);
    if (test.first) {
        test.second.setKeyKind(CSSStyleValuePair::KeyKind::MarginTop);
        cssValues.push_back(test.second);
        test.second.setKeyKind(CSSStyleValuePair::KeyKind::MarginBottom);
        cssValues.push_back(test.second);
        return true;
    }
    return false;
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
            getAttributeOrEmpty(starfish()->staticStrings()->m_background);
        if (value->length()) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::BackgroundImage);
            pair.setValueKind(CSSStyleValuePair::ValueKind::UrlValueKind);
            pair.setUrlValue(value);

            ValueList* values = new ValueList(Separator::CommaSeparator);
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
            getAttributeOrEmpty(starfish()->staticStrings()->m_bgcolor);
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
            getAttributeOrEmpty(starfish()->staticStrings()->m_text);
        if (value->length()) {
            pair.setKeyKind(CSSStyleValuePair::KeyKind::Color);
            CSSTokenVector v;
            v.push_back(value->toUTF8NonGCString());
            if (pair.updateValueColor(document(), v)) {
                cssValues.push_back(pair);
            }
        }
    }

    // https://html.spec.whatwg.org/multipage/rendering.html#the-page
    // attr leftmargin
    // attr rightmargin
    // attr marginwidth
    // attr parent marginwidth
    {
        CSSStyleValuePair pair;
        bool parsed = false;
        // attr parent marginwidth
        if (!document()->browsingContext()->isTopLevelBrowsingContext()) {
            String* value = document()
                                ->browsingContext()
                                ->sourceElement()
                                ->getAttributeOrEmpty(
                                    starfish()->staticStrings()->m_marginwidth);
            parsed = parseMarginWidthKind(value, cssValues);
        }

        // marginwidth
        if (!parsed) {
            String* value =
                getAttributeOrEmpty(starfish()->staticStrings()->m_marginwidth);
            parsed = parseMarginWidthKind(value, cssValues);
        }

        // leftmargin
        if (!parsed) {
            String* value =
                getAttributeOrEmpty(starfish()->staticStrings()->m_leftmargin);
            parsed = parseMarginWidthKind(value, cssValues);
        }

        // rightmargin
        if (!parsed) {
            String* value =
                getAttributeOrEmpty(starfish()->staticStrings()->m_rightmargin);
            parsed = parseMarginWidthKind(value, cssValues);
        }
    }

    // attr topmargin
    // attr bottommargin
    // attr marginheight
    // attr parent marginheight
    {
        CSSStyleValuePair pair;
        bool parsed = false;
        // attr parent marginheight
        if (!document()->browsingContext()->isTopLevelBrowsingContext()) {
            String* value =
                document()
                    ->browsingContext()
                    ->sourceElement()
                    ->getAttributeOrEmpty(
                        starfish()->staticStrings()->m_marginheight);
            parsed = parseMarginHeightKind(value, cssValues);
        }

        // marginheight
        if (!parsed) {
            String* value = getAttributeOrEmpty(
                starfish()->staticStrings()->m_marginheight);
            parsed = parseMarginHeightKind(value, cssValues);
        }

        // topmargin
        if (!parsed) {
            String* value =
                getAttributeOrEmpty(starfish()->staticStrings()->m_topmargin);
            parsed = parseMarginHeightKind(value, cssValues);
        }

        // bottommargin
        if (!parsed) {
            String* value = getAttributeOrEmpty(
                starfish()->staticStrings()->m_bottommargin);
            parsed = parseMarginHeightKind(value, cssValues);
        }
    }
}

bool HTMLBodyElement::isPotentiallyScrollable()
{
    // https://drafts.csswg.org/cssom-view/#potentially-scrollable
    auto pao = parentElement()->appliedOverflow();
    auto ao = appliedOverflow();
    if (pao.first > OverflowValue::VisibleOverflow &&
        pao.second > OverflowValue::VisibleOverflow &&
        ao.first > OverflowValue::VisibleOverflow &&
        ao.second > OverflowValue::VisibleOverflow) {
        return true;
    }
    return false;
}
} // namespace Starfish
