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
#include "core/dom/HTMLImageElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "platform/loader/ElementResourceClient.h"
#include "core/modules/message_loop/MessageLoop.h"

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

        if (m_element->frame()) {
            if (sizeBefore == sizeNow) {
                m_element->setNeedsPainting();
            } else {
                m_element->setNeedsLayout();
            }
        }
    }

protected:
    HTMLImageElement* m_element;
};

void HTMLImageElement::setSrc(String* src)
{
    setAttribute(starfish()->staticStrings()->m_src, src);
}

String* HTMLImageElement::src()
{
    if (hasAttribute(starfish()->staticStrings()->m_src) != SIZE_MAX) {
        return (new ResourceURL(
                    getAttributeOrEmpty(starfish()->staticStrings()->m_src),
                    document()->baseURI()))
            ->urlString();
    } else {
        return String::emptyString;
    }
}

unsigned long HTMLImageElement::width()
{
    unsigned long result = 0;
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

void HTMLImageElement::setWidth(unsigned long width)
{
    setAttribute(starfish()->staticStrings()->m_width, String::fromInt(width));
}

unsigned long HTMLImageElement::height()
{
    unsigned long result = 0;
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

void HTMLImageElement::setHeight(unsigned long height)
{
    setAttribute(starfish()->staticStrings()->m_height,
                 String::fromInt(height));
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
    if (frame()) {
        setNeedsLayout();
    }
}

void HTMLImageElement::loadImage(String* src)
{
    unloadImage();
    m_imageResource = document()->resourceLoader().fetchImage(
        new ResourceURL(src, document()->baseURL()->baseURI()),
        isInDocumentScopeAndDocumentParticipateInRendering());
    m_imageResource->addResourceClient(
        new ImageDownloadClient(this, m_imageResource));
    m_imageResource->addResourceClient(
        new ElementResourceClient(this, m_imageResource));

    GET_EFFECTIVE_REFERRERPOLICY();
    ResourceURL* rUrl = new ReferrerURL(document()->documentURI(), policy);
    if (rUrl->isFileURL()) {
        m_imageResource->request(Resource::ResourceRequestSyncLevel::AlwaysSync,
                                 rUrl, true);
    } else {
        m_imageResource->request(
            Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded, rUrl,
            true);
    }
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
}
