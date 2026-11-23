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
#include "binding/ScriptBindingInstance.h"
#include "core/style/FontFace.h"
#include "EscargotPublic.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/WebFont.h"
#include "platform/loader/Resource.h"
#include "platform/loader/ResourceLoader.h"

namespace Starfish {

DEFINE_EVENT_LISTENER(FontFaceSet, loading);
DEFINE_EVENT_LISTENER(FontFaceSet, loadingdone);
DEFINE_EVENT_LISTENER(FontFaceSet, loadingerror);

static bool isLoadingState(Resource* resource)
{
    return resource->isRequested() &&
           (resource->state() == Resource::State::BeforeSend ||
            resource->state() == Resource::State::Receiving);
}

FontFaceSet::FontFaceSet(Document* document)
    : m_document(document)
    , m_isReadyPromiseFulfilled(false)
    , m_hasFailedFonts(false)
    , m_hasPendingLayoutTask(false)
{
    // Created lazily on first script access, possibly mid-load.
    m_isLoading = hasLoadingFonts();
}

ExecutionContext* FontFaceSet::executionContext() const
{
    return m_document->executionContext();
}

bool FontFaceSet::isLoadingWebFonts(Document* document)
{
    const auto& fonts = document->loadedWebFontList();
    for (size_t i = 0; i < fonts.size(); i++) {
        if (isLoadingState(fonts[i])) {
            return true;
        }
    }
    return false;
}

bool FontFaceSet::hasLoadingFonts() const
{
    return isLoadingWebFonts(m_document);
}

// https://drafts.csswg.org/css-font-loading/#fontfaceset-pending-on-the-environment
bool FontFaceSet::isPendingOnTheEnvironment() const
{
    auto* context = m_document->browsingContext();
    return !m_document->domContentLoadedFired() ||
           m_document->resourceLoader().isDocumentInOpenState() ||
           context->hasPendingStyleSheet() || context->needsStyleRecalc() ||
           context->needsStyleRecalcForWholeDocument() ||
           context->needsFrameTreeBuild() || context->needsLayout();
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

void FontFaceSet::didFinishLayout()
{
    if (m_hasPendingLayoutTask ||
        (!m_isLoading && (!m_readyPromise || m_isReadyPromiseFulfilled)) ||
        isPendingOnTheEnvironment() || hasLoadingFonts()) {
        return;
    }

    // Font completion can run script; never dispatch it inside layout.
    m_hasPendingLayoutTask = true;
    m_document->webView()->messageLoop()->addIdler(
        m_document->window(),
        [](size_t, void* data) {
            auto* fonts = static_cast<FontFaceSet*>(data);
            fonts->m_hasPendingLayoutTask = false;
            fonts->didChangeFontLoadingState();
        },
        this);
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

    bool wasLoading = m_isLoading;
    m_isLoading = false;
    if (m_readyPromise && !m_isReadyPromiseFulfilled) {
        m_isReadyPromiseFulfilled = true;
        m_readyPromise->fulfill(scriptValue());
    }

    if (wasLoading) {
        fireEvent(staticStrings->m_loadingdone.localName());
        if (m_hasFailedFonts) {
            m_hasFailedFonts = false;
            fireEvent(staticStrings->m_loadingerror.localName());
        }
    }
}

static GCVector<String*> parseFontQuery(ExecutionContext* context, String* font,
                                        CSSStyleValuePair& style,
                                        CSSStyleValuePair& weight,
                                        bool rejectReservedFamily = false)
{
    auto raw = font->toUTF8NonGCString();
    CSSTokenVector tokens;
    CSSStyleDeclaration::tokenizeCSSValue(tokens, raw.data(), raw.length(),
                                          "/,", 2, true);

    // CSS-wide keywords cannot stand for a font or for an unquoted
    // family name in a FontFaceSet load query.
    if (rejectReservedFamily) {
        for (auto token : tokens) {
            std::transform(token.begin(), token.end(), token.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            if (token == "inherit" || token == "initial" || token == "unset" ||
                token == "revert" || token == "revert-layer" ||
                token == "default") {
                throw new DOMException(context, DOMException::SYNTAX_ERR);
            }
        }
    }
    CSSStyleValuePair size, lineHeight, fontFamily;
    if (!CSSStyleDeclaration::parseFontShorthand(tokens, &style, &weight, &size,
                                                 &lineHeight, &fontFamily)) {
        throw new DOMException(context, DOMException::SYNTAX_ERR,
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

    return familyNames;
}

static char queryWeight(FontWeightValue weight)
{
    if (weight >= OneHundredFontWeightValue &&
        weight <= NineHundredsFontWeightValue) {
        return weight - OneHundredFontWeightValue + 1;
    }
    if (weight == BoldFontWeightValue || weight == BolderFontWeightValue) {
        return 7;
    }
    return weight == LighterFontWeightValue ? 1 : 4;
}

static int faceMatchRank(WebFont& font, FontStyleValue style, char weight)
{
    FontStyleValue faceStyle = font.fontStyleValue();
    int styleRank = 0;
    if (style != faceStyle) {
        styleRank = style == NormalFontStyleValue
                        ? (faceStyle == ObliqueFontStyleValue ? 1 : 2)
                        : (faceStyle == NormalFontStyleValue ? 2 : 1);
    }
    char faceWeight = font.fontWeightValue();
    int weightRank;
    if (weight < 4) {
        weightRank = faceWeight <= weight ? weight - faceWeight : faceWeight;
    } else if (weight > 5) {
        weightRank =
            faceWeight >= weight ? faceWeight - weight : 9 - faceWeight;
    } else if (faceWeight >= weight && faceWeight <= 5) {
        weightRank = faceWeight - weight;
    } else {
        weightRank = faceWeight < weight ? 5 - faceWeight : faceWeight - 1;
    }
    return styleRank * 10 + weightRank;
}

class FontFaceSetLoad;

class FontFaceSetLoadClient : public ResourceClient {
public:
    FontFaceSetLoadClient(FontResource* resource, FontFaceSetLoad* load)
        : ResourceClient(resource)
        , m_load(load)
    {
    }

    void didLoadFinished() override;
    void didLoadFailed() override;
    void didLoadCanceled() override
    {
        didLoadFailed();
    }

private:
    FontFaceSetLoad* m_load;
};

class FontFaceSetLoad : public gc {
public:
    FontFaceSetLoad(Document* document, Promise* promise,
                    GCVector<FontFace*>&& faces)
        : m_document(document)
        , m_promise(promise)
        , m_faces(std::move(faces))
    {
    }

    void start()
    {
        for (auto* face : m_faces) {
            auto& font = face->font();
            if (font.fromLocal()) {
                m_failed |= !face->loadLocal();
                continue;
            }
            auto* resource = font.fontResource();
            switch (resource->state()) {
            case Resource::Finished:
                break;
            case Resource::Failed:
            case Resource::Canceled:
                m_failed = true;
                break;
            default:
                ++m_pending;
                resource->addResourceClient(
                    new FontFaceSetLoadClient(resource, this));
                resource->requestIfNeeded();
                break;
            }
        }
        didComplete(false);
    }

    void didComplete(bool failed)
    {
        // Settle the promise outside Resource's client iteration. Promise
        // reactions can run script and change styles or start new loads.
        m_failed |= failed;
        if (--m_pending && !m_failed) {
            return;
        }
        if (m_scheduled) {
            return;
        }
        m_scheduled = true;
        m_document->webView()->messageLoop()->addIdler(
            m_document->window(),
            [](size_t, void* data) {
                static_cast<FontFaceSetLoad*>(data)->settle();
            },
            this);
    }

private:
    void settle()
    {
        auto* context = m_document->executionContext();
        if (m_failed) {
            auto* error = new DOMException(context, DOMException::NETWORK_ERR);
            m_promise->reject(error->scriptValue());
            return;
        }

        const auto result = Escargot::Evaluator::execute(
            context->scriptBindingInstance()->scriptContext(),
            [](Escargot::ExecutionStateRef* state,
               FontFaceSetLoad* load) -> Escargot::ValueRef* {
                auto* faces = Escargot::ValueVectorRef::create();
                for (auto* face : load->m_faces) {
                    faces->pushBack(face->scriptValue());
                }
                return Escargot::ArrayObjectRef::create(state, faces);
            },
            this);
        STARFISH_ASSERT(result.isSuccessful());
        m_promise->fulfill(result.result);
    }

    Document* m_document;
    Promise* m_promise;
    GCVector<FontFace*> m_faces;
    // Holds completion while start() is registering clients.
    size_t m_pending{ 1 };
    bool m_failed{ false };
    bool m_scheduled{ false };
};

void FontFaceSetLoadClient::didLoadFinished()
{
    m_load->didComplete(false);
}

void FontFaceSetLoadClient::didLoadFailed()
{
    m_load->didComplete(true);
}

// https://drafts.csswg.org/css-font-loading/#font-face-set-load
Promise* FontFaceSet::load(String* font, String* text)
{
    CSSStyleValuePair style, weight;
    auto families =
        parseFontQuery(executionContext(), font, style, weight, true);
    GCVector<FontFace*> matches;

    // Match the active document rules. The renderer's font list also
    // contains previously resolved rules and is not the document font set.
    auto& resolver = m_document->styleResolver();
    resolver.recalcRuleSetIfNeeds();
    GCVector<std::pair<CSSStyleDeclaration*, WebFont>> candidates;
    for (const auto& rule : resolver.m_webFonts) {
        if (auto face = resolver.resolveWebFont(rule.first, rule.second)) {
            candidates.emplace_back(rule.first, face.value());
        }
    }
    for (auto it = m_faces.begin(); it != m_faces.end();) {
        if (std::none_of(
                candidates.begin(), candidates.end(),
                [&](const std::pair<CSSStyleDeclaration*, WebFont>& item) {
                    return item.first == it->first;
                })) {
            it = m_faces.erase(it);
        } else {
            ++it;
        }
    }

    if (text->length()) {
        auto sample = text->toUTF32NonGCString();
        auto queryStyle = style.fontStyleValue();
        auto requestedWeight = queryWeight(weight.fontWeightValue());
        for (auto* family : families) {
            int bestRank = INT_MAX;
            for (auto& candidate : candidates) {
                auto& face = candidate.second;
                if (face.familyName()->equalsIgnoreCase(family)) {
                    bestRank =
                        std::min(bestRank, faceMatchRank(face, queryStyle,
                                                         requestedWeight));
                }
            }
            for (auto& candidate : candidates) {
                auto& face = candidate.second;
                if (!face.familyName()->equalsIgnoreCase(family) ||
                    faceMatchRank(face, queryStyle, requestedWeight) !=
                        bestRank) {
                    continue;
                }
                auto* ranges = face.unicodeRanges();
                if (ranges && std::none_of(sample.begin(), sample.end(),
                                           [ranges](char32_t ch) {
                                               return ranges->contains(ch);
                                           })) {
                    continue;
                }
                auto& scriptFace = m_faces[candidate.first];
                if (!scriptFace || !(scriptFace->font() == face)) {
                    scriptFace = new FontFace(m_document, face);
                }
                if (std::find(matches.begin(), matches.end(), scriptFace) ==
                    matches.end()) {
                    matches.push_back(scriptFace);
                }
            }
        }
    }

    auto* promise = new Promise(executionContext()->scriptBindingInstance());
    auto* pending =
        new FontFaceSetLoad(m_document, promise, std::move(matches));
    m_document->webView()->messageLoop()->addIdler(
        m_document->window(),
        [](size_t, void* data) {
            static_cast<FontFaceSetLoad*>(data)->start();
        },
        pending);
    return promise;
}

// https://drafts.csswg.org/css-font-loading/#dom-fontfaceset-check
bool FontFaceSet::check(String* font, String* text)
{
    CSSStyleValuePair style, weight;
    auto familyNames = parseFontQuery(executionContext(), font, style, weight);
    // Style and weight matching and the text's unicode-range filtering are
    // not applied: every web font face of a listed family counts as matching.
    auto& webFonts = m_document->webFontList();
    for (size_t i = 0; i < webFonts.size(); i++) {
        auto& webFont = webFonts[i];
        if (webFont.fromLocal()) {
            continue;
        }
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
