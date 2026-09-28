/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "core/style/FontFaceSet.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "binding/ScriptWrappable.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/WebFont.h"
#include "core/modules/message_loop/Timer.h"
#include "platform/loader/Resource.h"

namespace Starfish {

DEFINE_EVENT_LISTENER(FontFaceSet, loading);
DEFINE_EVENT_LISTENER(FontFaceSet, loadingdone);
DEFINE_EVENT_LISTENER(FontFaceSet, loadingerror);

FontFaceSet::FontFaceSet(Document* document)
    : m_document(document)
    , m_status(LoadStatus::Loaded)
{
}

ExecutionContext* FontFaceSet::executionContext() const
{
    return m_document->executionContext();
}

String* FontFaceSet::status() const
{
    if (m_status == LoadStatus::Loading) {
        return String::fromUTF8("loading");
    }
    return String::fromUTF8("loaded");
}

Promise* FontFaceSet::ready()
{
    if (!m_readyPromise) {
        m_readyPromise = new Promise(
            m_document->executionContext()->scriptBindingInstance());
        if (m_status == LoadStatus::Loaded) {
            fulfillReadyPromise();
        }
    }
    return m_readyPromise;
}

void FontFaceSet::fulfillReadyPromise()
{
    if (m_readyPromise && !m_fulfilled) {
        m_fulfilled = true;
        m_readyPromise->fulfill(scriptValue());
    }
}

void FontFaceSet::didStartFontLoading()
{
    m_status = LoadStatus::Loading;
    if (m_fulfilled) {
        m_readyPromise = new Promise(
            m_document->executionContext()->scriptBindingInstance());
        m_fulfilled = false;
    }

    if (!m_fallbackTimerId) {
        m_fallbackTimerId = m_document->window()->setTimeout(
            [](void* data) {
                FontFaceSet* self = static_cast<FontFaceSet*>(data);
                self->m_fallbackTimerId = 0;
                self->didFinishFontLoading();
            },
            3000, this);
    }

    Event* event = new Event(executionContext(), m_document->window()
                                                     ->starfish()
                                                     ->staticStrings()
                                                     ->m_loading.localName());
    dispatchEvent(event);
}

void FontFaceSet::didFinishFontLoading()
{
    const auto& loadedFonts = m_document->loadedWebFontList();
    for (size_t i = 0; i < loadedFonts.size(); i++) {
        FontResource* res = loadedFonts[i];
        if (res && (res->state() == Resource::State::BeforeSend ||
                    res->state() == Resource::State::Receiving)) {
            return;
        }
    }

    if (m_fallbackTimerId) {
        m_document->window()->clearTimeout(m_fallbackTimerId);
        m_fallbackTimerId = 0;
    }

    m_status = LoadStatus::Loaded;
    fulfillReadyPromise();

    Event* event =
        new Event(executionContext(), m_document->window()
                                          ->starfish()
                                          ->staticStrings()
                                          ->m_loadingdone.localName());
    dispatchEvent(event);
}

void FontFaceSet::checkReadyState()
{
    if (m_status == LoadStatus::Loading) {
        didFinishFontLoading();
    } else {
        fulfillReadyPromise();
    }
}

bool FontFaceSet::check(String* font, String* text)
{
    if (!font || font->length() == 0) {
        return true;
    }

    auto raw = font->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length(),
                                          "/,", 2, true);

    CSSStyleValuePair style, weight, size, lineHeight, fontFamily;
    bool parsed = CSSStyleDeclaration::parseFontShorthand(
        tokens, &style, &weight, &size, &lineHeight, &fontFamily);

    if (!parsed || fontFamily.valueKind() !=
                       CSSStyleValuePair::ValueKind::KeywordValueKind) {
        return true;
    }

    String* familyName = fontFamily.keywordValue();
    auto& webFonts = m_document->webFontList();
    for (size_t i = 0; i < webFonts.size(); i++) {
        auto& webFont = webFonts[i];
        if (webFont.familyName()->equals(familyName)) {
            FontResource* res = webFont.fontResource();
            if (res && (res->state() == Resource::State::BeforeSend ||
                        res->state() == Resource::State::Receiving)) {
                return false;
            }
        }
    }

    return true;
}

} // namespace Starfish
