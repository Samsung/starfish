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
#include "core/dom/Event.h"
#include "core/dom/HTMLImageElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "platform/loader/ElementResourceClient.h"
#include "platform/loader/ResourceLoader.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/dom/WebOrigin.h"
#include "core/modules/message_loop/Timer.h"

namespace Starfish {

void* HTMLImageElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLImageElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLImageElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLImageElement, m_imageResource));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLImageElement, m_imageData));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLImageElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

class ImageDownloadClient : public ResourceClient {
public:
    ImageDownloadClient(HTMLImageElement* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        updateImage(m_element->document()->brokenImage());
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        updateImage(m_resource->asImageResource()->imageData());
    }

    void updateImage(NativeImageData* imageData)
    {
        m_element->m_requestErrorType = m_resource->requestErrorType();
        m_element->m_imageResource = nullptr;
        NativeImageData* imageDataBefore = m_element->imageData();
        if (!imageData) {
            // imageData can be null in mock port
            return;
        }

        LayoutSize sizeBefore(0, 0);
        LayoutSize sizeNow(imageData->width(), imageData->height());

        if (imageDataBefore) {
            sizeBefore =
                LayoutSize(imageDataBefore->width(), imageDataBefore->height());
        }

        m_element->m_imageData = imageData;
        if (imageData->hasAnimatedGIF()) {
            m_element->updateFrame(0);
        } else {
            if (m_element->frame()) {
                m_element->setNeedsPainting();
                if (sizeBefore != sizeNow) {
                    m_element->setNeedsLayout();
                }
            }
        }
    }

protected:
    HTMLImageElement* m_element;
};

HTMLImageElement::HTMLImageElement(Document* document)
    : HTMLImageElement(document, document->staticStrings()->m_imgTagName)
{
}

HTMLImageElement::HTMLImageElement(Document* document,
                                   const QualifiedName& qname)
    : HTMLElement(document, qname)
    , m_imageResource(nullptr)
    , m_imageData(nullptr)
    , m_requestErrorType(RequestErrorType::NoError)
{
}

HTMLImageElement::HTMLImageElement(Document* document, uint32_t width)
    : HTMLImageElement(document)
{
    setWidth(width);
}

HTMLImageElement::HTMLImageElement(Document* document, uint32_t width,
                                   uint32_t height)
    : HTMLImageElement(document, width)
{
    setHeight(height);
}

void HTMLImageElement::setSrc(String* src)
{
    setAttribute(starfish()->staticStrings()->m_src, src);
}

String* HTMLImageElement::src()
{
    if (hasAttribute(starfish()->staticStrings()->m_src) != SIZE_MAX) {
        return origin()->urlString();
    } else {
        return String::emptyString;
    }
}

Nullable<String*> HTMLImageElement::crossOrigin()
{
    return getAttribute(starfish()->staticStrings()->m_crossorigin);
}

void HTMLImageElement::setCrossOrigin(Nullable<String*> crossOrigin)
{
    if (crossOrigin.hasValue()) {
        setAttribute(starfish()->staticStrings()->m_crossorigin,
                     crossOrigin.getValue());
    } else {
        removeAttribute(starfish()->staticStrings()->m_crossorigin);
    }
}

uint32_t HTMLImageElement::width()
{
    uint32_t result = 0;
    String* widthStr =
        getAttributeOrEmpty(starfish()->staticStrings()->m_width);
    String* heightStr =
        getAttributeOrEmpty(starfish()->staticStrings()->m_height);

    if (widthStr->equals(String::emptyString)) {
        if (m_imageData) {
            if (heightStr->equals(String::emptyString)) {
                result = m_imageData->width();
            } else {
                result = String::parseInt(heightStr) *
                         (m_imageData->width() / m_imageData->height());
            }
        }
    } else {
        result = String::parseInt(widthStr);
    }
    return result;
}

void HTMLImageElement::setWidth(uint32_t width)
{
    setAttribute(starfish()->staticStrings()->m_width, String::fromInt(width));
}

uint32_t HTMLImageElement::height()
{
    uint32_t result = 0;
    String* widthStr =
        getAttributeOrEmpty(starfish()->staticStrings()->m_width);
    String* heightStr =
        getAttributeOrEmpty(starfish()->staticStrings()->m_height);

    if (heightStr->equals(String::emptyString)) {
        if (m_imageData) {
            if (widthStr->equals(String::emptyString)) {
                result = m_imageData->height();
            } else {
                result = String::parseInt(widthStr) *
                         (m_imageData->height() / m_imageData->width());
            }
        }
    } else {
        result = String::parseInt(heightStr);
    }
    return result;
}

void HTMLImageElement::setHeight(uint32_t height)
{
    setAttribute(starfish()->staticStrings()->m_height,
                 String::fromInt(height));
}

uint32_t HTMLImageElement::naturalWidth()
{
    if (m_imageData) {
        // TODO : Apply a current pixel density when it is implemented.
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return m_imageData->width();
    }
    return 0;
}

uint32_t HTMLImageElement::naturalHeight()
{
    if (m_imageData) {
        // TODO : Apply a current pixel density when it is implemented.
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        return m_imageData->height();
    }
    return 0;
}

void HTMLImageElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starfish()->staticStrings()->m_src &&
        !inHTMLConstructionSite()) {
        if (value->length() && document()->doesParticipateInRendering()) {
            loadImage(value);
        } else {
            unloadImage();
        }
    } else if (name == starfish()->staticStrings()->m_width ||
               name == starfish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    } else if (name == starfish()->staticStrings()->m_crossorigin) {
        if (attributeRemoved) {
            removeAttribute(value);
        } else if (attributeCreated || !old->equalsIgnoreCase(value)) {
            if (value->equalsIgnoreCase("use-credentials")) {
                setAttribute(starfish()->staticStrings()->m_crossorigin, value);
            } else {
                setAttribute(starfish()->staticStrings()->m_crossorigin,
                             String::fromUTF8("anonymous"));
            }
        }
    }
}

void HTMLImageElement::didNodeAdopted()
{
    HTMLElement::didNodeAdopted();
    if (document()->doesParticipateInRendering()) {
        Nullable<String*> srcStr =
            getAttribute(starfish()->staticStrings()->m_src);
        if (srcStr.hasValue() && srcStr.getValue()->length() > 0) {
            loadImage(srcStr.getValue());
        }
    } else {
        unloadImage();
    }
}

void HTMLImageElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    auto value = src();
    if (value->length() && document()->doesParticipateInRendering()) {
        loadImage(value);
    } else {
        unloadImage();
    }
}

void HTMLImageElement::unloadImage()
{
    if (m_imageResource) {
        m_imageResource->cancel();
        m_imageResource = nullptr;
    }
    m_imageData = nullptr;
    m_requestErrorType = RequestErrorType::NoError;
    if (frame()) {
        setNeedsLayout();
        setNeedsPainting();
    }
}

void HTMLImageElement::loadImage(String* src)
{
    auto resourceURL = new ResourceURL(src, document()->baseURL()->baseURI());
    unloadImage();
    if (!document()->contentSecurityPolicy()->allowSource(CSPDirectives::ImgSrc,
                                                          resourceURL)) {
        String* eventType = starfish()->staticStrings()->m_error.localName();
        Event* e =
            new Event(executionContext(), eventType, EventInit(false, false));
        dispatchEventIdleTimeByUA(e);
        return;
    }

    m_imageResource = document()->resourceLoader().fetchImage(resourceURL);
    m_imageResource->addResourceClient(
        new ImageDownloadClient(this, m_imageResource));
    m_imageResource->addResourceClient(
        new ElementResourceClient(this, m_imageResource));

    GET_EFFECTIVE_REFERRERPOLICY();
    RequestData* reqData = new RequestData();
    reqData->m_url = m_imageResource->url();
    reqData->m_referrer = new ReferrerURL(document()->documentURI(), policy);
    reqData->m_destination = RequestDestination::Image;

    if (reqData->m_url->isFileURL()) {
        reqData->m_syncLevel = RequestSyncLevel::AlwaysSync;
    } else {
        reqData->m_syncLevel = RequestSyncLevel::SyncIfAlreadyLoaded;
    }

    auto crossOrigin = getAttribute(starfish()->staticStrings()->m_crossorigin);
    if (crossOrigin.hasValue()) {
        reqData->m_mode = RequestMode::CORS;
        reqData->m_credentials =
            crossOrigin.getValue()->equalsIgnoreCase("use-credentials")
                ? RequestCredentials::Include
                : RequestCredentials::SameOrigin;
    } else {
        reqData->m_mode = RequestMode::NoCORS;
    }

    m_imageResource->request(reqData, true);
}

String* HTMLImageElement::referrerPolicy()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_referrerpolicy);
}

void HTMLImageElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(starfish()->staticStrings()->m_referrerpolicy, policy);
    }
}

String* HTMLImageElement::nameAttr()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_name);
}

void HTMLImageElement::setNameAttr(String* name)
{
    setAttribute(starfish()->staticStrings()->m_name, name);
}

WebOrigin* HTMLImageElement::webOrigin()
{
    return WebOrigin::createDocumentOrigin(origin());
}

ResourceURL* HTMLImageElement::origin()
{
    if (hasAttribute(starfish()->staticStrings()->m_src) != SIZE_MAX) {
        return new ResourceURL(
            getAttributeOrEmpty(starfish()->staticStrings()->m_src),
            document()->baseURI());
    } else {
        return new ResourceURL(String::emptyString);
    }
}

bool HTMLImageElement::hasRequestError()
{
    return m_requestErrorType != RequestErrorType::NoError;
}

void HTMLImageElement::updateFrame(size_t delay)
{
    if (m_updateFrameTimer) {
        document()->webView()->timer()->removeTimer(m_updateFrameTimer);
    }
    m_updateFrameTimer = document()->webView()->timer()->addTimer(
        delay * 10, nullptr,
        [](void* data) {
            HTMLImageElement* imageElement = (HTMLImageElement*)data;
            if (imageElement->m_imageData != nullptr) {
                if (imageElement->frame()) {
                    imageElement->setNeedsPainting();
                }
                imageElement->m_imageData->prepareNextFrame();
                size_t delay = imageElement->m_imageData->delay();
                imageElement->updateFrame(delay);
            }
        },
        this, false);
}
}
