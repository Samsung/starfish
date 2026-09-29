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
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "binding/ScriptWrappable.h"
#include "core/page/Window.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/WebFont.h"
#include "platform/loader/Resource.h"

namespace Starfish {

DEFINE_EVENT_LISTENER(FontFaceSet, loading);
DEFINE_EVENT_LISTENER(FontFaceSet, loadingdone);
DEFINE_EVENT_LISTENER(FontFaceSet, loadingerror);

static bool isLoadingState(Resource* resource)
{
    return resource->state() == Resource::State::BeforeSend ||
           resource->state() == Resource::State::Receiving;
}

FontFaceSet::FontFaceSet(Document* document)
    : m_document(document)
    , m_isReadyPromiseFulfilled(false)
    , m_hasFailedFonts(false)
{
    // Created lazily on first script access, possibly mid-load.
    m_isLoading = hasLoadingFonts();
}

ExecutionContext* FontFaceSet::executionContext() const
{
    return m_document->executionContext();
}

bool FontFaceSet::hasLoadingFonts() const
{
    const auto& fonts = m_document->loadedWebFontList();
    for (size_t i = 0; i < fonts.size(); i++) {
        if (isLoadingState(fonts[i])) {
            return true;
        }
    }
    return false;
}

// https://drafts.csswg.org/css-font-loading/#fontfaceset-pending-on-the-environment
bool FontFaceSet::isPendingOnTheEnvironment() const
{
    return !m_document->domContentLoadedFired();
}

String* FontFaceSet::status() const
{
    auto staticStrings = m_document->window()->starfish()->staticStrings();
    return m_isLoading ? staticStrings->m_loading.localName()
                       : staticStrings->m_loaded.localName();
}

// https://drafts.csswg.org/css-font-loading/#dom-fontfaceset-ready
Promise* FontFaceSet::ready()
{
    if (!m_readyPromise) {
        m_readyPromise = new Promise(
            m_document->executionContext()->scriptBindingInstance());
        m_isReadyPromiseFulfilled = false;
        if (!m_isLoading && !isPendingOnTheEnvironment()) {
            m_isReadyPromiseFulfilled = true;
            m_readyPromise->fulfill(scriptValue());
        }
    }
    return m_readyPromise;
}

void FontFaceSet::fireEvent(String* type)
{
    dispatchEvent(new Event(executionContext(), type));
}

// https://drafts.csswg.org/css-font-loading/#switch-the-fontfaceset-to-loading
// https://drafts.csswg.org/css-font-loading/#switch-the-fontfaceset-to-loaded
void FontFaceSet::didChangeFontLoadingState(bool fontLoadFailed)
{
    if (fontLoadFailed) {
        m_hasFailedFonts = true;
    }

    auto staticStrings = m_document->window()->starfish()->staticStrings();
    bool isLoading = hasLoadingFonts();
    if (isLoading && !m_isLoading) {
        m_isLoading = true;
        // A fulfilled ready promise is replaced by a new pending one; drop
        // it and let the next ready() access create that.
        if (m_isReadyPromiseFulfilled) {
            m_readyPromise = nullptr;
            m_isReadyPromiseFulfilled = false;
        }
        fireEvent(staticStrings->m_loading.localName());
        return;
    }

    if (isLoading || isPendingOnTheEnvironment()) {
        return;
    }

    if (m_readyPromise && !m_isReadyPromiseFulfilled) {
        m_isReadyPromiseFulfilled = true;
        m_readyPromise->fulfill(scriptValue());
    }

    if (m_isLoading) {
        m_isLoading = false;
        fireEvent(staticStrings->m_loadingdone.localName());
        if (m_hasFailedFonts) {
            m_hasFailedFonts = false;
            fireEvent(staticStrings->m_loadingerror.localName());
        }
    }
}

// https://drafts.csswg.org/css-font-loading/#dom-fontfaceset-check
bool FontFaceSet::check(String* font, String* text)
{
    auto raw = font->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length(),
                                          "/,", 2, true);

    CSSStyleValuePair style, weight, size, lineHeight, fontFamily;
    if (!CSSStyleDeclaration::parseFontShorthand(tokens, &style, &weight, &size,
                                                 &lineHeight, &fontFamily)) {
        throw new DOMException(executionContext(), DOMException::SYNTAX_ERR,
                               "Could not parse the font specification");
    }

    GCVector<String*> familyNames;
    if (fontFamily.valueKind() ==
        CSSStyleValuePair::ValueKind::KeywordValueKind) {
        familyNames.push_back(fontFamily.keywordValue());
    } else if (fontFamily.valueKind() ==
               CSSStyleValuePair::ValueKind::ValueListKind) {
        ValueList* list = fontFamily.multiValue();
        for (size_t i = 0; i < list->size(); i++) {
            familyNames.push_back(list->at(i).keywordValue());
        }
    }

    // Style and weight matching and the text's unicode-range filtering are
    // not applied: every web font face of a listed family counts as matching.
    auto& webFonts = m_document->webFontList();
    for (size_t i = 0; i < webFonts.size(); i++) {
        auto& webFont = webFonts[i];
        FontResource* resource = webFont.fontResource();
        if (!resource || !isLoadingState(resource)) {
            continue;
        }
        for (size_t j = 0; j < familyNames.size(); j++) {
            if (webFont.familyName()->equalsIgnoreCase(familyNames[j])) {
                return false;
            }
        }
    }
    return true;
}

} // namespace Starfish
