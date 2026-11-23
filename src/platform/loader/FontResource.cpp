/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/style/FontFaceSet.h"
#include "core/style/FontFaceSrcData.h"
#include "platform/loader/FontResource.h"
#include "platform/loader/ResourceLoader.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/Window.h"
#include "core/page/BrowsingContext.h"

namespace Starfish {

// A src list represents one face to layout and the Font Loading API. Each
// URL attempt has its own resource so the URL cache keeps its own result.
class FontSourceListResource : public FontResource {
public:
    FontSourceListResource(ResourceURL* base, ResourceLoader* loader,
                           FontFaceSrcData* sources, String* family, char style,
                           char weight)
        : FontResource(base, loader)
        , m_sources(sources)
        , m_family(family)
        , m_style(style)
        , m_weight(weight)
    {
    }

    bool matchesSource(ResourceURL* base, FontFaceSrcData* sources,
                       String* family, char style, char weight) override
    {
        return sources && m_style == style && m_weight == weight &&
               m_family->equals(family) &&
               url()->urlString()->equals(base->urlString()) &&
               (m_sources == sources || m_sources->equals(sources));
    }

    void request(RequestData*, bool) override
    {
        m_isRequested = true;
        tryNextSource();
    }

private:
    class CandidateClient : public ResourceClient {
    public:
        CandidateClient(FontResource* candidate, FontSourceListResource* list)
            : ResourceClient(candidate)
            , m_list(list)
        {
        }

        void didLoadFinished() override
        {
            m_list->m_fontFace = resource()->asFontResource()->fontFace();
            m_list->Resource::didLoadFinished();
        }

        void didLoadFailed() override
        {
            m_list->tryNextSource();
        }

        void didLoadCanceled() override
        {
            m_list->Resource::didLoadCanceled();
        }

    private:
        FontSourceListResource* m_list;
    };

    void tryNextSource()
    {
        // A refused request or cache hit may complete inside requestIfNeeded.
        // Continue synchronous failures in this loop without growing the stack.
        if (m_tryingSource) {
            return;
        }
        m_tryingSource = true;
        auto* document = loader()->document();
        while (m_next < m_sources->data().size()) {
            const auto& source = m_sources->data()[m_next++];
            auto* name = std::get<0>(source);
            if (std::get<1>(source) == FontFaceSrcData::Local) {
                m_fontFace = document->fontSelector()->loadLocalWebFont(
                    name, m_style, m_weight);
                if (m_fontFace) {
                    Resource::didLoadFinished();
                    break;
                }
                continue;
            }
            if (std::get<2>(source) == FontFaceSrcData::Unknown) {
                continue;
            }
            auto* candidate =
                loader()->fetchFont(new ResourceURL(name, url()->urlString()));
            candidate->addResourceClient(new CandidateClient(candidate, this));
            candidate->requestIfNeeded();
            if (!candidate->isFaildToFetchResource()) {
                m_tryingSource = false;
                return;
            }
        }
        m_tryingSource = false;
        if (!isFinished()) {
            Resource::didLoadFailed();
        }
    }

    FontFaceSrcData* m_sources;
    String* m_family;
    char m_style;
    char m_weight;
    bool m_tryingSource = false;
    size_t m_next = 0;
};

FontResource* FontResource::createSourceList(ResourceURL* base,
                                             ResourceLoader* loader,
                                             FontFaceSrcData* sources,
                                             String* family, char style,
                                             char weight)
{
    return new FontSourceListResource(base, loader, sources, family, style,
                                      weight);
}

void FontResource::requestIfNeeded()
{
    if (isRequested()) {
        return;
    }
    Document* document = loader()->document();
    RequestData* requestData = new RequestData();
    requestData->m_url = url();
    requestData->m_referrer = new ReferrerURL(document->documentURI());
    requestData->m_destination = RequestDestination::Font;
    requestData->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;

    request(requestData, true);
    if (auto fonts = document->fontsIfCreated()) {
        fonts.value()->didChangeFontLoadingState();
    }
}

void FontResource::didLoadFinished()
{
    m_fontFace = PlatformFontFace::create(
        (const uint8_t*)m_resourceRequest->response().data(),
        m_resourceRequest->response().size());
    if (!m_fontFace) {
        Resource::didLoadFailed();
        return;
    }
    Resource::didLoadFinished();
}
} // namespace Starfish
