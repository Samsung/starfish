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
#include "core/dom/ExecutionContext.h"
#include "core/modules/canvas/image/ImageDecoder.h"
#include "core/modules/threading/ThreadPool.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/extra/MimeType.h"
#include "core/dom/HTMLIFrameElement.h"
#include "core/dom/Traverse.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/page/BrowsingContext.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "core/modules/canvas/image/CompressedNativeImageData.h"
#include "core/modules/canvas/image/AnimatedGIFNativeImageData.h"
#include "core/modules/canvas/image/SVGNativeImageData.h"
#include "platform/loader/ImageResource.h"
#include "platform/loader/ResourceLoader.h"
#include "platform/file/File.h"

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
            if (!w || !h) {
                // Should not dispatch 'error' event
                // test/cairo/reftest/vendor/blink_original/svg/as-image/zero-size-svg-image-with-data-uri.html
                m_resource->Resource::didLoadFinished();
                return;
            }
            NativeImageData* imageData =
                SVGNativeImageData::create(w, h, svgBox);
            STARFISH_ASSERT(imageData != nullptr);
            imageData->clear();

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
                Base64Utils::encodeBase64HTMLDataURI(resposeTextUTF8, "svg");
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

#if defined(STARFISH_ENABLE_MULTI_THREAD_IMAGE_DECODING)
    if (!m_resourceRequest->isSync() &&
        m_resourceRequest->executionContext()->hasDocument()) {
        if (m_resourceRequest->executionContext()
                ->document()
                ->webView()
                ->isThereURLInActiveImageURLsInRenderingSet(
                    url()->urlString()->toUTF8NonGCString())) {
            struct ImageDecodeData : public gc {
                ResponseBody responseData;
                ImageResource* imageResource;
                ImageDecoder::DecodeResult decodeResult;
            };

            ImageDecodeData* d = new ImageDecodeData();
            d->imageResource = this;
            d->responseData = std::move(m_resourceRequest->response());

            m_resourceRequest->executionContext()
                ->document()
                ->webView()
                ->imageDecodeThreadPool()
                ->addWork(
                    m_resourceRequest->executionContext(),
                    [](void* data) -> void* {
                        STARFISH_ASSERT(data != nullptr);
                        ImageDecodeData* d = (ImageDecodeData*)data;

                        ImageDecoder id(d->responseData);
                        d->decodeResult = id.decode();
                        if (d->decodeResult.m_isAnimatedGIF) {
                            free(d->decodeResult.m_buffer);
                        }
                        d->imageResource->resourceRequest()
                            ->webBase()
                            ->messageLoop()
                            ->addIdlerWithNoGCRootingInOtherThread(
                                nullptr,
                                [](size_t handle, void* data) {
                                    STARFISH_ASSERT(data != nullptr);
                                    ImageDecodeData* d = (ImageDecodeData*)data;
                                    ResponseBody buffer =
                                        std::move(d->responseData);

                                    if (d->decodeResult.m_isSuccessful) {
                                        if (d->decodeResult.m_isAnimatedGIF) {
                                            d->imageResource->m_imageData =
                                                AnimatedGIFNativeImageData::
                                                    create(
                                                        buffer,
                                                        d->imageResource->url()
                                                            ->urlString()
                                                            ->toUTF8NonGCString(),
                                                        d->decodeResult.m_width,
                                                        d->decodeResult
                                                            .m_height,
                                                        d->decodeResult
                                                            .m_stride);
                                        } else {
                                            d->imageResource->m_imageData =
                                                CompressedNativeImageData::create(
                                                    buffer,
                                                    d->imageResource->url()
                                                        ->urlString()
                                                        ->toUTF8NonGCString(),
                                                    d->decodeResult.m_buffer,
                                                    d->decodeResult.m_width,
                                                    d->decodeResult.m_height,
                                                    d->decodeResult.m_stride);
                                        }
                                        d->imageResource
                                            ->Resource::didLoadFinished();
                                    } else {
                                        d->imageResource
                                            ->Resource::didLoadFailed();
                                    }
                                },
                                d);

                        return nullptr;
                    },
                    d);

            return;
        }
    }
#endif
    if (m_resourceRequest->response().size() != 0) {
        if (ImageDecoder::isAnimatedGIF(m_resourceRequest->response())) {
            ImageDecoder id(m_resourceRequest->response());
            auto result = id.decodeJustImageSize();
            m_imageData = AnimatedGIFNativeImageData::create(
                m_resourceRequest->response(),
                url()->urlString()->toUTF8NonGCString(), result.m_width,
                result.m_height, result.m_stride);
        } else {
            m_imageData = CompressedNativeImageData::create(
                m_resourceRequest->response(),
                url()->urlString()->toUTF8NonGCString());
        }
    }

    if (!m_imageData) {
        Resource::didLoadFailed();
        return;
    }
    Resource::didLoadFinished();
}
}
