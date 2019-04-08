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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
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
#include "core/csp/ContentSecurityPolicy.h"

namespace Starfish {

class MockHTMLIFrameElement : public HTMLIFrameElement {
public:
    MockHTMLIFrameElement(Document* document, ImageResource* resource)
        : HTMLIFrameElement(
              document, document->starfish()->staticStrings()->m_iframeTagName)
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
            m_browsingContext->layoutIfNeeded();
            SVGSVGElement* svg = r->asSVGSVGElement();
            if (!svg->frame()) {
                m_resource->didLoadFailed();
                return;
            }
            FrameSVGSVGBox* svgBox = (FrameSVGSVGBox*)svg->frame();

            size_t w = (int)svgBox->width();
            size_t h = (int)svgBox->height();
            NativeImageData* imageData = NativeImageData::create(w, h);
            imageData->clear();
            Canvas* canvas = Canvas::create(
                m_browsingContext->webView(), imageData->data(),
                imageData->width(), imageData->height(), imageData->stride());
            svgBox->paintReplaced(canvas);
            delete canvas;

            m_resource->m_imageData = imageData;
            m_resource->m_mockFrameForSVGDocument = nullptr;
            m_resource->m_imageData->setPreserveAspectRatioValue(
                svg->preserveAspectRatioValue());
            m_resource->Resource::didLoadFinished();
        } else {
            m_resource->didLoadFailed();
        }
    }

    ImageResource* m_resource;
};

void ImageResource::didLoadFinished()
{
    if (!m_resourceRequest) {
        Resource::didLoadFailed();
        return;
    }

    bool isSVG = false;
    auto m = MimeType::parseFromString(m_resourceRequest->responseMimeType());
    isSVG = m.subtype()->contains("svg", false);

    if (!isSVG) {
        isSVG = url()->urlString()->endsWith(".svg", false);
    }

    if (isSVG) {
        TextConverter* converter = new TextConverter(
            m_resourceRequest->responseMimeType(),
            m_resourceRequest->executionContext()->characterSet(),
            m_resourceRequest->response().data(),
            m_resourceRequest->response().size());
        String* resposeText =
            converter->convert(m_resourceRequest->response().data(),
                               m_resourceRequest->response().size(), true);

        // TODO enhance this heuristic algorithm
        String* testText = resposeText->substring(
            0, resposeText->length() > 128 ? 128 : resposeText->length());
        if (testText->contains("<svg") || testText->startsWith("<?xml")) {
            auto resposeTextUTF8 = resposeText->toUTF8NonGCString();
            auto dataURI =
                StringUtils::toBase64HTMLDataURI(resposeTextUTF8, "svg");
            m_mockFrameForSVGDocument =
                new MockHTMLIFrameElement(loader()->document(), this);
            m_mockFrameForSVGDocument->navigate(
                new ResourceURL(
                    String::fromUTF8(dataURI.data(), dataURI.length())),
                HistoryManagerAction::Add,
                new ReferrerURL(loader()->document()->documentURI()),
                CustomHTMLIFrameElementType::SVG);
            m_imageData = nullptr;
            return;
        }
    }

    m_imageData = NativeImageData::create(m_resourceRequest->response().data(),
                                          m_resourceRequest->response().size());
    if (!m_imageData) {
        Resource::didLoadFailed();
        return;
    }
    Resource::didLoadFinished();
}
}
