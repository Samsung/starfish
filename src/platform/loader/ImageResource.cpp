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
#include "platform/loader/ImageResource.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/file/File.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/Window.h"
#include "core/extra/MimeType.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Traverse.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"

namespace StarFish {

class MockHTMLIFrameElement : public HTMLIFrameElement {
public:
    MockHTMLIFrameElement(Document* document, ImageResource* resource)
        : HTMLIFrameElement(document)
        , m_resource(resource)
    {
    }

    void* operator new(size_t size)
    {
        STARFISH_ASSERT(size == sizeof(MockHTMLIFrameElement));
        static bool typeInited = false;
        static GC_descr descr;
        if (!typeInited) {
            GC_word desc[GC_BITMAP_SIZE(MockHTMLIFrameElement)] = { 0 };
            GC_set_bit(desc, GC_WORD_OFFSET(MockHTMLIFrameElement, m_resource));
            HTMLIFrameElement::fillGCDescriptor(desc);
            descr =
                GC_make_descriptor(desc, GC_WORD_LEN(MockHTMLIFrameElement));
            typeInited = true;
        }
        return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
    }
    void* operator new[](size_t size) = delete;

protected:
    virtual void childBrowsingContextLoaded()
    {
        HTMLIFrameElement::childBrowsingContextLoaded();
        Node* r =
            Traverse::findDescendant(contentDocument(), [](Node* n) -> bool {
                if (n->isSVGSVGElement()) {
                    return true;
                }
                return false;
            });
        if (r) {
            m_browsingContext->layoutIfNeeds(false);
            SVGSVGElement* svg = r->asSVGSVGElement();
            if (!svg->frame()) {
                m_resource->didLoadFailed();
            }
            FrameSVGSVGBox* svgBox = (FrameSVGSVGBox*)svg->frame();

            size_t w = (int)svgBox->width();
            size_t h = (int)svgBox->height();
            NativeImageData* imageData = NativeImageData::create(w, h);
            imageData->clear();
            Canvas* canvas = Canvas::createGenericCanvas(
                m_browsingContext->starFish(), imageData->data(),
                imageData->width(), imageData->height());
            svgBox->paintReplaced(canvas);
            delete canvas;

            m_resource->m_imageData = imageData;
            m_resource->m_mockFrameForSVGDocument = nullptr;
            m_resource->m_imageData->setPreserveAspectRatioValue(
                svg->preserveAspectRatioValue());
            m_resource->Resource::didLoadFinished();
        }
    }

    ImageResource* m_resource;
};

#if defined(PORT_CANVAS_BACKEND_EFL)
void ImageResource::doLoadFile(void* data)
{
    Resource* res = (Resource*)data;
    // special path for image resource
    if (res->url()->isFileURL()) {
        File* fio = File::create();
        // NOTE
        // we should special logic to load file url for image
        // we can pass src of image to platform layer in efl
        String* path = res->url()->urlStringWithoutSearchPart();
        String* filePath = path->substring(7, path->length() - 7);
        bool canLoad = fio->open(filePath, File::Read);
        delete fio;
        if (!canLoad) {
            res->didLoadFailed();
            return;
        }
        NativeImageData* id = NativeImageData::create(filePath);
        if (!id) {
            res->didLoadFailed();
            return;
        }
        res->asImageResource()->m_imageData = id;
        res->didLoadFinished();
    } else {
        res->didLoadFailed();
    }
}
#endif

void ImageResource::request(ResourceRequestSyncLevel syncLevel,
                            ResourceURL* referrerURL, bool allowCache)
{
#if defined(PORT_CANVAS_BACKEND_EFL)
    if (m_url->isFileURL() && !m_url->urlString()->endsWith(".svg", false)) {
        if (!loader()->requestResourcePreprocess(this, syncLevel)) {
            // cache miss
            if (ResourceRequestSyncLevel::NeverSync != syncLevel) {
                doLoadFile(this);
            } else {
                pushIdlerHandle(m_loader->starFish()->messageLoop()->addIdler(
                    loader()->document()->browsingContext(),
                    [](size_t handle, void* data) {
                        Resource* res = (Resource*)data;
                        res->removeIdlerHandle(handle);
                        res->asImageResource()->doLoadFile(data);
                    },
                    this));
            }
        }
    } else {
        Resource::request(syncLevel, referrerURL, allowCache);
    }
#else
    Resource::request(syncLevel, referrerURL, allowCache);
#endif
}

void ImageResource::didLoadFinished()
{
    bool isSVG = false;
    if (m_resourceRequest) {
        auto m =
            MimeType::parseFromString(m_resourceRequest->responseMimeType());
        isSVG = m.subtype()->contains("svg", false);
    }
    if (!isSVG) {
        isSVG = url()->urlString()->endsWith(".svg", false);
    }

    if (isSVG) {
        // FIXME
        // content of SVGDocument loaded twice from ImageResource &
        // HTMLIFrameElement
        m_mockFrameForSVGDocument =
            new MockHTMLIFrameElement(loader()->document(), this);
        m_mockFrameForSVGDocument->navigate(
            url(), HistoryManager::Action::Add,
            loader()->document()->documentURI());
        m_imageData = nullptr;
        return;
    }
#if defined(PORT_CANVAS_BACKEND_EFL)
    if (!m_url->isFileURL()) {
        m_imageData =
            NativeImageData::create(m_resourceRequest->response().data(),
                                    m_resourceRequest->response().size());
        if (!m_imageData) {
            Resource::didLoadFailed();
            return;
        }
    }
#else
    m_imageData = NativeImageData::create(m_resourceRequest->response().data(),
                                          m_resourceRequest->response().size());
    if (!m_imageData) {
        Resource::didLoadFailed();
        return;
    }
#endif
    Resource::didLoadFinished();
}
}
